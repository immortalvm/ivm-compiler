#ifndef _IVM64_HELPER_H_
#define _IVM64_HELPER_H_

// Functions in ivm64_utils.c
char *gen_tempfile(const char *template);
void file_put_contents(char *filename, FILE* out_file);
void stdin_to_file(char *filename);
char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

// Native instructions operation codes
enum OPCODES {
    OPCODE_EXIT         = 0x00,
    OPCODE_NOP          = 0x01,
    OPCODE_JUMP         = 0x02,
    OPCODE_JZFWD        = 0x03,
    OPCODE_JZBACK       = 0x04,
    OPCODE_SETSP        = 0x05,
    OPCODE_GETPC        = 0x06,
    OPCODE_GETSP        = 0x07,
    OPCODE_PUSH0        = 0x08,
    OPCODE_PUSH1        = 0x09,
    OPCODE_PUSH2        = 0x0a,
    OPCODE_PUSH4        = 0x0b,
    OPCODE_PUSH8        = 0x0c,
    OPCODE_LOAD1        = 0x10,
    OPCODE_LOAD2        = 0x11,
    OPCODE_LOAD4        = 0x12,
    OPCODE_LOAD8        = 0x13,
    OPCODE_STORE1       = 0x14,
    OPCODE_STORE2       = 0x15,
    OPCODE_STORE4       = 0x16,
    OPCODE_STORE8       = 0x17,
    OPCODE_ADD          = 0x20,
    OPCODE_MULT         = 0x21,
    OPCODE_DIV          = 0x22,
    OPCODE_REM          = 0x23,
    OPCODE_LT           = 0x24,
    OPCODE_AND          = 0x28,
    OPCODE_OR           = 0x29,
    OPCODE_NOT          = 0x2a,
    OPCODE_XOR          = 0x2b,
    OPCODE_POW          = 0x2c,
    OPCODE_CHECK        = 0x30,
    // native IO insn
    OPCODE_READCHAR     = 0xf8,
    OPCODE_PUTBYTE      = 0xf9,
    OPCODE_PUTCHAR      = 0xfa,
    OPCODE_ADDSAMPLE    = 0xfb,
    OPCODE_SETPIXEL     = 0xfc,
    OPCODE_NEWFRAME     = 0xfd,
    OPCODE_READPIXEL    = 0xfe,
    OPCODE_READFRAME    = 0xff,
};

/* Opcode to native mnemonic table */
/*
static char *OPCODE_STR[256] =
{
    [0x00]="exit"         ,
    [0x01]="nop"          ,
    [0x02]="jump"         ,
    [0x03]="jzfwd"        ,
    [0x04]="jzback"       ,
    [0x05]="setsp"        ,
    [0x06]="getpc"        ,
    [0x07]="getsp"        ,
    [0x08]="push0"        ,
    [0x09]="push1"        ,
    [0x0a]="push2"        ,
    [0x0b]="push4"        ,
    [0x0c]="push8"        ,
    [0x10]="load1"        ,
    [0x11]="load2"        ,
    [0x12]="load4"        ,
    [0x13]="load8"        ,
    [0x14]="store1"       ,
    [0x15]="store2"       ,
    [0x16]="store4"       ,
    [0x17]="store8"       ,
    [0x20]="add"          ,
    [0x21]="mult"         ,
    [0x22]="div"          ,
    [0x23]="rem"          ,
    [0x24]="lt"           ,
    [0x28]="and"          ,
    [0x29]="or"           ,
    [0x2a]="not"          ,
    [0x2b]="xor"          ,
    [0x2c]="pow"          ,
    [0x30]="check"        ,
    [0xf8]="readchar"     ,
    [0xf9]="putbyte"      ,
    [0xfa]="putchar"      ,
    [0xfb]="addsample"    ,
    [0xfc]="setpixel"     ,
    [0xfd]="newframe"     ,
    [0xfe]="readpixel"    ,
    [0xff]="readframe"
};
*/
#endif
