#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define __USE_GNU
#define _GNU_SOURCE
#include <dlfcn.h>


#define STRX(x) #x
#define STR(x) STRX(x)
#define LOGHDR "[" __FILE__ ":" STR(__LINE__) "] "

// SET to 1  to enable log-traces  or set HOOKUP_VERBOSE env-variable to non-zero
static int hookup_verbose = 0;

static 
struct {
    int   fd;
    FILE *p1;
} track_files [1024];
static int track_files_cap = (sizeof(track_files)/sizeof(*track_files)) ;
static int track_files_cnt = 0 ;

__attribute__((constructor)) void so_init(void) 
{
    char *hv = 0 ;
    if( (hv=getenv("HOOKUP_VERBOSE" )) ) { if(atoi(hv)) hookup_verbose  = 1; }
    if(hookup_verbose) {
        dprintf(2, LOGHDR "library loaded!\n");
    }
}

__attribute__((destructor)) void so_release(void) 
{
    if(hookup_verbose)  {
        dprintf(2, LOGHDR  "library UNLOADed !\n");
    }
}


// Saave function pointer to real functions here .
int (*__real_open)    (const char *pathname, int flags, mode_t mode) = NULL;
int (*__real_close)   (int fd ) = NULL;
int (*__real_fstat)   (int fd, struct stat *statbuf) = NULL;
int (*__real_stat)   (const char *pathname, struct stat *statbuf) = NULL;

void setup_hook( void **ptr )
{
    if( ptr && *ptr == __real_open  ) __real_open  = dlsym(RTLD_NEXT ,  "open");
    if( ptr && *ptr == __real_close ) __real_close = dlsym(RTLD_NEXT , "close");
    if( ptr && *ptr == __real_fstat ) __real_fstat = dlsym(RTLD_NEXT , "fstat");
    if( ptr && *ptr == __real_stat  ) __real_stat  = dlsym(RTLD_NEXT ,  "stat");    
}

int 
fstat(int fd, struct stat *statbuf)
{
    if ( ! __real_fstat ) {
        setup_hook( (void*)& __real_fstat ); 
    }
    
    // dprintf(2, LOGHDR "fstat: fd:#%d\n", fd ) ;
    return __real_fstat(fd, statbuf);
}

int 
stat(const char *path, struct stat *statbuf)
{
    if ( ! __real_stat ) {
        setup_hook( (void*)& __real_stat ); 
    }
    // dprintf(2, LOGHDR "stat: [%s]\n", path ) ;

    
    { // 
        char key[]="{curl:" ; //  "}" -- to not distress my editor 
        // char *url = 0;
        // int urllen ; 
        char *p = strstr( path, key ); 
        int isdir_check = 0;
        if(!p) {
            return __real_stat(path, statbuf) ;     // <-- EXIT  return 
        }
        
        
        char *start = p + strlen(key) ;
        char *end = strrchr(p, '}') ;  
        if(end == 0) {
            isdir_check = 1;
            end = start + strlen(start);
        }
        int len  = end-start ;
        char *url = start ;
        if(hookup_verbose)  {dprintf(2, LOGHDR "HOOKED [%d] stat [%s] :[%.*s]\n", getpid(),  p, len , start) ; }
        // int urllen = len;
        
        // fake stat of current directory 
        if(isdir_check) {
            return __real_stat("./", statbuf) ;    // <-- EXIT  return 
        } 
        else 
        { 
            char tempfile_name[4096] = "./curl_include_XXXXXX.tmp" ;
            int temp_file_fd = -1;
            int tempfile_name_unliked = 0;
            do { // do {} while(0) for break && clean ;
                // Note: !!!  This block most probably woun't be used by clang !!!
                // download to temp file 
                // TODO : instead of unlink -- reuse downloaded tempfile in hooked open 
                temp_file_fd = mkstemp(tempfile_name) ;
                if(temp_file_fd < 0 ) {
                    if(hookup_verbose) { dprintf(2, LOGHDR "EE: hook cant create tempfile\n") ; }
                    errno = EINVAL ;
                    break;
                }
                int tempfile_name_len = strlen(tempfile_name);
                if(hookup_verbose) {  dprintf(2, LOGHDR "EE: hook stat :download to tempfile [%s]\n", tempfile_name) ; }
            
                char command[4000]; 
                snprintf(command, sizeof(command)-1, 
                      "( unset LD_PRELOAD ;  curl \"%s\" > %.*s ) "
                    , url, tempfile_name_len, tempfile_name 
                ) ;
                
                int res = system(command) ;
                if(res!=0) {  
                    if(hookup_verbose) {  dprintf(2, LOGHDR "EE: hook downaload url \n") ; }
                    errno = EINVAL ;
                    break;
                }
                
                if(temp_file_fd<0) { __real_close(temp_file_fd) ; { temp_file_fd = -1; } }
                unlink(tempfile_name) ; tempfile_name_unliked = 1 ; 
            
                return __real_stat(tempfile_name, statbuf) ;
            } while(0) ;
            
            // cleanup tempfile 
            if(! tempfile_name_unliked) {  unlink(tempfile_name);  tempfile_name_unliked=1; }
            if(temp_file_fd >=0 ) { __real_close(temp_file_fd) ; temp_file_fd = -1; }
        } // temfile download && stat 
    }
    
    return __real_stat(path, statbuf);
}

int
open( const char *pathname, int flags, mode_t mode) 
{
    if ( ! __real_open ) {
        setup_hook( (void*)& __real_open ); 
    }
    
    { // HOOKED open [{curl:https://raw.githubusercontent.com/tsoding/sv/master/sv.h}] 
        char key[]="{curl:" ; //  "}" -- to not distress my editor 
        char *url = 0; int urllen= 0;
        if( pathname )
        {
            char *p = strstr( pathname, key ); 
            if(p) {
                char *start = p + strlen(key) ;
                char *end = strrchr(p, '}') ;
                int len  = end-start ;
                url = start ;
                if(hookup_verbose)  {
                    dprintf(2, LOGHDR "HOOKED open [%s] :[%.*s]\n", p, len , start) ;
                }
                urllen = len;
            }
            
            if(url) {
                char command[4096] ; snprintf(command, sizeof(command)-1
                    , "curl -s \"%.*s\"" , urllen, url 
                );
                if(hookup_verbose) { dprintf(2, LOGHDR "HOOKED open  command :[%s]\n", command ) ; }
                
                if( track_files_cnt >= track_files_cap )
                { 
                    if(hookup_verbose) { dprintf(2, LOGHDR "NO more room to trackfiles\n") ;  }
                    errno = EMFILE;
                    goto failed ;                                       // <-- mark : EXIT 
                }
                FILE *p1 = popen(command, "r") ; 
                if(p1) {
                    int fd = fileno(p1);
                    if(fd<0) {
                        errno = EMFILE; 
                        goto failed;                                    //  <--mark : EXIT 
                    }
                    if(hookup_verbose) { dprintf(2, LOGHDR "open subprocess [p1:%p] for [fd:#%d]\n", p1, fd) ; }
                    track_files[track_files_cnt].fd =  fd;
                    track_files[track_files_cnt].p1 =  p1;
                    track_files_cnt++; 
                    return fd; 
                }
            }
        }
    }
    return __real_open( pathname, flags, mode);
failed :
    return -1; 
}


int close(int fd) 
{
    if ( ! __real_close ) {
        setup_hook( (void*)&__real_close );
    }
    
    FILE *p2close = 0 ; 
    { // search fd in track_files_cap  and do pclose() on related process-FILE 
        int found_idx = -1; 

        for(int i=0;i<track_files_cnt; i++) 
        {
            if(fd == track_files[i].fd) 
            {
                if(hookup_verbose) {  dprintf(2, LOGHDR "TT close (#%d) .. tracked by hook [p1:%p]\n", fd, track_files[i].p1) ; }
                if(track_files[i].p1) { 
                    p2close =  track_files[i].p1 ;
                    track_files[i].p1 = NULL;
                    track_files[i].fd = -1 ;
                }; 
                found_idx = i;
                break;
            }
            if (track_files[i].fd == -1) {
                found_idx = -1;
                break;
            }
        } // for(...)
        
        if( track_files_cnt>1 ) 
        {
            if(found_idx >= 0)  // just "if any" ..
            { 
                // close "gap" in array  by move last-element in place of removed one 
                track_files[found_idx] = track_files[track_files_cnt];
                track_files[track_files_cnt].fd = -1;
                track_files[track_files_cnt].p1 = NULL;
                track_files_cnt -- ;
            }
        }
    }
        
    int res =  __real_close(fd) ; //  first close 
    if(p2close) pclose(p2close) ;  // then pclose 
    return res; 
}
