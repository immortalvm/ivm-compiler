/*
 * Preservation Virtual Machine Project
 * Startup code for GNU compiler for ivm64
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: May 2020
 *
 */

/*
 A multipurpose crt0.c for the ivm64 architecture that parses the
 argument file and converts it into an argv[] structure which is
 passed to main(). It is valid with and without entry point.

 The separator of different arguments is the null char (\0)
 to be compatible with the format of /proc/<pid>/cmdline
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

int main(int argc, char* argv[], char* env[]);
void exit(int status);

static jmp_buf env;

// Start here if entry point option "-e _start" is set
long _start(void *args_start, unsigned long args_length, void *heap_start, ...) // Header for ivm v0.25
{
    char* __args_start = 0;
    char* __heap_start = 0;

    volatile unsigned long *ret = (volatile unsigned long*)&heap_start;
    __IVM64_exit_jb__ = &env;

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

    // Extracting arguments
    char **argv= (char**)__heap_start;
    unsigned long argc = 0;

    if (args_length > 1){
        int newarg = 0;
        argc = 1;
        argv[0] = __args_start;
        for (unsigned long i=0; i<args_length; i++){
            if (newarg) {
                argv[argc++] = __args_start + i;
                newarg = 0;
            }
            newarg = IS_END(*(__args_start+i));
        }
    }
    argv[argc] = NULL;

    // Allocate space for argv[argc+1]
    __heap_start += (argc+8)*sizeof(char*);

    // Global heap start pointer
    __IVM64_heap_pointer__ = __heap_start;

    // Call main
    if (__builtin_setjmp(env) == 0) {
        exit(main(argc, argv, NULL));
    }

    *ret = __IVM64_exit_val__;
    return __IVM64_exit_val__;
}

