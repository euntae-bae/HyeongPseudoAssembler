hyasm: main.o
	gcc -o hyasm main.o

main.o: main.c
	gcc -c main.c

clean:
	rm *.o hyasm*
