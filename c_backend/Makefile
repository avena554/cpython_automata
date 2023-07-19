CC = gcc
DEBUG=0
CFLAGS =  -g -W -Wall -Wextra -std=c11 -DDEBUG=$(DEBUG)# enable many GCC warning
LDFLAGS = -g
VGFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

avl:
	$(MAKE) -C avl avl.o

avl/avl.c:
	$(MAKE) -C avl avl.c

avl_src: avl/avl.c

dict.c: dict.h
automata.c: automata.h

dict.o: dict.c avl
	$(CC) $(CFLAGS) -c  dict.c -o $@

test_dict: dict.o avl/avl.o list.o utils.o test_dict.c
test_automata: automata.o dict.o avl/avl.o utils.o list.o test_automata.c

test_inter: automata.o dict.o avl/avl.o utils.o list.o test_inter.c

grind_inter: test_inter
	valgrind $(VGFLAGS) test_inter

run_test_automata: test_automata
	test_automata

grind_test_automata: test_automata
	valgrind $(VGFLAGS) test_automata

test_list: list.o test_list.c

test_utils: utils.o test_utils.c

run_test_utils: test_utils
	test_utils

grind_test_utils: test_utils
	valgrind $(VGFLAGS) test_utils

run_test_list: test_list
	test_list

grind_test_list: test_list
	valgrind $(VGFLAGS) test_list

build_pyta: avl/avl.c
	python3 setup.py build

install: avl/avl.c
	python3 -m build
	pip3 install dist/pyta_c_backend-*.whl

clean:
	rm -f *.o
	rm -f test_dict 
	$(MAKE) -C avl clean
	rm -rf build/
