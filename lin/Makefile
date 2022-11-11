build: so_stdio.o
	gcc -shared -g  so_stdio.o -o libso_stdio.so
	
so_stdio.o: so_stdio.c so_stdio.h
	gcc -c -fPIC -Wall -g so_stdio.c

clean:
	rm *.o libso_stdio.so
