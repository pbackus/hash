CC=gcc
CFLAGS= -std=c99 -pedantic -Wall

hash_test: hash_test.o hash.o

check: hash_test
	./hash_test

clean:
	rm hash_test *.o
