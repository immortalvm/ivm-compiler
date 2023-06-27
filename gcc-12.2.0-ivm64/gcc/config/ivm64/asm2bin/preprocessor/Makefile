FILE=aspp
CFLAGS=-O2 -static
CC=gcc

all: aspp0 aspp1

aspp0: y.tab.0.c lex.yy.0.c ivm64_alias_helper.c ../ivm64_utils.c
	$(CC) $(CFLAGS) $< $(word 2,$^) ../ivm64_utils.c -lfl -o $@ -Wstringop-overflow=0

aspp1: y.tab.1.c lex.yy.1.c ivm64_alias_helper.c ../ivm64_utils.c
	$(CC) $(CFLAGS) $< $(word 2,$^) ../ivm64_utils.c -lfl -o $@ -Wstringop-overflow=0

y.tab.%.c: $(FILE)%.y
	bison -v -y -d $< -o $@

lex.yy.%.c: $(FILE)%.l
	flex -o $@ $<

clean: clean_lex clean_yacc clean_exec

clean_yacc:
	rm -f y.tab.*.*

clean_lex:
	rm -f lex.yy.*.*

clean_exec:
	rm -f aspp0 aspp1
