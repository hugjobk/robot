#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wait.h>
#include <unistd.h>
#include "graph.h"
#include "search.h"

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 5000 /*port*/
#define LISTENQ 8 /*maximum number of client connections */
#define WIDTH 6
#define HEIGHT 8
//Direction Types:
#define LEFT "left"
#define RIGHT "right"
#define UP "up"
#define DOWN "down"

GtkWidget *window;
GtkWidget *grid;
GtkWidget *buttons[WIDTH][HEIGHT];

int listenfd, connfd, sockfd;
pid_t childpid;
socklen_t clilen;
char buf[MAXLINE], sendline[MAXLINE], recvline[MAXLINE];
struct sockaddr_in cliaddr, servaddr;

int map[WIDTH][HEIGHT];
FILE *f;
Graph g;

typedef struct {
  int x;
  int y;
} Point;

gpointer start_server(gpointer args);
Point getPosition(GtkWidget *button);
static void callback(GtkWidget *widget, gpointer data);
char *getDir(int n1, int n2);

int main(int argc, char *argv[])
{
  f = fopen("map.txt", "r+");
  g = graph_create(WIDTH * HEIGHT);

  gtk_init(&argc, &argv);
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(window, 600, 800);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  g_signal_connect_swapped(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
  gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

   while (!feof(f)) {
    int x, y, status;
    fscanf(f, "%d %d %d\n", &x, &y, &status);
    map[x][y] = status;
    buttons[x][y] = gtk_button_new();
    if (map[x][y] == 0)
      gtk_button_set_label(GTK_BUTTON(buttons[x][y]), "X");
    else if (map[x][y] == 2)
      gtk_button_set_label(GTK_BUTTON(buttons[x][y]), "O");
    g_signal_connect(buttons[x][y], "clicked", G_CALLBACK(callback), NULL);
    gtk_grid_attach(GTK_GRID(grid), buttons[x][y], x, y, 1, 1);
  }

  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (x < (WIDTH - 1))
	if (map[x][y] == 1 && map[x + 1][y] == 1) {
	  graph_add_edge(g, x + y * WIDTH, (x + 1) + y * WIDTH);
	  graph_add_edge(g, (x + 1) + y * WIDTH, x + y * WIDTH);
	}
       if (y < (HEIGHT - 1))
	 if (map[x][y] == 1 && map[x][y + 1] == 1) {
	   graph_add_edge(g, x + y * WIDTH, x + (y + 1) * WIDTH);
	   graph_add_edge(g, x + (y + 1) * WIDTH, x + y * WIDTH);
	 }
    }
  }

  gtk_container_add(GTK_CONTAINER(window), grid);

  gtk_widget_show_all(window);

  g_thread_new(NULL, start_server, NULL);
  
  gtk_main();
}

gpointer start_server(gpointer args)
{
  connfd = 0;
  //creation of the socket
  listenfd = socket (AF_INET, SOCK_STREAM, 0);
    
  //preparation of the socket address 
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);
  
  bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    
  listen (listenfd, LISTENQ); 
  printf("Server started...\n");
  
  for ( ; ; ) {
    clilen = sizeof(cliaddr);
    connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
    char client[25];
    strcpy(client, inet_ntoa(cliaddr.sin_addr));
    printf("%s connected to server.\n", client);
    while (recv(connfd, buf, MAXLINE,0) > 0)  {
      printf("%s\n", buf);
      memset(buf, 0, sizeof(buf));
    }
    connfd = 0;
  }
}

static void callback(GtkWidget *widget, gpointer data) {
  Point p = getPosition(widget);

  struct search_info *s = search_info_create(g);
  
  bfs(s, 0);

  int size  = s->depth[p.x + p.y * WIDTH] + 1;
  int path[size];

  int i = p.x + p.y * WIDTH;
  int j = size - 2;

  path[0] = 0;
  path[size - 1] = i;

  while (s->parent[i] != 0) {
    i = s->parent[i];
    path[j] = i;
    j--;
  }

  strcpy(sendline, "DIR:");

  for (i = 0; i < size - 1; i++) {
    strcat(sendline, getDir(path[i], path[i + 1]));
    if (i < size - 2)
      strcat(sendline, "|");
  }
  printf("%s\n", sendline);
  if (connfd != 0) {
    send(connfd, sendline, strlen(sendline), 0); 
  }

  search_info_destroy(s);
}

Point getPosition(GtkWidget *button)
{
  Point point;
  point.x = -1;
  point.y = -1;
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (button == buttons[i][j]) {
	point.x = i;
	point.y = j;
      }
    }
  }
  return point;
}

char *getDir(int n1, int n2) {
  int dis = n2 - n1;
  switch (dis) {
  case 1:
    return RIGHT;
  case -1:
    return LEFT;
  case 6:
    return UP;
  case -6:
    return DOWN;
  default:
    return NULL;
  }
}
