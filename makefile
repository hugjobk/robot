all: server

server: server.o graph.o search.o
	gcc -o server server.o graph.o search.o `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

server.o: server.c
	gcc -c server.c `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

graph.o: graph.c
	gcc -c graph.c

search.o: search.c
	gcc -c search.c

clean:
	rm -rf *o server test_search
