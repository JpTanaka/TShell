CFLAGS = -Wall -Wextra

tshell: main.o built_in_functions.o
	gcc $(CFLAGS) main.o built_in_functions.o -o TShell

main.o: main.c 
	gcc $(CFLAGS) -c main.c

built_in_functions.o: 
	gcc $(CFLAGS) -c built_in_functions.c

