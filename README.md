# IVM64 GCC: The iVM C/C++ Compiler (and assembler)

## Index

1. [Introduction](#intro)
1. [Features](#features)
1. [Requirements](#requirements)
1. [Building from sources](#build)
1. [Comparison between versions](#comparison)
1. [Quick start](#quick)
1. [Running IVM64 programs](#environ)
1. [Compiler Driver](#bin)
1. [Assembler](#assembler)
1. [Compiling and Linking](#complink)
1. [Libraries](#libs)
1. [Testing compiler features](#testing)
1. [ABI](#abi)
1. [Extra feature: DCE](#dce)
1. [Compiler Explorer Tool](#cedemo)

## Introduction <a name="intro"></a>

This project includes the current released version of the **IVM64 C/C++** compiler (```ivm64-gcc```/```ivm64-g++``` ) and toolchain (```as``` and ```ld```).

Also it provides an assembler  (```ivm64-as```) to translate the _assembly executable_ produced by the compiler to binary.

The last version is **ivm64-3.4**, which is based on **GNU CC v12.2.0**. It is suitable for the ivm64 architecture 2.1 (ivm v2.1).

## Features <a name="features"></a>

The main features of the IVM64 GCC compiler are:

- Support for signed and unsigned integer data types: char (8bits), short (16bits), int (32bits), long (64bits)

- Native integer arithmetic through IVM64 arithmetic (pseudo-)instructions

- Support for floating data types: float (32bits) and double (64bits)

- Floating point arithmetic through calls to the _soft-float_ library

- Complex number arithmetic for complex of 64 and 128 bits, both integer and float complex

- Support for type ```__int128``` as integer of 128 bits (note that ```long long``` is still 64 bit wide)

- Supported asm inlined in C

- Support for nested functions, trampolines and nonlocal gotos

- Complete C standard library (libc, libm) from newlib

- Functions setjump/longjump supported in libc, as well as built-in functions ```__builtin_setjmp``` and ```__builtin_longjmp```

- Support of primitive alloca() and VLAs (variable length array) via library functions in libgcc

- Linker can eliminate not used library objects (DCE), which is enabled by default

- A complete startup file ```crt0``` is provided (which includes the initialization of the heap; also it adds support for command line argument).

- Command line arguments can be used when an autoexecutable output is run, e.g., you could invoke ```./a.out arg1 arg2``` (```/proc``` filesystem required).

- Unified ABI for variadic and non-variadic functions based on _caller pops arguments_ convention (see ABI section). It allows simple data types and structures, both for arguments and return value.

Additionaly, this project includes a bunch of C codes to test the compiler.

For more information about the toolchain see the slides in the folder ```docs```.

## Requirements <a name="requirements"></a>

The provided linking system is based on shellscripts.
At least ```bash``` version 4.4.20 is required.

The following common UNIX utilities are required as well: ```bash, ar, gawk, sed, grep, egrep,    file, which, find, sort, tr, diffutils (cmp), coreutils (basename, dirname, md5sum, cut, cat, true, readlink, csplit)```.
The ```perl``` interpreter is recommended as well, because it is used to speed up some actions.

Also _flex_ and _bison_ are required in order to compile the assembler.

## Building from sources <a name="build"></a>
Before building, read carefully instructions in https://gcc.gnu.org/install/.

Specific configuration options for the ivm64 target are these:

<font size="0">

```
Building ivm64-gcc from sources
===============================

    export GCC_SRCDIR=/path/to/gcc-10.2.0-ivm64/
    export GCC_INSTALLDIR=/path/to/installdir/gcc-10.2.0-ivm64/
    export GCC_PKGVERSION=$(grep "#define\s\+IVM64_GCC_VERSION" $GCC_SRCDIR/gcc/config/ivm64/ivm64.h|gawk '{print $3}' | tr -d '"')


Prerequisites
-------------

    EITHER install the required packages described in
           $GCC_SRCDIR/INSTALL/prerequisites.html for your system
    For example, in a deb based linux distribution, they can be installed as:
        sudo apt update
        sudo apt install libgmp-dev libmpfr-dev libmpc-dev libisl-dev libzstd-dev


    OR build them from sources:
        cd "$GCC_SRCDIR"
        ./contrib/download_prerequisites

Configuration
-------------

    cd "$GCC_SRCDIR"
    mkdir -p build
    cd build

    CFLAGS="-O2 -g" \
    CXXFLAGS="-O2 -g" \
    CFLAGS_FOR_TARGET="-O2" \
    CXXFLAGS_FOR_TARGET="-O2" \
    RANLIB_FOR_TARGET=/usr/bin/ranlib  \
    AR_FOR_TARGET=/usr/bin/ar \
    AS_FOR_TARGET=${GCC_SRCDIR}/gcc/config/ivm64/as \
    LD_FOR_TARGET=${GCC_SRCDIR}/gcc/config/ivm64/ld \
    ../configure \
    --target=ivm64 \
    --prefix="${GCC_INSTALLDIR}" \
    --with-pkgversion="ivm64-gcc ${GCC_PKGVERSION}" \
    --enable-languages=c,c++ \
    --disable-threads --disable-tls --disable-libgomp \
    --disable-bootstrap --disable-nls --disable-shared \
    --disable-multilib --disable-decimal-float \
    --disable-libmudflap --disable-libssp --disable-libquadmath \
    --disable-target-libiberty --disable-target-zlib --disable-libmpx \
    --disable-libatomic --disable-gcov --disable-plugin \
    --without-headers \
    --enable-libgcc \
    --with-newlib \
    --with-libgloss \
    --enable-newlib-mb \
    --disable-newlib-multithread \
    --disable-libstdcxx-filesystem-ts \
    --enable-sjlj-exceptions

Compile all
-------------------
    make
    make -j4 # in parallel

Compile only gcc without libraries
----------------------------------
    make all-gcc

Install
-------
    make install     # gcc and libraries
    make install-gcc # gcc only

Cleaning
--------
    make distclean # clean distribution, reconfiguration needed
    make clean     # clean all
    make clean-target-libgcc   # clean only libgcc
    make clean-target-newlib   # clean only newlib (libc)
    make clean-target-libgloss # clean only libgloss

```
</font>

## Quick start <a name="quick"></a>

To test the compiler, do the following:

* Clone this project; build it from sources or download the binaries from the release section

* Go to the project folder

  ```
    cd /path/to/project
  ```

* Add the folder cointaining the compiler driver 'ivm64-gcc' to the path (/path/to/installation is where binaries has been placed)

  ```
    export PATH=$(readlink -f $(dirname $(find /path/to/installation -iname "ivm64-gcc"))):${PATH}
  ```

* You may wish to define an emulator to use and set a memory limit:

  ```
    export IVM_MAXMEM=2000000000
    export IVM_EMU=/path/to/ivm64-emu
  ```

* Go to the example folder:

  ```
    cd ./misc/tests/examples
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
  IVM64 flex/bison-based asm to bin v2.0 - compatible with ivm-v2.0
  ....
  Binary written to:  ./a.out.b
  Symbols written to: ./a.out.sym
  Simulating with IVM_EMU=ivm64-emu
  Yet another ivm emulator, v2.0.1-fast-io
  Compatible with ivm-2.0
  Compiled with: -DSTEPCOUNT -DWITH_IO

  History Begins at Sumer

  Binary file size: 6651 bytes (6.7 KB)
  Executed 3042 instructions

  End stack:
  0x..00007e      126

  (showing all stack positions; define IVM_EMU_MAX_DUMPED_STACK to shorten)

  $ echo $?
  126
  ```

  Notice that:

  - The return value (126) is always kept on the top of the stack, 
    when the ivm simulation ends.
  - Many other values might remain in the stack above the return value, 
    due to the _crt0_ startup code that calls the standard function 
    ```exit()``` (see [Libraries](#libs)).
    Once ```exit()``` finishes its work (standard functionality), 
    it invokes ```_exit()```. This causes the machine instruction ```exit``` 
    to be executed, leaving the stack as it was at that moment.

* Argument can be passed to the generated executable (/proc filesystem required)

  ```
    $ ivm64-gcc 18-main_args.c

    $ export IVM_EMU=/path/to/ivm64-emu
    $ ./a.out arg1 arg2 2> /dev/null

    ./a.out found 3 arguments
      argv[0]=./a.out
      argv[1]=arg1
      argv[2]=arg2
      argv[3] is null
  ```


## Running IVM64 programs <a name="environ"></a>

When invoking the "executable" (e.g. a.out), we can define the emulator or assembler to be used and how to deal with the memory size and the input/output directory if needed, through the environment variables IVM_EMU, IVM_AS, IVM_MAXMEM, IVM_OUTDIR and IVM_INDIR:

- If defined ```IVM_EMU``` to be a valid emualtor (e.g export IVM_EMU=ivm64-emu), it is used as simulator, otherwise it simulates with 'ivm run'
- If defined ```IVM_AS``` to be a valid ivm assembler (asm to binary), it is used as assembler (e.g export IVM_AS=ivm64-as); by default the assembler included with the ivm compiler, 'ivm64-as', is used but you could define IVM_AS='ivm as'
- If defined ```IVM_MAXMEM=<N>```, it is used as memory size (passed to the emulator with option ```-m <N>```), otherwise it uses 64MB
- If defined ```IVM_OUTDIR=<dir>```, and directory &lt;dir&gt; exists, it is passed to the emulator with option ```-o <dir>```
- If defined  ```IVM_INDIR=<dir>```, and directory &lt;dir&gt; exists, it is passed to the emulator with option ```-i <dir>```



  For example:
  ```bash
    export IVM_MAXMEM=2000000000
    export IVM_EMU=/path/to/ivm64-emu
    export IVM_AS=ivm64-as
    ./a.out
  ```

  or in one line:

  ```bash
    IVM_MAXMEM=2000000000 IVM_EMU=/path/to/ivm64-emu IVM_AS=ivm64-as ./a.out
  ```

  It the program includes ivm input/output instructions like ```put_char```, ```new_frame```, ```set_pixel```, the input and output directory can be specified (do not forget creating these directories prior to the execution):

  ```bash
    IVM_INDIR=/tmp/in IVM_OUTDIR=/tmp/out IVM_MAXMEM=2000000000 IVM_EMU=/path/to/ivm64-emu ./a.out
  ```


## Compiler Driver <a name="bin"></a>

The compiler driver IVM64 GCC is provided for Linux. The current compiler driver has been compiled and tested in a "Ubuntu 22.04.1 LTS" distribution with linux kernel 5.15.0-56-generic for x86_64. It is located in:
```
<installation dir>/ivm64-gcc # for C
<installation dir>/bin/ivm64-g++ # for C++
```

The driver IVM64 GCC calls to the compiler itself, **cpp/cc1** (C preprocessor and compiler), and the scripts **as** (assembler) and **ld** (linker). These programs are called according to the following toolchain:

- First, 'cpp' translates the input C source file (i.e., *prog.c*) into and ASCII intermediate file (*prog.i*), and, next, 'cc1' translates the intermediate file into and ASCII assembly language file (*prog.s*).

- Second, the assembler script 'as' translates the assembly file (*prog.s*) into a relocatable object file (*prog.o*). In our case, the object file is basically the same as the assembly file, which we call the _assembly object_.

- Third, the linker script 'ld' combines one or several object files (inlcuding libraries) into a single executable file. With this purpose, all local labels of each object are renamed in case they are reused in different files. Note that this executable is a concatenation of assembly pieces with a shebang header that makes it into a true executable. This is why we call it the _assembly executable_.

- The assembly executable is just a concatenation of:
    - A shebang header, that enables its execution (this header calls the assembler and then the simulator to emulate the resulting binary)
    - the *crt0.o* startup file, which is placed at the very beginning,
    - all object files (which are really assembly files),
    - the standard C/C++ libraries, and
    - other libraries provided in the command line

## Assembler <a name="assembler"></a>

This ivm compiler is distributed together with an assembler that convert an _assembly executable_ into a binary file, which is ready to be executed by the emulator.

It is located here:
```
<installation dir>/bin/ivm64-as
```

The ```ivm64-as``` program should not be confused with the previously mentioned ```as``` script in charge of generating the assembly objects.

To build the program ivm64-as, the lexer and parser generators _flex_ and _bison_ are required.

## Compiling and Linking <a name="complink"></a>

This project contains an wide set of sample C source files, located in:
```
./misc/tests/examples
./misc/tests/tcc
```

Before continuing with the examples in this section, follow the steps in [Quick start](#quick) to prepare the working environment.

To compile and test the examples, do the following:

* Go to the example folder:
  ```
    cd ./misc/tests/examples
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
      IVM64 flex/bison-based asm to bin v2.0 - compatible with ivm-v2.0
      ....
      Binary written to:  ./a.out.b
      Symbols written to: ./a.out.sym
      Simulating with IVM_EMU=ivm64-emu

      Yet another ivm emulator, v2.0.1-fast-io
      Compatible with ivm-2.0
      Compiled with: -DSTEPCOUNT -DWITH_IO

      Hello world!
      Again, Hello world!
      strlen("Hello world!")=12
        No. of chars printed so far: 61(=0x3d)

      Binary file size: 162955 bytes (163.0 KB)
      Executed 69527 instructions

      End stack:
      0x..00003d 61

      $ echo $? # Observe the return value passed to the shell
      61
  ```

  * Also, you can execute the assembly code by invoking the ivm simulator (in this case the return value of the program is not available, and the output printed by the C program is sent to the stderr) after generating the binary:

  ```
      ivm64-as a.out
      ivm64-emu a.b
  ```


Finally, you have a complete description of the usage of both tools, 'as' and 'ld' in the files themselves.

<font size="0">


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
    # calling an emulator
    #
    # Other options:
    # --version
    #      Print version to stdout and quit
    #
    # --verbose
    #      show extra information about the linking process.
    #      Use as:
    #           ld --verbose ....
    #       or from the driver:
    #           ivm64-gcc -Xlinker --verbose ...
    #
    # -mdce / -mno-dce
    #      Enable/disable DEAD CODE ELIMINATION at library object level
    #      It is enabled by default
    #
    # --ivm64-trace, -mtrace
    #      add a label to each instruction when generating an assembly
    #      executable (extension other than .S/.o); this breaks all
    #      basic blocks, preventing the emulator from optimizing the assembly.
    #      To be deprecated as we have "ivm --noopt" to disable emulation
    #      optimizations.
    #      Use as:
    #           ld --ivm64-trace ...
    #       or from the driver:
    #           ivm64-gcc -Xlinker --ivm64-trace ...
    #
    # -mcrt0, -mno-crt0
    #      force to prepend or not a startup sequence (crt0) when
    #      generating an executable.
    #      By default, it is not prepended because crt0.o is commonly
    #      passed by the driver.
    #      But it can be useful when gcc is invoked  with -nostdlib
    #      or -nostartfiles
    #      Example:
    #           ivm64-gcc -Xlinker -mcrt0 -nostartfiles ...
    #      It uses the startup file crt0.S if available; otherwise a startup
    #      sequence if generated
    #
    # -mbin
    #      generate the binary after the "assembly executable" is done,
    #      by invoking a true assembly. This assembly application is taken
    #      from the environment variable IVM_AS. Do not forget including
    #      the corresponding path in PATH variable.
    #      For example:
    #          export IVM_AS="ivm as"                 # use the ivm implementation
    #          export IVM_AS="ivm64-as"          # use the flex/bison based ivm assembly
    #          export IVM_AS="ivm64-as --noopt"  # use the flex/bison based ivm assembly w/o optimization
    #      This flag can be very useful when configuring projects that use
    #      the configure program from gnu autotools. The configure program may
    #      need to check if a function is available by compiling a given test
    #      (e.g. mmap()). So in these cases the binary needs to be generated in
    #      case such a function is not defined (to make fail the linking).
    #      For example a typical use would be:
    #           IVM_AS="ivm64-as --noopt" LDFLAGS="-Xlinker -mbin" CC=ivm64-gcc ../configure --host=ivm64
    #
    #
    # The behaviour of this script is affected by these environment variables:
    #    IVM64_LD_MAX_LABEL_LEN
    #        This variable specifies the maximum number of characters used
    #        when comparing labels during DCE. Its default value is 24. Higher
    #        values increases the ld execution time but could make the DCE process
    #        more effective. Too low values can affect negatively both the
    #        linking time and the size of final program.
    #    IVM64_LD_TMPDIR
    #        This variable set the temporary directory. The default value
    #        is this one of mktemp command.
    #
    # File extension convention:
    #   .s -> assembly (from c source)
    #   .S -> assembly (precompiled libraries or modules, startup files,...)
    #   .o -> object (actually containing assembly)
    #   .a -> ar file containing objects
    #
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
    #
    # The resulting "executable" can be executed thanks to a
    # shebang preamble. To do this, the ivm implementation (ivm) is required to be
    # in PATH. Also an alternative simulator (IVM_EMU), and a memory limit (IVM_MAXMEM)
    # can be set.
    #
    # Examples:
    #
    #   you can run this file:
    #       ./a.out
    #   using your favourite ivm emulator:
    #       IVM_EMU=ivm64-emu ./a.out
    #   passing arguments:
    #       IVM_EMU=ivm64-emu ./a.out 1 2 3
    #   setting the memory size
    #       IVM_MAXMEM=100000000 IVM_EMU=ivm64-emu ./a.out
    #   setting the memory size and selecting output/input directory:
    #       IVM_MAXMEM=100000000 IVM_OUTDIR=/tmp IVM_INDIR=/tmp/in IVM_EMU=ivm64-emu ./a.out
    #       setting the assembler (using "ivm as" by default)
    #       IVM_AS="ivm64-as" ./a.out
    #       IVM_AS="ivm as" ./a.out
    #           or
    #       export IVM_EMU=ivm64-emu
    #       export IVM_AS="ivm64-as"
    #       export IVM_MAXMEM=100000000
    #       export IVM_OUTDIR=/tmp
    #       export IVM_INDIR=/tmp/in
    #       ./a.out 1 2 3
    # 
    # This script processes assembly on a line basis, so that codes
    # are subjected to the following conventions:
    #
    #   - One and only instruction per line
    #      Supported:             Not supported:
    #          push! 3              push! 3 push! 4
    #          push! 4
    #
    #          push! 5              push!
    #                                     5
    #
    #   - Label/Alias declaration: alone and in one only line
    #      Supported:             Not supported:
    #          main:                main
    #                                    :
    #
    #          var:                 var: data1 [0]
    #               data1 [0]
    #
    #   - Data declarations must be in one only line
    #      Supported:             Not supported:
    #          data1 [0 0 0 0]      data1 [0 0
    #                                      0 0]
    #
    
```

</font>

## Libraries <a name="libs"></a>

The current distribution includes several precompiled libraries. These are:

- _libc.a_ and _libm.a_: the standard and mathematical C libraries from **newlib**
- _libgcc.a_: the **soft float** library from GCC
- _libstdc++.a_: the standard gcc C++ library
- _crt0.o_: the C runtime startup file

Although a simplistic _crt0.o_ could be as follows, the provided one is more sophisticated including the parsing of command line arguments and environment variables:

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

## Testing compiler features <a name="testing"></a>

A testsuite coming from [tcc compiler project](https://bellard.org/tcc/) is provided in this project. It is placed in the directory ```./tests/tcc```.

The requierements for this test are the 'make' tool and the 'gcc' compiler for the host (in order to generate the reference output files to compare).

To run the test, just type:

```bash
make very-clean
make -k check
```

This command will compile each C source code with both the 'gcc' for the host and the ivm64-gcc compilers. The host executable is run to provide the reference output for such program (with the .expect suffix). The ivm64 executable is run (through _ivm as-run_) and its output is stored with the '.sdout' suffix. Finally, both outputs are compared, when they match a '.test' file is generated, otherwise an '.error' file produced.

From the original set, those C codes requiring features not supported by the abstract machine (e.g. frame pointer register, file operations,...) are excluded from compiling. In the other hand, those tests whose output depend on the data alignment are not compared as they always differ from the host version, but they can run correctly if invoked by hand.

## ABI <a name="abi"></a> (calling convention)
This section describes in short the Application Binary Interface (ABI) related to the calling convention used by ivm64-gcc. Assembly codes aspiring to interoperate with gcc-generated functions must satisfy these conventions.

A unique ABI is used for all functions regardless the number of arguments (no arguments/fixed number of arguments/variable number of arguments). This calling convention is based on the "caller pops arguments" rule, so that the **caller** is in charge of releasing the arguments and move the return value to its destination:

  - Caller may allocate one stack one or more slots for the return value
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


## Extra linker feature: DCE <a name="dce"></a>

The current linker is able to perform Dead Code Elimination (DCE) of those library objects not referenced by the main program files. This feature allows the linker to find the objects, and only these objects, on which the codes being compiled depend, and incorporate them to the assembly output (instead of the full libraries).

This feature is enable by default in current version.

To enable/disable this feature, add the flags "```-Xlinker -mdce```"  or" ```-Xlinker -mno-dce```", respectively, to the gcc command line invokation. For example:

```bash
ivm64-gcc -Xlinker -mdce 00-main.c  # This generates a.out 

```

You can observe that the ```a.out``` output file is much smaller than the resulting file when the DCE feature is not enabled. Here are some examples:

```bash
ivm > ### NO DCE
ivm > time ivm64-gcc -Xlinker -mno-dce 00-main.c

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



## Evaluation <a name="comparison"></a>

Here you can find some figures comparing different optimization levels:
- codes have been compiled and executed on a Intel Core i7-3770 @3.40GHz server running a linux kernel 4.15.0
- compiler version **ivm64-2.1** based on **GCC 10.2.0** was used
- binaries were generated with **ivm v0.37** (be aware that ISA changed in this version)
- simulations were carried out with the **yet another (fast) IVM emulator v1.17**

<!--
This table shows the results for the "static-unboxer" program. Note that this code includes some embedded data, so the reduction in size is less noticeable.

|                      | #executed<br>instructions | asm size | binary size | "ivm as" time<br>(only assembling) | ivm\_emu\_fast time<br>(simulation time) |
| -------------------- | ------------------------- | -------- | ----------- | ---------------------------------- | ---------------------------------------- |
| **static-unboxer** (-O2) |                |          |             |        |       |
| ivm64-gcc 1.0rc3     | 111477 millions    | 12MB     | 2.6MB       | 14.2s  | 2m35s |
| ivm64-gcc 1.0rc4     | 63138 millions     | 11MB     | 2.2MB       | 12.8s  | 1m25s |
| ivm64-gcc 1.0rc5     | 39481 millions     | 9MB      | 1.8MB       | 12.0s  | 0m57s |
| ivm64-gcc 1.1rc1     | 18701 millions     | 8.8MB    | 1.6MB       | 10.8s  | 0m38s |                                   
| ivm64-gcc 1.2rc1 (gcc8)  | 17665 millions     | 8.7MB    | 1.5MB       | 10.8s  | 0m38s |
| ivm64-gcc 1.3rc1 (gcc10) | 16277 millions | 8.6MB    | 1.4MB       | 10.5s  | 0m33s |
-->

This table shows the results for the **static-unboxer** program for different optimization levels . Note that this code includes some embedded data, so the reduction in size is less noticeable.


| Optimization<br>level | Executed<br>instructions | Binary size | space x time<br>factor<sup>$</sup> |
|-----------------|----------------------|-------------|---------------|
| -O3             | **14408 millions**   | 1466 KB     | 22 |
| -Os             | 16036 millions       | **1279 KB** | 21 |
| -O2 (default)   | 14968 millions       | 1369 KB     | **20** |
| -O0             | 45171 millions       | 1700 KB     | 77 |

<sup> $</sup><small>executed_instructions (millions) * size (KB)/1e6</small>

For this example, next table summarized the time required to assembly the resulting code and simulate it using the fast simulator (yet_another_ivm_emulator repository):

| Optimization<br>level | assembly time<br>(ivm as) | simulation time<br>(ivm64-emu)|
|--------------------|--------------------------|-------------|
| -O2 (default)      | ~10s. | 35s. |
| -Os                | ~10s. | 45s. |

## Compiler Explorer Tool <a name="cedemo"></a>

The IVM64 GCC compiler is also available for testing purposes through the web-based Compiler Explorer tool.

This tool can be accessed through the URL:  https://ivm.ac.uma.es/ce-layout.html.  

A simplified view of the tool is also available in: https://ivm.ac.uma.es.

Note that as the simulation may run slow after including all the standard libraries, you may wish to close the execution panel in order to browse the compiled assembly without waiting the execution ends.


