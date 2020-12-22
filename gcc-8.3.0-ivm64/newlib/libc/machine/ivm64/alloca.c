/*
   alloca -- (mostly) portable public-domain implementation -- D A Gwyn

   last edit: April 2020
   Preservation Virtual Machine Project
   Adapted for the ivm64 gcc compiler
   by E. Gutierrez, S. Romero & O. Plata

   last edit:   86/05/30        rms
   include config.h, since on VMS it renames some symbols.
   Use xmalloc instead of malloc.

   This implementation of the PWB library alloca() function,
   which is used to allocate space off the run-time stack so
   that it is automatically reclaimed upon procedure exit,
   was inspired by discussions with J. Q. Johnson of Cornell.

   It should work under any C implementation that uses an
   actual procedure stack (as opposed to a linked list of
   frames).  There are some preprocessor constants that can
   be defined when compiling for your specific system, for
   improved efficiency; however, the defaults should be okay.

   The general concept of this implementation is to keep
   track of all alloca()-allocated blocks, and reclaim any
   that are found to be deeper in the stack than the current
   invocation.  This heuristic does not reclaim storage as
   soon as it becomes invalid, but it will do so eventually.

   As a special case, alloca(0) reclaims storage without
   allocating any.  It is a good idea to use alloca(0) in
   your main control loop, etc. to force garbage collection.
 */


#ifdef __ivm64__

//#include <stddef.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#define IVM64_ALLOCA_DEBUG_

#ifdef IVM64_ALLOCA_DEBUG
#include <stdio.h>
#endif

#undef NULL
#define NULL ((void*)0)

typedef unsigned long size_t;
typedef void* pointer;	/* generic pointer type */
extern void free(pointer ptr);
extern pointer malloc(size_t size);
extern void exit(int status);
extern int puts(const char *s);

/*ivm64: Extra offset to emulate the behaviour
  of _builtin_stack_save() and _builting_stack_restore()
  that may surround basic blocks with VLA declaration
  like loops*/
static long extra_offset = 0;

/*
   Define STACK_DIRECTION if you know the direction of stack
   growth for your system; otherwise it will be automatically
   deduced at run-time.

   STACK_DIRECTION > 0 => grows toward higher addresses
   STACK_DIRECTION < 0 => grows toward lower addresses
   STACK_DIRECTION = 0 => direction of growth unknown
 */
#define STACK_DIRECTION -1  /*ivm64: stack grows downward*/

#ifndef STACK_DIRECTION
#define	STACK_DIRECTION	0	/* direction unknown */
#endif

#if STACK_DIRECTION != 0
#define	STACK_DIR	STACK_DIRECTION		/* known at compile-time */
#else  /* STACK_DIRECTION == 0; need run-time code */
static int          stack_dir;	/* 1 or -1 once known */
#define	STACK_DIR	stack_dir
static void         find_stack_direction ()
{
  static char       *addr = NULL;	/* address of first
					   `dummy', once known */
  auto char           dummy;	/* to get stack address */

   if (addr == NULL)
     {				/* initial entry */
	addr = &dummy;

	find_stack_direction ();	/* recurse once */
     }
   else
      /* second entry */ if (&dummy > addr)
      stack_dir = 1;		/* stack grew upward */
   else
      stack_dir = -1;		/* stack grew downward */
}
#endif /* STACK_DIRECTION == 0 */

/*
   An "alloca header" is used to:
   (a) chain together all alloca()ed blocks;
   (b) keep track of stack depth.

   It is very important that sizeof(header) agree with malloc()
   alignment chunk size.  The following default should work okay.
 */
#ifndef	ALIGN_SIZE
#define	ALIGN_SIZE	sizeof(double)
#endif

typedef union hdr
{
   char                align[ALIGN_SIZE];	/* to force sizeof(header) */
   struct
     {
	union hdr          *next;	/* for chaining headers */
	char               *deep;	/* for stack depth measure */
     }
   h;
}
header;

/*
   alloca( size ) returns a pointer to at least `size' bytes of
   storage which will be automatically reclaimed upon exit from
   the procedure that called alloca().  Originally, this space
   was supposed to be taken from the current stack frame of the
   caller, but that method cannot be made to work for some
   implementations of C, for example under Gould's UTX/32.
 */
static header      *last_alloca_header = NULL;	/* -> last alloca header */

__attribute__((noinline))
__attribute__((optimize("O0")))
pointer  alloca_function (unsigned long size, unsigned long probe_offset)
{
   auto char           probe;	/* probes stack depth: */
   volatile char*      depth = &probe;


#if STACK_DIRECTION == 0
   if (STACK_DIR == 0)		/* unknown growth direction */
      find_stack_direction ();
#endif

   /*ivm64: adjust depth as it is called from a wrapper*/
   if (STACK_DIR < 0) {
       depth = depth + probe_offset - extra_offset;
   } else {
       depth = depth - probe_offset + extra_offset;
   }

#ifdef IVM64_ALLOCA_DEBUG
          fprintf(stderr, "[alloca] size=%ld probe_offset=%ld, extra_offset=%ld, depth=%p\n",
                          size, probe_offset, extra_offset, depth);
#endif


   /* Reclaim garbage, defined as all alloca()ed storage that
      was allocated from deeper in the stack than currently. */
   {
      register header    *hp;	/* traverses linked list */

      for (hp = last_alloca_header; hp != NULL;)
	 if ((STACK_DIR > 0 && hp->h.deep > depth)
	     || (STACK_DIR < 0 && hp->h.deep < depth))
	   {
	      register header    *np = hp->h.next;

#ifdef IVM64_ALLOCA_DEBUG
          fprintf(stderr, "[alloca] freeing %p\n", hp);
#endif

	      free ((pointer) hp);	/* collect garbage */
	      hp = np;		/* -> next header */
	   }
	 else
	    break;		/* rest are not deeper */
      last_alloca_header = hp;	/* -> last valid storage */
   }

   if (size == 0)
      return NULL;		/* no allocation required */

   /* Allocate combined header + user data storage. */
   {
      register pointer    pNew = (pointer) malloc (sizeof (header) + size);

#ifdef IVM64_ALLOCA_DEBUG
      if (pNew){
          fprintf(stderr, "[alloca] %d bytes at %p\n", size, pNew);
      } else {
          fprintf(stderr, "[alloca] alloca(%d) failed, returning null\n", size);
      }
#endif

      if (!pNew) {
          /* The alloca() function returns a pointer to the beginning of the allocated space.
           * If the allocation  causes  stack  overflow,  program behavior is undefined.
           * We will exit in these cases */
          puts("[alloca] alloca() failed; maybe too many allocated bytes");
          exit(255);
      }

      /* address of header */

      ((header *) pNew)->h.next = last_alloca_header;
      ((header *) pNew)->h.deep = (char *)depth;

      last_alloca_header = (header *) pNew;

      /* User storage begins just after header. */

      return (pointer) (pNew + sizeof (header));
   }
}

/*imv64: All alloca-like functions used by the compiler
  (alloca, __builtin_alloca*, ...)
  must use the corresponding offset in order to have
  the same depth when called from a function. Basically
  this adjustment is the size of extra arguments */

/* Note that gcc replace "__builtin_alloca" by "alloca"
   so, only alloca needs to be defined */
__attribute__((noinline))
__attribute__((optimize("O0")))
void *alloca (size_t size)
{
   return alloca_function(size, 1*sizeof(size_t));
}

__attribute__((noinline))
__attribute__((optimize("O0")))
void *__builtin_alloca_with_align (size_t size, size_t alignment)
{
   return alloca_function(size, 2*sizeof(size_t));
}


static void *stack_save_sp;
#define EXTRA_DEPTH_FOR_STACK_SAVE 1024

__attribute__((noinline))
__attribute__((optimize("O0")))
void *_builtin_stack_save_ivm64 (long dummy)
{
    stack_save_sp = &dummy;
#ifdef IVM64_ALLOCA_DEBUG
          fprintf(stderr, "\n[alloca] _stack_save  | sp=%p\n", stack_save_sp);
#endif
    extra_offset += EXTRA_DEPTH_FOR_STACK_SAVE;
    return NULL;
}

__attribute__((noinline))
__attribute__((optimize("O0")))
void _builtin_stack_restore_ivm64 (void *p)
{
   void *stack_restore_sp = &p;

   long inc = stack_save_sp - stack_restore_sp;
#ifdef IVM64_ALLOCA_DEBUG
          fprintf(stderr, "\n[alloca] _stack_resto  | sp=%p, inc=%ld\n", stack_restore_sp, inc);
#endif
   if (inc > 0)
        extra_offset -= inc;
   extra_offset -= EXTRA_DEPTH_FOR_STACK_SAVE;
   alloca_function(0, 0);

   extra_offset += inc;

   return;
}

#endif
