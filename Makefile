all: lex

lex: lex.c
	gcc -O2 -o lex lex.c

dbg: lex.c
	gcc -g -o lex lex.c

clean:
	rm -f lex
