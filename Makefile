# "hookup-library"  tcc / gcc  
# clang -- not works -- it even not try to open file . 
# upd : clang actually works - but it request stats prior to open file via stat/fstat directory first 
#       add preper hook to stat - resolves issue .
# 
# Not a problem to run other simple tools using libc-open and 
# eg : $ LD_PRELOAD=./libinclude_hook.so  cat "{curl:https://raw.githubusercontent.com/tsoding/sv/master/sv.h}" | cloc 

CC=gcc
CFLAGS= -Wall -g -O0

.PHONY: all
all: libinclude_hook.so 1_clang 1_gcc 1_tcc 1_tcc_run

libinclude_hook.so: libinclude_hook.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@  $<

1_clang: 1.tcc.c libinclude_hook.so
	LD_PRELOAD=./libinclude_hook.so  clang  $(CFLAGS) -o $@  $<

1_gcc:  1.tcc.c libinclude_hook.so
	LD_PRELOAD=./libinclude_hook.so  gcc  $(CFLAGS) -o $@  $<

1_tcc:  1.tcc.c libinclude_hook.so
	LD_PRELOAD=./libinclude_hook.so  tcc  $(CFLAGS) -o $@  $<

1_tcc_run:  1.tcc.c libinclude_hook.so
	LD_PRELOAD=./libinclude_hook.so  tcc   $(CFLAGS) -run  $<


.PHONY: clean
clean:
	rm -fv libinclude_hook.so 
	rm -fv 1_clang 1_gcc 1_tcc



