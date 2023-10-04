/*
 * Preservation Virtual Machine Project
 * Startup code for GNU compiler for ivm64
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: May 2020 - Jul 2023
 *
 */

/*
 A multipurpose crt0.c for the ivm64 architecture that parses the
 argument file and converts it into an char *argv[] structure which is
 passed to main().

 Also it processes an environment file following the argument file
 generating the global char *environ[] structure, which is also passed
 to main() and can be used with getenv()/setenv().

 This crt0 is valid with and without entry point.

 The separator of different arguments is the null char (\0)
 to be compatible with the format of linux file
 /proc/<pid>/cmdline. The same applies to the environment
 file which needs to have the same format as /proc/<pid>/environ
 in linux.
*/

/*
 This version also provides the possibility to make a "return"
 instead of an "exit" when called from the initialization ivm binary preamble
 if the enviroment variable IVM_CRT0_EXIT_POLICY is set to MUST_RETURN,
 and the entry point _start is used.
 Otherwise, it returns in the normal way that without entry point returns
 to the "__start" code which will halt the ivm machine with "exit" insn.
*/


/*
 The ivm startup and memory layout is described in this
 file "Assembler.Tests/test_code/entry_points/main_entry.s"

 As of ivm v0.25 the memory layout and startup are different
 with or without entry point (ivm as -e entry)

   With NO entry point:                     With entry point:
    +-----------------+ 0x0                  +-----------------+ 0x0
    |      ...        |                      |      ...        |
    | #initialization |                      | #initialization |
    | #code           |                      | #code           |
    |      ...        |                      |      ...        |
    |                 |                      | push! heapStart |
    |                 |                      | push! argLength |
    |                 |                      | push! argStart+8|
    |                 |                      | call! _start    |
    | jump! progStart |                      |                 |
    |      ...        |                      |      ...        |
    +-----------------+                      |                 |
    | argStart        |                      |                 |
    | heapStart       |                      |                 |
    +-----------------+                      +-----------------+
    | #program        | <- progStart         | #program        | <- progStart
    | __start:        |                      | __start:        |
    |                 |                      |                 |
    |                 |                      | _start:         | <- entry point
    |                 |                      |                 |
    |      ...        |                      |      ...        |
    +-----------------+                      +-----------------+
    | argLenght       | <- argStart          | argLength       | <- argStart
    |                 |                      |                 | <- argStart+8
    |      ...        |                      |      ...        |
    +-----------------+                      +-----------------+
    | envLenght       | <- envStart          | envLength       | <- envStart
    |                 |                      |                 | <- envStart+8
    |      ...        |                      |      ...        |
    +-----------------+                      +-----------------+
    |                 | <- space             |                 | <- space
    |      ...        |                      |      ...        |
    +-----------------+                      +-----------------+
    |                 | <- heapStart         |                 | <- heapStart
    |      ...        |                      |      ...        |
    +-----------------+                      +-----------------+
    |                 |                      |                 |
    |                 | <- SP                |                 | <- SP
    |                 |                      |                 |
    |                 |                      |                 |
    |                 | high |               |                 | high |
                             V                                        V


 Argument file area is placed after the program with a first word containing
 its size. If an entry point is set, the value passed as args_start points to
 the first data byte of the argument area, skipping the word with the size.

  (ivm v0.25) with NO -e: <arg start> is a pointer to a memory area with this structure
  +------------------------------------+-------+--------+----- -+-------+
  | no. of bytes of arguments (1 word) | byte0 | byte 1 | ....  | N-1   |
  +------------------------------------+-------+--------+----- -+-------+

  (ivm v0.25) with -e: passed <arg start> points to the first byte of the argument area:
  (that is, it is args_start_with_no_entry_point+8)
                                       +-------+--------+----- -+-------+
                                       | byte0 | byte 1 | ....  | N-1   |
                                       +-------+--------+----- -+-------+

  In adition to the argument file, this run-time considers also a zone
  with the same format placed after the argument file area for the environment
  variables: the first word of the environment area is its size.
  (Note that this second envirnment area is not supported the F# ivm implementation)
  +----------------------------------------+-------+--------+----- -+-------+
  | N = no. of bytes of arguments (1 word) | byte0 | byte 1 | ....  | N-1   |
  +----------------------------------------+-------+--------+----- -+-------+
  | M = no. of bytes of environ.  (1 word) | byte0 | byte 1 | ....  | M-1   |
  +----------------------------------------+-------+--------+----- -+-------+
  The memory must be zero initialized before the load of the program and argument
  file (at least the next word ater the argument file, in case no enviroment
  was loaded, this position must be zero (no bytes in enviroment area))
*/


#define NULL ((void*)0)
#define IS_END(c) ((c=='\0') || (c==12))

// Start here with no entry point
__attribute__((no_reorder))
void __start()
{
    long _start(void *, unsigned long, void*, ...);
    long ret = _start(NULL, 0, NULL);
    asm volatile ("exit");
    asm volatile ("" :: "r"(ret));
}

/* Global variable '__IVM64_heap_pointer__' must contain the
   pointer to the start of the heap. It is used by sbrk/malloc;
   It can be declared at the end of the full assembly file
   by means of the 'space' directive. With that aim, the
   the crtn.o can be used if it exists, or the linker can add
   it at the end. */
extern char* __IVM64_heap_pointer__;

/* Global variables '__IVM64_exit_jb__' and '__IVM64_exit_val__'
   are declared in libgloss/ivm64/_exit.c */
#include <setjmp.h>
extern jmp_buf * __IVM64_exit_jb__;
extern unsigned long __IVM64_exit_val__;

int main(int argc, char* argv[], char* environ[]);
void exit(int status);
void *memset(void *s, int c, long n);
void *memcpy(void *dest, const void *src, long n); // newlib's memcpy does not check overlapping
char *getenv(char *);
int unsetenv(const char *name);
long strtol(const char *nptr, char **endptr, int base);

void __IVM64_do_global_ctors__();
void __IVM64_do_global_dtors__();

static jmp_buf jenv;

static char* __IVM64_env_pointer__ = NULL;
extern char **environ; // Use the libc environment variable

// Start here if entry point option "-e _start" is set
__attribute__((no_reorder))
long _start(void *args_start, unsigned long args_length, void *heap_start, ...) // Header for ivm v0.25
{
    char* __args_start = 0;
    char* __heap_start = 0;

    volatile unsigned long *ret = (volatile unsigned long*)&heap_start + 1;
    __IVM64_exit_jb__ = &jenv;

    // If entry point, get data from the arguments (stack)
    __heap_start = heap_start;
    __args_start = args_start;
    if (! heap_start){ // Is there entry point (-e)?
        // If no entry point, get data from the positions preceding the program
        __heap_start = (char *)*(unsigned long*)((unsigned long)&__start-8);

        __args_start = (char *)*(unsigned long*)((unsigned long)&__start-16);
        args_length = *((unsigned long*)__args_start);
        __args_start = __args_start + sizeof(long); // Skip argument file length
    }

    // Make a copy of the environment area after the space area
    // Note that this changes the heap pointer, which will be place
    // just after this copy. Do this before extracting argument,
    // as extracting arguments writes the heap.
    void *env_start0 = (void *)((char *)__args_start + args_length);
    unsigned long env_length = *(unsigned long *)(env_start0);
    __IVM64_env_pointer__ = NULL;
    __heap_start += env_length + 8; // Space occupied by the environment
    if (env_length) {
        // Move the environment file to the starting of the heap to
        // leafe the space area as it should be
        // Heap pointer must be suitably increased
        memcpy(__heap_start, env_start0, env_length + 8 );
        memset(env_start0, 0, env_length + 8);
        __IVM64_env_pointer__ = __heap_start + 8;
        __heap_start += env_length + 8; // Space occupied by the environment
                                        // is not currently counted by the initialization!!!
    }

    // The argument file must follows the same format
    // that /proc/<pid>/cmdline file in Linux, that is,
    // a string with '\0'-separated arguments.
    // For example:
    // "arg0\0arg1\0arg2\0arg3\0"
    //
    // Extracting arguments
    char **argv= (char**)__heap_start;
    unsigned long argc = 0;

    if (args_length > 1){
        int newarg = 0;
        argc = 1;
        argv[0] = (char *)__args_start;
        for (unsigned long i=0; i<args_length; i++){
            if (newarg) {
                argv[argc++] = (char*)__args_start + i;
                newarg = 0;
            }
            newarg = IS_END(*(__args_start+i));
            if (newarg) {
                *((char*)__args_start+i) = '\0';
            }
        }
    }
    argv[argc] = NULL;

    // Allocate space for array argv[argc+1]
    __heap_start += (argc+8)*sizeof(char*);

    // The environment file must follows the same format
    // that /proc/<pid>/environ file in Linux, that is,
    // a string with '\0'-separated pairs "var=val".
    // For example:
    // "var1=val1\0var2=val2\0var3=val3\0"
    //
    // Creating a environ structure  "char *environ[]
    char *env_start = __IVM64_env_pointer__;
    environ = (char**)__heap_start;
    unsigned long envc = 0;

    if (env_length > 1){
        int newenv = 0;
        envc = 1;
        environ[0] = (char *)env_start;
        for (unsigned long i=0; i<env_length; i++){
            if (newenv) {
                environ[envc++] = (char *)env_start + i;
                newenv = 0;
            }
            newenv = IS_END(*((char*)env_start+i));
            if (newenv) {
                *((char*)env_start+i) = '\0';
            }
        }
    }
    environ[envc] = NULL;

    // Allocate space for array environ[envc+1]
    __heap_start += (envc+8)*sizeof(char*);

    // Update the global heap start pointer
    __IVM64_heap_pointer__ = __heap_start; // from this point malloc() can be used


    // spawn program will find these environment variables
    jmp_buf *spawn_jb_p = NULL;
    char *jb_str = getenv("IVM_SPAWN_RETURN_JB");
    if (jb_str) {
        spawn_jb_p = (jmp_buf*)strtol(jb_str, NULL, 16);
        unsetenv("IVM_SPAWN_RETURN_JB");
    }

    int *spawn_val_p = NULL;
    char *val_str = getenv("IVM_SPAWN_RETURN_VAL");
    if (val_str) {
        spawn_val_p = (int*)strtol(val_str, NULL, 16);
        unsetenv("IVM_SPAWN_RETURN_VAL");
    }

    // Call main
    if (__builtin_setjmp(jenv) == 0) {
        // Call constructors before main
        __IVM64_do_global_ctors__();
        // Call main
        exit(main(argc, argv, NULL));
    }

    // As destructors can also call exit(), use setjmp again
    if (__builtin_setjmp(jenv) == 0) {
        // Call destructors
        __IVM64_do_global_dtors__();
    }

    // Return for spawned programs (to the caller)
    // If environment variables IVM_SPAWN_RETURN_JB and IVM_SPAWN_RETURN_VAL are defined
    // this function will return to the caller program using the longjump buffer
    // instead of returning normally.
    if (spawn_jb_p && spawn_val_p) {
        *spawn_val_p = __IVM64_exit_val__;
        __builtin_longjmp(*spawn_jb_p, 1);
    }

    // Normal exit
    volatile unsigned long reti = (volatile unsigned long)&heap_start;
    *(volatile unsigned long*)reti = __IVM64_exit_val__;

    return __IVM64_exit_val__;
}

__attribute__((noinline, no_reorder))
void __IVM64_do_global_ctors__()
{
    asm volatile("push"); // nop
}

__attribute__((noinline, no_reorder))
void __IVM64_do_global_dtors__()
{
    asm volatile("push"); // nop
}
