teste: recep.o appif.o
	gcc -Wall -o recep recep.o appif.o

appif.o: appif.c appif.h typedef.h
	gcc -Wall -c appif.c

recep.o: recep.c
	gcc -Wall -c recep.c

clean:
	rm *.o recep
