#-------------------------------------------------------------------------------
# File: Makefile
#
# $ make -k all           // compiles every C_source_file into diferent execs
# $ make <C_source_file_w/o_C_extension>        // compiles 1 program
# $ make <C_source_file_w/o_C_extension>.test   // tests 1 program
# $ make <C_source_file_w/o_C_extension>.stdout // runs 1 program and gets the output
# $ make run                                  // compiles & run every program
# $ make check                                // compiles & tests every program
# $ make expect           // generates reference output executing in host
# $ make clean                                // clean execs & test output files
# $ make clean-test                           // clean test output files
# $ make very-clean                           // clean all generated files
#
#
# Authors: Sergio Romero Montiel
#          Eladio Guierrez Carrasco
#
# Created on February 2020
#-------------------------------------------------------------------------------

#---------------------- FILES & DIRECTORIES VARIABLES --------------------------
# FILES
# Source list
SOURCES=$(sort $(wildcard *.c))
# skip side-files (variables for program depending on extern vars)
SKIPSRC :=
# skip non supported features
SKIPSRC += 34_array_assignment.c 83_utf8_in_identifiers.c 85_asm-outside-function.c 78_vla_label.c 79_vla_continue.c 96_nodata_wanted.c 40_stdio.c 98_al_ax_extend.c 46_grep.c 60_errors_and_warnings.c
EXECSRC := $(filter-out $(SKIPSRC), $(SOURCES))
# ivm64 executables (only compiling & linking)
EXECS := ${EXECSRC:.c=}

# skip programs that never return 
SKIPTEST := 90_struct-init.c 95_bitfields.c
TESTSRC := $(filter-out $(SKIPTEST), $(EXECSRC))
# ivm64 executables for checking
CHECKS := ${TESTSRC:.c=.test}
STDOUT := ${TESTSRC:.c=.stdout}
EXPECT := ${TESTSRC:.c=.expect}

# DIRECTORIES


#---------------------- AUTOMATIC VARIABLES ------------------------------------
# Compiler
CC = ivm64-gcc
# Compiler opts
CFLAGS = -O0
LDFLAGS = -Xlinker -mdce
#Only for math:
#LDLIBS = -lm

# --------------------- RULES --------------------------------------------------
# Targets & suffixes
.PHONY: all check expect clean clean-test very-clean
.PRECIOUS: %.expect %.output

# rule for compiling all
all:  $(EXECS)

run: $(STDOUT)

# rule to do all the testing (generate .expect, run execs and cmp results)
check: $(CHECKS)

expect: $(EXPECT)

%.test: %.expect %.stdout
	@echo "-------$@------------------------"
	@cmp $^ && echo "[+] Output matched the expected one" | GREP_COLOR="38;5;21" grep --color=always ".*" | tee $@ || echo "[-] Output mismatched the expected one" | tee $@.error | GREP_COLOR="38;5;160" grep --color=always ".*"
	@echo "---------------------------------"

%.stdout: %
	./$< | tee $@ | GREP_COLOR="38;5;162" grep --color=always ".*" ; true

%.expect: %.c
	@gcc $< && ./a.out > $@ && rm a.out || true

# cleanup rules
clean: clean-test
	@-rm -fv $(EXECS)

clean-test:
	@-rm -fv $(STDOUT) $(CHECKS)

very-clean: clean
	@-rm -fv *.error a.out

