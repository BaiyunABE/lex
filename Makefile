all: lex

debug: lex.c
	gcc -g -o lex lex.c

clean:
	rm -f lex
