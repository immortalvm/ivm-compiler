Preprocessing IVM64 assembly aliases (aka macros or abbreviations)
with flex/bison.

This preprocessing is only necessary when the assembly
includes complex macros, that are not pure constants or labels
(e.g. "a=3", "b=hello" are handled by the assembler,
but "p=(load8 b)" is not).

>>> Note that this preprocessor handles aliases that must
>>> be declared always in one only line,

The ivm64-gcc compiler never generates complex macros, but
they may be present if some by-hand assembly code was linked.

The preprocessor performs two phases (aspp0, aspp1):

   First, it puts all the alias definitions at the beginning
   of the file, after replacing aliases in aliases recursively.

   And then, aliases are replaced in the rest of the code

Usage:
    # all aliases should be expanded in output.s
    # which does not already include any alias definition
    aspp0 < file.s | assp1 > output.s
