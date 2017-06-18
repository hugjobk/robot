all: server test_search

server: server.o graph.o search.o
	gcc -o server server.o graph.o search.o `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

server.o: server.c
	gcc -c server.c `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

test_search: test_search.o graph.o search.o
	gcc -o test_search test_search.o graph.o search.o

test_search.o: test_search.c
	gcc -c test_search.c

graph.o: graph.c
	gcc -c graph.c

search.o: search.c
	gcc -c search.c

clean:
	rm -rf *o server test_search
