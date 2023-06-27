FILE=as

.PHONY: all parser preprocessor

CC=gcc
CFLAGS=-O2 -static

all: parser preprocessor

parser: as0 as1 asopt1

as0: y.tab.0.c lex.yy.0.c ivm64_helper_bison.c ivm64_helper_lex.c ivm64_utils.o
	$(CC) $(CFLAGS) $< $(word 2,$^) ivm64_utils.o -lfl -o $@

as1: y.tab.1.c lex.yy.1.c ivm64_helper_bison.c ivm64_helper_lex.c ivm64_utils.o
	$(CC) $(CFLAGS) $< $(word 2,$^) ivm64_utils.o -lfl -o $@

asopt1: y.tab.opt1.c lex.yy.opt1.c ivm64_helper_bison.c ivm64_helper_lex.c ivm64_utils.o
	$(CC) $(CFLAGS) $< $(word 2,$^) ivm64_utils.o -lfl -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

y.tab.%.c: $(FILE)%.y
	bison -v -y -d $< -o $@

lex.yy.%.c: $(FILE)%.l
	flex -o $@ $<

preprocessor:
	cd preprocessor; make; cp aspp0 aspp1 ..

clean: clean-lex-yacc clean-exec clean-preprocessor

clean-lex-yacc:
	rm -f y.tab.*.* lex.yy.*.* *.o
	cd preprocessor; rm -f y.tab.*.* lex.yy.*.* *.o; cd ..

clean-exec:
	rm -f as0 as1 aspp0 aspp1 asopt1

clean-preprocessor:
	cd preprocessor; make clean
