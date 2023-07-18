CC = gcc
DEBUG=0
CFLAGS =  -g -W -Wall -Wextra -std=c11 -DDEBUG=$(DEBUG)# enable many GCC warning
LDFLAGS = -g
VGFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

pyta/avl:
	$(MAKE) -C pyta/avl avl.o

pyta/avl/avl.c:
	$(MAKE) -C pyta/avl avl.c

avl_src: pyta/avl/avl.c

pyta/dict.c: pyta/dict.h
pyta/automata.c: automata.h

pyta/dict.o: pyta/dict.c pyta/avl
	$(CC) $(CFLAGS) -c  pyta/dict.c -o $@

pyta/test_dict: pyta/dict.o pyta/avl/avl.o pyta/list.o pyta/utils.o pyta/test_dict.c
pyta/test_automata: pyta/automata.o pyta/dict.o pyta/avl/avl.o pyta/utils.o pyta/list.o pyta/test_automata.c

pyta/test_inter: pyta/automata.o pyta/dict.o pyta/avl/avl.o pyta/utils.o pyta/list.o pyta/test_inter.c

grind_inter: pyta/test_inter
	valgrind $(VGFLAGS) pyta/test_inter

run_test_automata: pyta/test_automata
	pyta/test_automata

grind_test_automata: pyta/test_automata
	valgrind $(VGFLAGS) pyta/test_automata

pyta/test_list: pyta/list.o pyta/test_list.c

pyta/test_utils: pyta/utils.o pyta/test_utils.c

run_test_utils: pyta/test_utils
	pyta/test_utils

grind_test_utils: pyta/test_utils
	valgrind $(VGFLAGS) pyta/test_utils

run_test_list: pyta/test_list
	pyta/test_list

grind_test_list: pyta/test_list
	valgrind $(VGFLAGS) pyta/test_list

clean:
	cd pyta
	rm -f *.o
	rm -f test_dict 
	$(MAKE) -C pyta/avl clean
