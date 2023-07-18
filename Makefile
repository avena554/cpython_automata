CC = gcc
DEBUG=0
CFLAGS =  -g -W -Wall -Wextra -std=c11 -DDEBUG=$(DEBUG)# enable many GCC warning
LDFLAGS = -g
VGFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

build_pyta:
	make -C src avl_src
	python3 setup.py build
	cp build/lib*/pyta*.so src/

clean:
	make -C src clean
	rm src/pyta_core*.so
	rm -rf build
