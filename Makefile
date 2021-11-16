CC = gcc
CFLAGS = -g -W -Wall -std=c11 # enable many GCC warning
LDFLAGS = -g

avl:
	cd avl && $(MAKE) avl.o 
dict.c: dict.h
automata.c: automata.h

dict.o: dict.c avl
	$(CC) $(CFLAGS) -c  dict.c -o $@

test_dict: dict.o avl/avl.o test_dict.c
test_automata: automata.o dict.o avl/avl.o test_automata.c

clean:
	rm -f *.o
	rm -f test_dict
	cd avl && $(MAKE) clean
