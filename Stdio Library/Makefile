all: build

build:
	gcc -Wall -g -fPIC -c tema2.c
	gcc -Wall -g -shared tema2.o -o libso_stdio.so

clean:
	rm -f *.o
