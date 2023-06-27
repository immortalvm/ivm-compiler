#!/bin/bash

# Script to compare test outputs using "ivm64-asm2bin" and "ivm as"

RUNDIR=run

ASM1="ivm64-asm2bin"
ASM2="ivm as"

EMU="ivm_emu"

OUTPUT1=output-ias.txt
OUTPUT2=output-ivm-as.txt

mkdir -p "$RUNDIR"

test_run() {
    rm -f "$RUNDIR"/*.b "$RUNDIR"/*.sym
    for i in *.s
    do
         echo "$i================="
         $1 $i --bin "$RUNDIR/$i.b" --sym "$RUNDIR/$i.sym"
         echo
         test -f "$RUNDIR/$i.b" && $2 "$RUNDIR/$i.b"
         echo
    done &> "$3"
} # Usage: test_run <ASM> <EMU> <OUTPUTFILE>


unset IVM_EMU_MAX_DUMPED_STACK
test_run "$ASM1" "$EMU" "$OUTPUT1" # Using ivm64-asm2bin
test_run "$ASM2" "$EMU" "$OUTPUT2" # Using "ivm as"

