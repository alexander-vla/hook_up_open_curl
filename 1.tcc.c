#include <stdio.h>
#include <unistd.h>
#include <time.h>

// 1) build libinclude_hook.so 
//  $ make    
//  or 
//  $ gcc -Wall -g -O0 -shared -fPIC -o libinclude_hook.so  libinclude_hook.c

// Usage  : 
//   with TCC  :  $ LD_PRELOAD=./libinclude_hook.so     tcc -run ./1.tcc.c  
//   with GCC  :  $ ( LD_PRELOAD=./libinclude_hook.so   gcc -o 1 1.tcc.c && exit 0 ) && ./1 
//   with clang:  $ ( LD_PRELOAD=./libinclude_hook.so clang -o 1 1.tcc.c && exit 0 ) && ./1 

#define SV_IMPLEMENTATION
#include "{curl:https://raw.githubusercontent.com/tsoding/sv/master/sv.h}"

#define STB_SPRINTF_IMPLEMENTATION
#include "{curl:https://raw.githubusercontent.com/nothings/stb/master/stb_sprintf.h}"


char *random_string(char *b, int nn)
{
    static int first  = 1; if(first) { srand( ( time(NULL) & getpid() ) ^ 0x112345  ); first= 0 ; }
    const char dict[] = "0123456789"  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" ;
    for(int i =0 ; i<nn; i++) {
        int idx = (random() % (sizeof(dict)-1)) ;
        b[i] = dict[idx] ; 
    }
    return b;    
}


int main()
{
    char msg[] = "stuff: Hellow World !" ;

    // Do some stuff to ensure all works 
    String_View svv = sv_from_cstr(msg) ;
    String_View sv1 ;
    sv1=svv ;
    dprintf(2,  "<"SV_Fmt">\n"
                "<"SV_Fmt">\n"
        ,  SV_Arg( sv_chop_by_delim(&svv, ' ')  )  
        ,  SV_Arg(sv1)
    ) ;
    

    // Do some other stuff to ensure all works 
    for(int i = 0; i < 1e1; i++ ) 
    {
        char b[1024] ; int o=0; char b1[24] ; 
        int vv = (random() % 1000 ) ;
        o+=stbsp_snprintf(b+o, sizeof(b)-o, "[%d] ", i );
        o+=stbsp_snprintf(b+o, sizeof(b)-o, "%03d", vv );
        o+=stbsp_snprintf(b+o, sizeof(b)-o,"-%06.2f", ((float)vv) + 0.5 );
        o+=stbsp_snprintf(b+o, sizeof(b)-o,"-%.*s \n", 3 , random_string(b1, 3) );
        {int wr = write(2, b, o) ;  if(wr<=0) { dprintf(2 , "EE: WR:%d\n", wr) ; break; }  }
    }
    
    dprintf(2, "Done\n"); 
    return 0;
}
