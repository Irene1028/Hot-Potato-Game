
/*
ECE 650 Hw03 Hot Potato

This file contains the code for only one player

Author: Ying Xu
Date: 02/15/2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include "potato.h"
#include <netinet/in.h>
#include <arpa/inet.h>


void connect_neighbor(int *status, int *socket_fd, struct addrinfo *host_info, struct addrinfo **host_info_list, const char *hostname, const char *port){
  memset(host_info, 0, sizeof(*host_info));//make sure the struct is empty 

  host_info->ai_family   = AF_UNSPEC;
  host_info->ai_socktype = SOCK_STREAM;
  *status = getaddrinfo(hostname, port, host_info, host_info_list);
  if (*status != 0) {
    fprintf(stderr, "Error: cannot get address info for host\n");
    exit(1);
  } //if

  *socket_fd = socket((*host_info_list)->ai_family, 
		      (*host_info_list)->ai_socktype, 
		      (*host_info_list)->ai_protocol);
  if (*socket_fd == -1) {
    fprintf(stderr,"Error: cannot create socket\n");
    exit(1);
  } //if

  /* connect */
  *status = connect(*socket_fd, (*host_info_list)->ai_addr, (*host_info_list)->ai_addrlen);
  if (*status == -1) {
    fprintf(stderr, "Error: cannot connect to socket\n");
    exit(1);
  } //if
}


void connect_master(int *status, int *socket_fd, struct addrinfo *host_info, struct addrinfo **host_info_list, const char *hostname, const char *port){
  //call connect_neighbor to do normal connect job
  connect_neighbor(status, socket_fd, host_info, host_info_list, hostname, port);

  //Get hostname of player and send to master
  char host[100] = {0};
  if(gethostname(host,sizeof(host)) < 0){
    fprintf(stderr, "gethostname wrong\n");
    exit(1);
  }
  struct hostent *hp;
  if ((hp=gethostbyname(host)) == NULL){
    fprintf(stderr, "gethostbyname\n");
    exit(1);
  }
  /*
  int i = 0;
  while(hp->h_addr_list[i] != NULL){
    printf("hostname: %s\n",hp->h_name);
    printf("      ip: %s\n",inet_ntoa(*(struct in_addr*)hp->h_addr_list[i]));
    i++;
  }
  */
  int len = strlen(hp->h_name);
  send(*socket_fd, &len, sizeof(int), 0);
  send(*socket_fd, hp->h_name, len, 0);  
}


void receive_playerinfo(int *socket_fd, struct player_info *info){
  memset(info, 0, sizeof(*info));//make sure the struct is empty 
  recv(*socket_fd, info, sizeof(struct player_info), 0);
  printf("Connected as player %d out of %d total players\n", info->player_number, info->num_players);
}

void receive_neighinfo(int *socket_fd, struct neighbor_info *nb){
  memset(nb, 0, sizeof(*nb));//make sure the struct is empty 
  recv(*socket_fd, nb, sizeof(struct neighbor_info), 0);
  //printf("Left player is %d through port %d, right player is %d through port %d.\n", nb->left, nb->left_port, nb->right, nb->right_port);
}

int accept_client(struct sockaddr * addr, socklen_t * len, int *s_socket_fd, struct neighbor_info *my_nb, int *master_socket, struct player_info *my_info){
  //accept neighbor's connection as a server, use s_socket_fd.
  int neighbor_connection_fd = accept(*s_socket_fd, addr, len);
  if (neighbor_connection_fd == -1) {
    printf("Error: cannot accept connection on socket\n");
    exit(1);
  } //if
  //printf("Neighbor connection from player %d is ready.\n", my_nb->right);
  return neighbor_connection_fd;
}

int main(int argc, char *argv[])
{  
  if (argc != 3) {
    fprintf(stderr, "Wrong number of command line parameter\n");
    exit(1);
  }
  if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 65535)){
    fprintf(stderr, "Invalid port number\n");
    exit(1);
  }

  struct addrinfo host_info;
  struct addrinfo *p_host = &host_info;
  struct addrinfo *host_info_list;
  struct addrinfo **p_host_info_list = &host_info_list;
  const char *hostname = argv[1];
  const char *port     = argv[2];
  int status;
  int *p_status = &status;
  int socket_fd;
  int *p_socket_fd = &socket_fd;
  /* Connect Master */
  connect_master(p_status, p_socket_fd, p_host, p_host_info_list, hostname, port);
  
  /* Receiving player_info and neigh_info */  
  struct player_info my_info;
  receive_playerinfo(p_socket_fd, &my_info);
  struct neighbor_info my_nb;
  receive_neighinfo(p_socket_fd, &my_nb);


  /* Connect to neighbor */
  /* Yourself should be a server */
  /* Set up a Server and Listen */
  int s_status;
  int s_socket_fd;  //socket return value, int
  struct addrinfo s_host_info;
  struct addrinfo *s_host_info_list;
  char *s_hostname = NULL;
  char s_port[20];

  sprintf(s_port, "%d", my_nb.right_port);
  
  memset(&s_host_info, 0, sizeof(s_host_info));

  s_host_info.ai_family   = AF_UNSPEC;
  s_host_info.ai_socktype = SOCK_STREAM;
  s_host_info.ai_flags    = AI_PASSIVE;

  s_status = getaddrinfo(s_hostname, s_port, &s_host_info, &s_host_info_list);
  if (s_status != 0) {
    fprintf(stderr, "Error: cannot get address info for host\n");
    exit(1);
  } //if

  s_socket_fd = socket(s_host_info_list->ai_family, 
		     s_host_info_list->ai_socktype, 
		     s_host_info_list->ai_protocol);
  if (s_socket_fd == -1) {
    fprintf(stderr, "Error: cannot create socket\n");
    exit(1);
  } //if
  
  int s_yes = 1;
  s_status = setsockopt(s_socket_fd, SOL_SOCKET, SO_REUSEADDR, &s_yes, sizeof(int));
  s_status = bind(s_socket_fd, s_host_info_list->ai_addr, s_host_info_list->ai_addrlen);
  if (s_status == -1) {
    fprintf(stderr, "Error: cannot bind socket\n");
    exit(1);
  } //if

  s_status = listen(s_socket_fd, 100);
  if (s_status == -1) {
    fprintf(stderr, "Error: cannot listen on socket\n");
    exit(1);
  }  

  /* Definition for connect to left neighbor */
  struct addrinfo c_host_info;
  struct addrinfo *c_host = &c_host_info;
  struct addrinfo *c_host_info_list;
  struct addrinfo **pc_host_info_list = &c_host_info_list;
  char *c_hostname = my_nb.left_addr;
  char c_port[20];
  sprintf(c_port, "%d", my_nb.left_port);
  int c_status;
  int *pc_status = &c_status;
  int c_socket_fd;
  int *pc_socket_fd = &c_socket_fd;
  
  /* Definition for accept connection from right */
  struct sockaddr socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int neighbor_connection_fd;
  
  /* Connect to your left neighbor */
  if(my_info.player_number == 0){
    /* If you are player 0 */
    /* FIRST Accept first and hold on. */
    int id = my_info.player_number;
    send(socket_fd, &id, sizeof(int),0);
    //    printf("sent master I am ready\n");
    neighbor_connection_fd = accept_client(&socket_addr, &socket_addr_len, &s_socket_fd, &my_nb, p_socket_fd, &my_info);
    
    /* After receiving YES from master, connect to last player */
    int yes;
    recv(socket_fd, &yes, sizeof(int), 0);
    //printf("master told me %d is ready\n", yes);
    connect_neighbor(pc_status, pc_socket_fd, c_host, pc_host_info_list, c_hostname, c_port);
    
    /* when player0 finished, ring built up, send yes to master */
    char *buf="I am okay";
    send(socket_fd, buf, strlen(buf), 0);
    //printf("send master Finish ring!\n");
  }
  else{
    /* If you are not player 0 */
    /* FIRST Connect to your left neighbor */
    int Accing = 0;
    recv(socket_fd, &Accing, sizeof(int), 0);
    //printf("master told me %d is ready\n", Accing);
    connect_neighbor(pc_status, pc_socket_fd, c_host, pc_host_info_list, c_hostname, c_port);
    
    /* SECOND Accept request from right neighbor */
    int id = my_info.player_number;
    send(socket_fd, &id, sizeof(int), 0);
    //printf("send master I am ready\n");
    neighbor_connection_fd = accept_client(&socket_addr, &socket_addr_len, &s_socket_fd, &my_nb, p_socket_fd, &my_info);

  }

  fd_set master_set;    // master file descriptor list
  fd_set read_set;  // temp file descriptor list for select()
  int fdmax = 0;
  FD_ZERO(&master_set);    // clear the master and temp sets
  FD_ZERO(&read_set);

  FD_SET(c_socket_fd, &master_set);
  if(c_socket_fd>fdmax){
    fdmax = c_socket_fd;
  }
  FD_SET(socket_fd, &master_set);
  if(socket_fd>fdmax){
    fdmax = socket_fd;
  }
  FD_SET(neighbor_connection_fd, &master_set);
  if(neighbor_connection_fd>fdmax){
    fdmax = neighbor_connection_fd;
  }

  struct potato p;
  
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 500000;

  while(1){
    read_set = master_set; // copy it
    if(select(fdmax+1, &read_set, NULL, NULL, NULL) == -1){
      perror("select");
      exit(1);
    }
    // run through the existing connections looking for data to read
    for(int i = 0; i <= fdmax; i++){
      if(FD_ISSET(i, &read_set)){ // we got one.
	//printf("it is from %d\n", i);
	int nbytes;
	if((nbytes = recv(i, &p, sizeof(p), 0)) <= 0){
	  //printf("nbytes = %d", nbytes);
	  //printf("I am here close %d\n", i);
	  close(i); // bye!
	  FD_CLR(i, &master_set); // remove from master set
	}
	else{
	  // we got some data from a client
	  //printf("p.GameOver == %d\n", p.GameOver);
	  if(p.GameOver == 0){
	    //if NOT GameOver
	    p.hops = p.hops - 1;
	    p.trace[p.k] = my_info.player_number;
	    p.k = p.k + 1;
	    //printf("p.hops = %d, p.k = %d\n", p.hops, p.k);
	    if(p.hops == 0){
	      //send potato to master
	      send(socket_fd, &p, sizeof(p), 0);
	      //printf("sent master\n");
	      printf("I'm it\n");
	    }
	    else{
	      //send it out randomly to left or right
	      srand((unsigned int)time(NULL) + p.k);
	      if((rand()%2) == 0){
		printf("Sending potato to %d\n", my_nb.left);
		send(c_socket_fd, &p, sizeof(p), 0);
	      }
	      else{
		printf("Sending potato to %d\n", my_nb.right);
		send(neighbor_connection_fd, &p, sizeof(p), 0);
	      }
	    }
	    //	    printf("yes I sent it out\n");
	    break;
	  }
	  else{
	    //if GameOver 
	    //printf("before free\n");
	    freeaddrinfo(host_info_list);
	    //printf("before close\n");
	    close(s_socket_fd);
	    return 0;
	  }
	}
      }
    }
  }
  return 0;
}
