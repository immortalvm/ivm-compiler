# IVM64 GCC: The iVM Compiler

## Index

1. [Introduction](#intro)
1. [Requirements](#requirements)
1. [Comparison between versions](#comparison)
1. [Quick start](#quick)
1. [Compiler Driver](#bin)
1. [Extra feature: DCE](#dce)
1. [Compiling and Linking](#complink)
1. [Libraries](#libs)
1. [Testing compiler features](#testing)
1. [ABI](#abi)
1. [Compiler Explorer Tool](#cedemo)

## Introduction <a name="intro">

This project presents the current released version of the IVM64 GCC compiler and toolchain (```as``` and ```ld```).

The current version, ivm64-1.0rc4, is based on GNU CC v8.3.0.

The **main features** of the IVM64 GCC compiler are:

- Support for signed and unsigned integer data types: char (8bits), short (16bits), int (32bits), long (64bits)

- Native integer arithmetic through IVM64 arithmetic (pseudo-)instructions

- Support for floating data types: float (32bits) and double (64bits)

- Floating point arithmetic through calls to the _soft-float_ library

- Jump, branch, calls and indirect calls (pointers to functions) are currently implemented, so there is full support of all C control structures ('if-then', 'for', 'while', ...)

- ABI allows simple data types and structures, both for arguments and return value, with support for variadic functions.

- Supported asm inlined in C

- Complete C standard library (libc, libm) from newlib

- Startup file ```crt0``` included

**New features included in the 1.0rc4** version are:

- Resulting assembly performs better than previous versions; the number of ivm executed instructions is reduced around a 40% with -O2 

- Functions setjump/longjump supported in libc

- Support of primitive alloca() and VLAs (variable length array) via library functions in libgcc

- By default, the elimination of not used library objects (DCE) in enabled in the linker, so it is not necessary enabling it with ```-Xlinker -mdce```

- Improved startup script crt0; a minimal stack is kept after the program exits

- Command line arguments can be used when an autoexecutable output is run, e.g., you could invoke ```./a.out arg1 arg2``` (```/proc``` filesystem required)


**New features included in the 1.0rc5** version are:

- Resulting assembly performs better than previous versions; the number of ivm executed instructions is reduced around a factor of x1.6 with respect version 1.0rc4 using -O2

- Complex number aritmetic support for complex of 64 and 128 bits, both integer and float complex

- Support for type ```__int128``` as integer of 128 bits (note that ```long long``` is still 64 bit wide)

- Built-in functions ```__builtin_setjmp``` and ```__builtin_longjmp```  supported

- Unified ABI for variadic and non-variadic functions based on _caller pops arguments_ convention (see ABI section).

**New features included in the 1.1rc1** version are:

- Resulting assembly performs better than previous versions; the number of ivm executed instructions is reduced around a factor of x2 with respect version 1.0rc5 using -O2

**New features included in the 1.2rc1** version are:

- This version includes the releasing of the source codes of the compiler
- Optimization option ```-O2``` enabled by default
- Resulting assembly performs slightly better than the previous version

Additionaly this project includes a bunch of C codes to test the compiler.

Not supported features:
Particular intrinsics that requires frame pointer, such as ```__builtin_frame_address``` (note that alloca() and VLAs are supported via library functions);

For more information about the toolchain see the slides in the folder ```docs```.

## Requirements <a name="requirements">

The provided linking system is based on shellscripts.
At least bash version 4.4.20 is required.

The following common UNIX utilities are required: ```bash, ar, gawk, sed, grep, egrep,    file, which, find, diffutils (cmp), coreutils (basename, dirname, md5sum, cut, cat, true, readlink, csplit)```



## Comparison between versions <a name="comparison">

Here you can find some figures comparing different releases.
Executions have been performed on a Intel Core i7-3770 @3.40GHz server.

The first table correspond to example 13-math.c, a small code invoking mathematical functions from libm.

|                      | #executed<br>instructions | asm size | binary size | "ivm as" time<br>(only assembling) | ivm\_emu\_fast time<br>(simulation time) |
| -------------------- | ------------------------- | -------- | ----------- | ---------------------------------- | ---------------------------------------- |
| **13-math.c** (-O2 -lm)  |                           |          |             |                                    |                                          |
| ivm64-gcc 1.0rc3     | 3.7 millions             | 1.6MB    | 578KB       | 2.8s                               | 0.021s                                   |
| ivm64-gcc 1.0rc4     | 1.9 millions             | 1.2MB    | 432KB       | 2s                                 | 0.012s                                   |
 ivm64-gcc 1.0rc5     | 1.3 millions             | 859KB    | 301KB       | 2s                                 | 0.007s                                   |
 ivm64-gcc 1.1rc1     | 0.6 millions             | 717KB    | 227KB       | 1.6s                                 | 0.007s                                   |
 | speedup  <br> (1.1rc1 vs 1.0rc5)            | x**2.2**                      |          |             |                                    |                                          |


This table shows results for the "static-unboxer" program. It executes x1000 million instructions. Note that this code includes some embedded data, so the reduction in size is less noticeable.

|                      | #executed<br>instructions | asm size | binary size | "ivm as" time<br>(only assembling) | ivm\_emu\_fast time<br>(simulation time) |
| -------------------- | ------------------------- | -------- | ----------- | ---------------------------------- | ---------------------------------------- |
| **static-unboxer** (-O2) |                           |          |             |                                    |                                          |
| ivm64-gcc 1.0rc3     | 111477 millions           | 12MB     | 2.6MB       | 14.2s                              | 2m35s                                    |
| ivm64-gcc 1.0rc4     | 63138 millions            | 11MB     | 2.2MB       | 12.8s                              | 1m25s                                    |
| ivm64-gcc 1.0rc5     | 39481 millions            | 9MB      | 1.8MB       | 12.0s                              | 0m57s                                    |
| ivm64-gcc 1.1rc1     | 18701 millions            | 8.8MB    | 1.6MB       | 10.8s                              | 0m38s                                    |
| speedup <br> (1.1rc1 vs 1.0rc5)                  | x**2.1**               |          |             |                                    |                                          

Note: Version 1.1rc1, tested with ```ivm v0.32```

|                      | #executed<br>instructions | asm size | binary size | "ivm as" time<br>(only assembling) | ivm\_emu\_fast time<br>(simulation time) |
| -------------------- | ------------------------- | -------- | ----------- | ---------------------------------- | ---------------------------------------- |
| **static-unboxer** (-O2) |                           |          |             |                                    |                                          |                             |
| ivm64-gcc 1.2rc1     | 17665 millions    | 8.7MB    | 1.5MB       | 10.8s                              | ~0m40s                                    |

Note: Version 1.2rc1, tested with ```ivm v0.37``` (be aware that ISA changed in this version)

## Quick start <a name="quick">

To compile and test the compiler, do the following:

* Clone this project

* Go to the project folder

  ```
  cd /path/to/project
  ```

* Add the folder cointaining the compiler driver 'ivm64-gcc' to the path

  ```
  export PATH=$(readlink -f $(dirname $(find . -iname "ivm64-gcc"))):${PATH}
  ```

* Add the folder containing the ivm linux64 executable (assembler) to the path (in order to simulate asm codes):

  ```
  export PATH=/path/to/ivm/bin:${PATH}
  ```

* Go to the example folder:

  ```
  cd ./tests/examples
  ```

* Compile one code, for example ```04-asm-io-puts.c``` which prints a string:

  ```bash
  ivm64-gcc 04-asm-io-puts.c  # a.out generated
  ```

* Execute the resulting file:

  ```bash
  ./a.out  # you can execute directly
  echo $?  # print the return value
  ```

* Observe the execution output:

  ```bash
  $ ./a.out
  iVM Assembler and VM, v0.21
  Opening a.out...
  Opening a.out...
  Output ignored

  Execution steps: 20787

  End stack:
  0x..0007E     126
  0x..00000       0
  0x..00010      16
  0x..01B30 16784176
  0x..00000       0
  0x..BB9F4  768500
  0x..0228C    8844
  0x..0007E     126
  0x..0007E     126
  0x..0007E -4294967170
  0x..01B48 541182663496

  History Begins at Sumer

  $ echo $?
  126
  ```

  Notice that:

  - The return value (126) is always kept on the top of the stack, 
    when the ivm simulation ends.
  - Many other values remain in the stack above the return value, 
    due to the _crt0_ startup code that calls the standard function 
    ```exit()``` (see [Libraries](#libs)).
    Once ```exit()``` finishes its work (standard functionality), 
    it invokes ```_exit()```. This causes the machine instruction ```exit``` 
    to be executed, leaving the stack as it was at that moment.

* Argument can be passed to the generated executable (/proc filesystem required)

  ```
  $ ivm64-gcc 18-main_args.c
  
  $ export IVM_EMU=/path/to/ivm_emu_fast
    ./a.out arg1 arg2 2> /dev/null

  ./a.out found 3 arguments
    argv[0]=./a.out
    argv[1]=arg1
    argv[2]=arg2
    argv[3] is null
  ```


### Some suggestions

* When invoking the "executable" (e.g. ```a.out```), we can define the emulator to be used and the memory size, through the environment variables IVM_EMU and IVM_MAXMEM:
   - If defined IVM_EMU to be a valid simulator, it is used as simulator, otherwise it simulates with 'ivm run'
   - If defined IVM_MAXMEM, it is used as memory size, otherwise it uses 64MB

  For example: 
  ```bash
  export IVM_MAXMEM=2000000000
  export IVM_EMU=/path/to/ivm_emu_fast 
  ./a.out
  ```
  
  or in one line:
  
  ```bash
  IVM_MAXMEM=2000000000 IVM_EMU=/path/to/ivm_emu_fast ./a.out
  ```

## Compiler Driver <a name="bin">

The compiler driver IVM64 GCC is provided for Linux. The current compiler driver has been compiled and tested in a "Ubuntu 18.04.3 LTS" distribution with linux kernel 4.15.0-72-generic. It is located in:
```
./gcc-8.3.0-ivm64-1.0rc/bin/ivm64-gcc
```

The driver IVM64 GCC calls to the compiler itself, **cpp/cc1** (C preprocessor and compiler), and the scripts **as** (assembler) and **ld** (linker). These programs are called according to the following toolchain:

- First, 'cpp' translates the input C source file (i.e., *prog.c*) into and ASCII intermediate file (*prog.i*), and, next, 'cc1' translates the intermediate file into and ASCII assembly language file (*prog.s*).

- Second, the assembler 'as' translates the assembly file (*prog.s*) into a relocatable object file (*prog.o*). In our case, the object file is basically the same as the assembly file.

- Third, the linker 'ld' combines one or several object files (inlcuding libraries) into a single executable file. With this purpose, all local labels of each object are renamed in case they are reused in different files.

- The executable file is just a concatenation of:
  - the *crt0.o* startup file, which is placed at the very beginning,
  - all object files (which are really assembly files), and
  - the standard C libraries.

## Extra feature: DCE <a name="dce">

The current linker is able to perform Dead Code Elimination (DCE) of those library objects not referenced by the main program files. This feature allows the linker to find the objects, and only these objects, on which the codes being compiled depend, and incorporate them to the assembly output (instead of the full libraries).

This feature is enable by default in version 1.0rc4.

To enable/disable this feature, add the flags "```-Xlinker -mdce```"  or" ```-Xlinker -mno-dce```", respectively, to the gcc command line invokation. For example:

```bash
ivm64-gcc -Xlinker -mdce 00-main.c  # This generates a.out 

```

You can observe that the ```a.out``` output file is much smaller than the resulting file when the DCE feature is not enabled. Here are some examples:

```bash
ivm > ### NO DCE
ivm > time ivm64-gcc -Xlinder -mno-dce 00-main.c

real    0m0,393s
user    0m0,285s
sys     0m0,117s

ivm > ls -lh a.out
-rwxr-xr-x 1 user group 6,3M mar  5 14:04 a.out

ivm > time ./a.out >& /dev/null

real    0m9,849s
user    0m9,866s
sys     0m0,390s


ivm > ### DCE ENABLED BY DEFAULT
ivm > time ivm64-gcc 00-main.c

real    0m0,799s
user    0m0,858s
sys     0m0,273s

ivm > ls -lh a.out
-rwxr-xr-x 1 user group 141K mar  5 14:04 a.out

ivm > time ./a.out >& /dev/null

real    0m0,958s
user    0m0,941s
sys     0m0,047s


```

Notice that the DCE feature can reduce the size of the output at the expense of increasing the compilation time. It is also remarkable the reduction of the time spent by ivm simulator when processing the file.



## Compiling and Linking <a name="complink">

This project contains an wide set of sample C source files, located in:
```
./tests/examples
./tests/tcc
```

Before continuing examples in this section, follow the steps in [Quick start](#quick) to prepare the working environment.

To compile and test the examples, do the following:

* Go to the example folder:
```
cd ./tests/examples
```

* Compile the example code you wish. Some ways to invoke the compiler driver are detailed below:

  * Generating the assembly code file:

    ```bash
    ivm64-gcc -S 00-main.c  # 00-main.s generated
    ```

  * Generating an assembly "object" file:

    ```bash
    ivm64-gcc -c 00-main.c  # 00-main.o generated (=asm)
    ```

  * Compiling and linking several C source codes:

    ```
    ivm64-gcc 16-import.c 16-export.c -o a.out
    # Note that a.out is an assembly code, but executable at the same time
    ```

  * Creating a library and linking with it:

    ```
    ivm64-gcc -c 16-export.c
    ar cr libmy.a 16-export.o
    ivm64-gcc 16-import.c -L. -lmy -o a.out
    ```

  * Executing the resulting assembly code:

    You can execute directly the resulting "executable assembly" if the emulator is included in the shell PATH environment variable. In this case, the file behaves like an executable file: the return value of the C program is passed to the shell, and the standard output of the C program are printed to the stdout:

    ```bash
    $ ivm64-gcc 10-strings.c
    $ ./a.out
    iVM Assembler and VM, version 0.21
    Opening a.out
    Opening a.out
    Output ignored

    End stack:
    0x..0003D      61
    0x..00000       0
    .
    .
    .

    Hello world!
     Again, Hello world!
      strlen("Hello world!")=12
        No. of chars printed so far: 61(=0x3d)
    $
    $ echo $? # Observe the return value passed to the shell
    61
    ```

    _Note_: The assembly executable generated by the linker always includes the ```--noopt``` flag, so as to avoid aggresive ivm implementation optimizations.

  * Also, you can execute the assembly code by invoking the ivm simulator (in this case the return value of the program is not available, and the output printed by the C program is sent to the stderr)

    ```
    ivm as-run --noopt a.out
    ```

Finally, you have a complete description of the usage of both tools, 'as' and 'ld' in the files themselves.


```bash
# Expected syntax:
#
#    ld [normal link options (ignored)] [-o outfile] [-L libdir] [-lname] [-e entry] objfiles
#
# There can be several '-L' options if more than one
# library directories are to be included.
# If no '-o' option output file will be 'a.out'.
# If the output file has no extension '.S' nor '.o', an
# "executable assembly" is created prepending a shebang preamble
# in charge of assembling and emulating the code by 
# calling the emulator "ivm"

# Other options:
#
# -mdce / -mno-dce
#      Enable/disable DEAD CODE ELIMINATION at library object level
#      It is enabled by default
#

# File extension convention:
#   .s -> assembly (from c source)
#   .S -> assembly (precompiled libraries or modules, startup files,...)
#   .o -> object (actually containing assembly)
#   .a -> ar file containing objects

# Examples:
#   - Generate several assembly files from c sources:
#       ivm64-gcc -S file1.c file2.c ...
#
#   - Compile generating objects from c sources:
#       ivm64-gcc -c fle1.c file2.c ...
#
#   - Link several c sources and/or objects in one unique "executable" assembly:
#       ivm64-gcc main.c mod1.c mod2.c obj1.o obj2.o ... -o a.out.s
#
#   - Precompile a library including objects from several c sources:
#       ivm64-gcc lib-fun1.c lib-fun2.c -o lib.S
#
#   - Build a library by compiling the objects and then archiving them:
#       ivm64-gcc -c lib-fun1.c lib-fun2.c ... # Generate all .o
#       ar r lib.a lib-fun1.o lib-fun2.o ...
#
#   - Link c sources with precompiled assembly libraries:
#       ivm64-gcc main.c lib1.S lib2.S -o a.out.s
#
#   - Compile c sources and link with ar/obj/assembly files:
#       ivm64-gcc main.c lib1.a lib2.S mod3.o -o a.out.s
#
#   - Both -L/-l flags can be used; for example, linking with libmy.a in /path/to:
#       ivm64-gcc main.c -L/path/to/ -lmy -o a.out.s

# The resulting "executable" can be executed thanks to a 
# shebang preamble. To do this, the ivm implementation (ivm) is required to be
# in PATH. Also an alternative simulator (IVM_EMU), and a memory limit (IVM_MAXMEM)
# can be set.
# Examples:
#   you can run this file ('ivm run' used as default emulator):
#      ./a.out 
#   using your favourite ivm emulator: 
#      IVM_EMU=ivm_emu_fast ./a.out 
#   passing arguments: 
#      IVM_EMU=ivm_emu_fast ./a.out 1 2 3 
#   setting the memory size: 
#      IVM_MAXMEM=100000000 IVM_EMU=ivm_emu_fast ./a.out 

```


## Libraries <a name="libs">

The current distribution includes several precompiled libraries. These are:

- _libgcc.a_: the **soft float** library from GCC
- _libc.a_ and _libm.a_: the standard and mathematical C libraries from **newlib**
- _crt0.o_: the C runtime startup file, a simplistic one could be:

  ```c
    extern void exit(int code);
    extern int main(int argc, char **argv);
    void _start() {
        int ex = main(0,0);
        exit(ex);
    }
   ```

By default, libraries _libgcc_ and _libc_ are linked to the executable by the driver; thus, the two next invocations are equivalent:

``` 
    ivm64-gcc 00-main.c
    ivm64-gcc 00-main.c -lc -lgcc # equivalent to the command above
```

As usual, the math library must be explicitly set for those programs using mathematical functions (sin, cos, pow, ...):

``` 
    ivm64-gcc 13-math.c -lm
```

If a program does not need any of these standard libraries, you can avoid linking them by using the ```-nostdlib``` flag, but as no crt0 file is added you must provide the entry point:

```
    ivm64-gcc 04-asm-io-puts.c -nostdlib -e main   # no libc, no libgcc, no crt0
```

## Testing compiler features <a name="testing">

A testsuite coming from [tcc compiler project](https://bellard.org/tcc/) is provided in this project. It is placed in the directory ```./tests/tcc```.

The requierements for this test are the 'make' tool and the 'gcc' compiler for the host (in order to generate the reference output files to compare).

To run the test, just type:

```bash
make very-clean
make -k check
```

This command will compile each C source code with both the 'gcc' for the host and the ivm64-gcc compilers. The host executable is run to provide the reference output for such program (with the .expect suffix). The ivm64 executable is run (through _ivm as-run_) and its output is stored with the '.sdout' suffix. Finally, both outputs are compared, when they match a '.test' file is generated, otherwise an '.error' file produced.

From the original set, those C codes requiring features not supported by the abstract machine (e.g. frame pointer register, file operations,...) are excluded from compiling. In the other hand, those tests whose output depend on the data alignment are not compared as they always differ from the host version, but they can run correctly if invoked by hand.

## ABI <a name="abi"> (call convention)
This section describes in short the Application Binary Interface (ABI) related to the calling convention used by ivm64-gcc. Assembly codes aspiring to interoperate with gcc-generated functions must satisfy these conventions.

The call convention has changed from release 1.0rc4 to 1.0rc5.

_Note:_ This ABI can be subject to change due to future optimizations.

A unique ABI is used for all functions regardless the number of arguments (no arguments/fixed number of arguments/variable number of arguments). This call convention is based on the "caller pops arguments" rule, so that the **caller** is in charge of releasing the arguments and move the return value to its destination:

  - Caller may allocate one stack slot for the return value
  - Caller passes all arguments on the stack
  - Caller invokes the function (call instruction)
  - After the function returns, the return value has been placed in the stack position immediately above the return address (note that the return address has been just popped, so the return value is on the top of the stack)
  - At this point, the **caller copies the return value and releases arguments**, if necessary
  - Arguments are passed from right to left in the order of the C declaration
  
  
 Note that the callee stores the return value in the stack position immediately above the return address, overwritting the first C argument (or the corresponding stack slot if there are no arguments). 
 
 Next figure shows the stack layout when calling a function (stack grows downwards):
 
| just after the call |   just after the return |
|-----|----|
| arg N | arg N |
| ...   | ... | 
| arg 2 | arg 2 |
| arg 1 | **return value**| 
| **return address** |


 Here are some examples of how to call a C function from assembly:

```assembly
  # calling a C function foo(a, b, c)
  # valid prototypes may be  'long foo(int a, int b, int c)',  
  #   'long foo(long a, ...)', 'long foo(long a, long b, ...)' 
  push! 0   # allocate a stack slot for return value
  load8! c  # push arguments
  load8! b
  load8! a
  call! foo
  store8! (+ &0 (* 3 8))        # caller copies the return value
  set_sp! (+ &0 (* (+ 3 -1) 8)  # caller releases the remaining arguments            
  # now the return value is on the top of stack
  ```

```assembly
  # calling a C function with prototype 'long foo()', as foo() 
  push! 0   # allocate a stack slot for return value
  call! foo
  # copying the return value is not necessary and there are no arguments to release            
  # now the return value is on the top of stack
  ```

```assembly
  # calling a C function with prototype 'long foo(long a)', as foo(a) 
  push! 0   # allocate a stack slot for return value
  load8! a  # push only one argument
  call! foo
  store8! &1         # caller copies the return value
  # there are no arguments left to release
  # now the return value is on the top of stack
  ```

```assembly
  # calling a void C function foo(a, b, c)
  # prototype 'void foo(int a, int b, int c)'
  load8! c  # push arguments
  load8! b
  load8! a
  call! foo
  # there is no return value
  set_sp! (+ &0 (* 3 8))  # caller must release all the arguments            
  ```
## Compiler Explorer Tool <a name="cedemo">

The IVM64 GCC compiler is also available for testing purposes through the web-based Compiler Explorer tool.

This tool can be accessed through the URL:  https://ivm.ac.uma.es/ce-layout.html.  
You need a user ("gcc") and a password ("ivm64").

A simplified view of the tool is also available in: https://ivm.ac.uma.es.

Some information about using the tool follows:

- The tool shows a view composed of 5 panels (see PDF file included in the 'snapshot' folder), from left to right: (1) the C source code; (2) the resulting assembly code; (3) the resulting execution in the  iVM simulator (ivm as-run); (4) the resulting iVM simulation trace (ivm as-trace); (5) the resulting execution on a x86 architecture to compare results.
 
- It is very important to select the ivm64 compiler (that is, ivm64 gcc-8.3.0, which it is by default) in the assembly panel (number (2)). In that panel you can also can select the gcc-7.4.0 (x86-64) compiler.

- The panels 'ivm as-run' and 'ivm as-trace' can be opened by clicking in the tools icon at the top of the assembly code panel (number (2)).

- There a number of C code examples, that can be selected by clicking in the disk icon at the top of the C source code panel (number (1)).

Note that as the simulation may run slow after including all the standard libraries, you may wish to close the execution panel in order to browse the compiled assembly without waiting the execution ends.
