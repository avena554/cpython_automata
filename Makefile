CC = gcc
CFLAGS = -g -W -Wall -Wextra -std=c11 # enable many GCC warning
LDFLAGS = -g
VGFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

avl:
	cd avl && $(MAKE) avl.o 
dict.c: dict.h
automata.c: automata.h

dict.o: dict.c avl
	$(CC) $(CFLAGS) -c  dict.c -o $@

test_dict: dict.o avl/avl.o test_dict.c
test_automata: automata.o dict.o avl/avl.o test_automata.c

run_test_automata: test_automata
	./test_automata

grind_test_automata: test_automata
	valgrind $(VGFLAGS) ./test_automata

test_list: list.o test_list.c

run_test_list: test_list
	./test_list

grind_test_list: test_list
	valgrind $(VGFLAGS) ./test_list

build_pyta: setup.py
	python3 setup.py build
	cp build/lib.*/*.so pyta/

clean:
	rm -f *.o
	rm -f test_dict
	rm -rf build/* 
	cd avl && $(MAKE) clean
