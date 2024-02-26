#ifdef __cplusplus
extern "C" {
#endif
/*
   alloca -- (mostly) portable public-domain implementation -- D A Gwyn

   last edit: April 2020 - Feb 2024
   Preservation Virtual Machine Project
   Adapted for the ivm64 gcc compiler
   by E. Gutierrez, S. Romero & O. Plata,
   University of Malaga

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

#undef  NULL
#define NULL ((void*)0)

typedef unsigned long size_t;
typedef void* pointer;    /* generic pointer type */
extern void free(pointer ptr);
extern pointer malloc(size_t size);
extern void exit(int status);
extern int puts(const char *s);

// ivm64: The extra offset (subdepth) is introduced to emulate
// the behaviour of __builtin_stack_save() and __builtin_stack_restore()
// that may surround basic blocks with VLA declaration like loops

// There may be an "extra_offset" per function calling alloca-related
// functions, so we index this extra offset with the value of the meassured
// SP when alloca_function() is called; that is, the index is the depth
// computed from the probe inside alloca_function()
typedef struct extra_offset_s {
    char *sp;
    long extra_offset;
    struct extra_offset_s *next;
} extra_offset_t;

// We define a dictionary which maps SP (probe depth in alloca_function())
// to the associated extra offset at this point:
//    eo_dict[ sp=probe depth in alloca_funcion() ] = extra_offset at this point
static extra_offset_t eo_dict_first = (extra_offset_t){0,0,NULL};
static extra_offset_t *eo_dict = &eo_dict_first;

__attribute__((noinline))
static extra_offset_t* extra_offset_find(char *sp)
{
    extra_offset_t *p = eo_dict;
    extra_offset_t *ret = NULL;
    while(p){
        if (p->sp == sp){
            ret = p;
            break;
        } else{
            p = p->next;
        }
    }
    return ret;
}

__attribute__((noinline))
static long extra_offset_get(char *sp)
{
    extra_offset_t* p = extra_offset_find(sp);
    if(p)
        return p->extra_offset;
    else
        return 0;
}

__attribute__((noinline))
static void extra_offset_insert(char *sp, long value)
{
    extra_offset_t *p = eo_dict;
    if (p) {
        extra_offset_t *q = (extra_offset_t*)malloc(sizeof(extra_offset_t));
        if (q) {
            q->sp = sp;
            q->extra_offset = value;
            q->next = p->next;
            p->next = q;
        }
    }
}

__attribute__((noinline))
static void extra_offset_del(extra_offset_t* n){
    if (!n || !eo_dict) return;
    extra_offset_t *prev = eo_dict, *p=prev->next;
    while(p){
        if (p == n){
            prev->next = p->next;
            free(p);
            p=prev->next;
        } else{
            p = p->next;
        }
    }
}

__attribute__((noinline))
static extra_offset_t* extra_offset_inc(char *sp)
{
    extra_offset_t* p = extra_offset_find(sp);
    if(p) {
        p->extra_offset++;
    } else {
        // If not found its extra offset is 0
        // So, on incrementing we must insert with 1
        extra_offset_insert(sp, 1);
    }
    return p;
}

__attribute__((noinline))
static extra_offset_t* extra_offset_dec(char *sp)
{
    extra_offset_t* p = extra_offset_find(sp);
    if( p != NULL ) {
        if (p->extra_offset == 1) {
            extra_offset_del(p);
            p = NULL;
        } else {
            p->extra_offset--;
        }
    }
    return p;
}

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
#define STACK_DIRECTION  0  /* direction unknown */
#endif

#if STACK_DIRECTION != 0
#define    STACK_DIR    STACK_DIRECTION        /* known at compile-time */
#else  /* STACK_DIRECTION == 0; need run-time code */
static int  stack_dir;    /* 1 or -1 once known */
#define    STACK_DIR    stack_dir
static void find_stack_direction ()
{
  static char *addr = NULL;    /* address of first
                                  `dummy', once known */
  char dummy;                  /* to get stack address */

  if (addr == NULL) {  /* initial entry */
       addr = &dummy;
       find_stack_direction ();  /* recurse once */
  } else {
       /* second entry */
       if (&dummy > addr)
           stack_dir = 1;      /* stack grew upward */
       else
           stack_dir = -1;     /* stack grew downward */
  }
}
#endif /* STACK_DIRECTION == 0 */

/*
   An "alloca header" is used to:
   (a) chain together all alloca()ed blocks;
   (b) keep track of stack depth.

   It is very important that sizeof(header) agree with malloc()
   alignment chunk size.  The following default should work okay.
*/
#ifndef    ALIGN_SIZE
#define    ALIGN_SIZE    sizeof(double)
#endif

typedef union hdr
{
   char  align[ALIGN_SIZE]; /* to force sizeof(header) */
   struct
   {
        union hdr *next;    /* for chaining headers */
        char      *deep;    /* for stack depth measure */
        long      subdepth; /* ivm64: used to differenciate vla blocks with the same depth*/
   }
   h;
} header;

/*
   alloca( size ) returns a pointer to at least `size' bytes of
   storage which will be automatically reclaimed upon exit from
   the procedure that called alloca().  Originally, this space
   was supposed to be taken from the current stack frame of the
   caller, but that method cannot be made to work for some
   implementations of C, for example under Gould's UTX/32.
 */
static header  *last_alloca_header = NULL;    /* -> last alloca header */

__attribute__((noinline))
__attribute__((optimize("O0")))
static pointer alloca_function (unsigned long size, unsigned long probe_offset, int return_probe_depth)
{
    char           probe;    /* probes stack depth: */
    volatile char* depth = &probe;

#if STACK_DIRECTION == 0
    if (STACK_DIR == 0)        /* unknown growth direction */
        find_stack_direction ();
#endif

    /*ivm64: adjust depth as it is called from a wrapper*/
    if (STACK_DIR < 0) {
        depth = depth + probe_offset;
    } else {
        depth = depth - probe_offset;
    }

    if (return_probe_depth){
        // Return only depth if return_probe_depth=true
        return (char*) depth;
    }


    // The subdepth is the extra offset for the function calling
    // alloca(), characterized by its depth
    long subdepth = extra_offset_get((char *)depth);

    /* Reclaim garbage, defined as all alloca()ed storage that
       was allocated from deeper in the stack than currently. */
    {
        header  *hp;    /* traverses linked list */

        for (hp = last_alloca_header; hp != NULL;) {
            int eqdeep = (hp->h.deep == depth) && (hp->h.subdepth > subdepth);

            if ((STACK_DIR > 0 && (hp->h.deep > depth || eqdeep))
                 || (STACK_DIR < 0 && (hp->h.deep < depth || eqdeep)))
            {
                header    *np = hp->h.next;
                free ((pointer) hp); /* collect garbage */
                hp = np; /* -> next header */
            } else {
                break;        /* rest are not deeper */
            }
        }
        last_alloca_header = hp;    /* -> last valid storage */
    }

    if (size == 0) {
        return NULL;        /* no allocation required */
    }

    /* Allocate combined header + user data storage. */
    {
        pointer  pNew = (pointer) malloc (sizeof (header) + size);
        if (!pNew) {
            /* ivm64: the alloca() function returns a pointer to the beginning of
               the allocated space. If the allocation  causes  stack  overflow,
               program behavior is undefined.  We will exit in these cases */
            puts("[alloca] alloca() failed; maybe too many allocated bytes");
            exit(255);
        }

        /* address of header */
        ((header *) pNew)->h.next = last_alloca_header;
        ((header *) pNew)->h.deep = (char *)depth;
        ((header *) pNew)->h.subdepth = subdepth;

        last_alloca_header = (header *) pNew;

        /* User storage begins just after header. */
        return (pointer) (pNew + sizeof (header));
    }
}


__attribute__((noinline))
__attribute__((optimize("O0")))
static long fun_activation_block_size(char* fun_last_arg){
    return ((long)fun_last_arg - (long)&fun_last_arg);
}

/*ivm64: All alloca-like functions used by the compiler
  (alloca, __builtin_alloca*, ...)
  must use the corresponding offset in order to have
  the same depth when called from a function. Basically
  this adjustment is the size of the activation record*/

/* Note that gcc replace "__builtin_alloca" by "alloca"
   so, only alloca needs to be defined */
__attribute__((noinline))
__attribute__((optimize("O0")))
void *alloca(size_t size)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&size);
    static void* ret; ret = alloca_function(size, ab, 0);
    return ret;
}

__attribute__((noinline))
__attribute__((optimize("O0")))
void* __builtin_alloca_with_align(size_t size, size_t alignment)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&alignment);
    static void* ret; ret = alloca_function(size, ab, 0);
    return ret;
}


__attribute__((noinline))
__attribute__((optimize("O0")))
void __ivm64_alloca_enter(void* dummy)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&dummy);
    static char *depth ; depth = alloca_function(0, ab, 1);
    extra_offset_t* e = extra_offset_find(depth);
    if (e) extra_offset_del(e);
    return;
}

__attribute__((noinline))
__attribute__((optimize("O0")))
void __ivm64_alloca_exit(void* dummy)
{
    /*Nothing to do yet*/
}

/* Call this function before return in order to free
   memory allocated by alloca() for the current function
   Observe that the offset is 1 unit larger than the
   activation block size to force garbage collection*/
__attribute__((noinline))
__attribute__((optimize("O0")))
void __ivm64_alloca_free(void* dummy)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&dummy);
    alloca_function(0, ab+1, 0);
}

/* This is a replacement of __builtin_stack_save*/
__attribute__((noinline))
__attribute__((optimize("O0")))
void *__builtin_stack_save_ivm64(long dummy)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&dummy);
    static char *probe; probe= alloca_function(0, 0, 1);
    static char *depth; depth = probe + ab;
    extra_offset_inc(depth);
    return NULL;
}

/* This is a replacement of __builtin_stack_restore*/
__attribute__((noinline))
__attribute__((optimize("O0")))
void __builtin_stack_restore_ivm64(void *p)
{
    volatile static long ab; ab = fun_activation_block_size((char*)&p);
    static char *probe; probe= alloca_function(0, 0, 1);
    static char *depth; depth = probe + ab;
    extra_offset_dec(depth);
    return;
}

#endif
#ifdef __cplusplus
}
#endif

/*
*/
