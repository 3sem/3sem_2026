all: echo_test

echo_test: main.c dup_pipe.c dup_pipe.h
	gcc -Wall -o echo_test main.c dup_pipe.c

clean:
	rm -f task2