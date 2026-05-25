build: myShell.c
	gcc -Wall -g myShell.c -o myShell

run: build
	./myShell

clean:
	rm -f myShell
