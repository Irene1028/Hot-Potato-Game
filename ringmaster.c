/*
ECE 650 Hw03 Hot Potato

This file contains the code about ringmaster

Author: Irene
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
#include <ctype.h>
#include <arpa/inet.h>

#include "potato.h"

int main(int argc, char *argv[])
{
  if(argc != 4){
    fprintf(stderr, "Wrong number of command line parameter!\n");
    exit(1);
  }
  if((atoi(argv[1]) < 0) || (atoi(argv[1]) > 65535)){
    fprintf(stderr, "Invalid port number\n");
    exit(1);
  }
  if(atoi(argv[2]) <= 1){
    fprintf(stderr, "Players_num must be greater than 1\n");
    exit(1);
  }
  if((atoi(argv[3]) < 0) || (atoi(argv[3]) > 512)){
    fprintf(stderr, "Num_hops must be in the range [0-512]\n");
    exit(1);
  }
  
  int status;
  int socket_fd;  //socket return value, int
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = argv[1];
  const int num_players = atoi(argv[2]);
  int hops = atoi(argv[3]);
  int port_number = atoi(argv[1]);
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    fprintf(stderr, "Error: cannot get address info for host\n");
    exit(1);
  } //if

  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    fprintf(stderr, "Error: cannot create socket\n");
    exit(1);
  } //if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    fprintf(stderr, "Error: cannot bind socket\n");
    exit(1);
  } //if

  status = listen(socket_fd, 100);
  if (status == -1) {
    fprintf(stderr, "Error: cannot listen on socket\n");
    exit(1);
  } //if
  
  /* Stop here, begin waiting for players */

  printf("Potato Ringmaster\n");
  printf("Players = %d\n", num_players);
  printf("Hops = %d\n", hops);

  struct player_info info[num_players];
  struct sockaddr socket_addr[num_players];
  socklen_t socket_len[num_players];
  for(int i = 0; i<num_players; i++){
    socket_len[i]=sizeof(socket_addr[i]);
  }
  for(int i = 0; i<num_players; i++){
    // get this player's addr, socket, addr_len;
    memset(&info[i], 0, sizeof(info[i]));//make sure the struct is empty 
    info[i].socket = accept(socket_fd, &socket_addr[i], &socket_len[i]);
    if (info[i].socket == -1) {
      fprintf(stderr, "Error: cannot accept connection on socket\n");
      exit(1);
    } //if
    info[i].player_number = i;
    info[i].num_players = num_players;
    for(int k = 0; k<256; k++){
      info[i].addr[k] = 0;
    }
    int len = 0;
    recv(info[i].socket, &len, sizeof(int), 0);
    //printf("len is %d\n", len);
    if(recv(info[i].socket, info[i].addr, len, 0)== 0){
      printf("the remote closed connection with you\n");
      exit(1);
    }
    printf("Player %d is ready to play\n", info[i].player_number);

    /* transmit player number to player after connection */
    send(info[i].socket, &info[i], sizeof(info[i]), 0);
  }
  
  /* Set neighbor info here */
  struct neighbor_info neigh[num_players];
  for(int i = 0; i<num_players; i++){
    //Initialization
    for(int k = 0; k < 256; k++){
      neigh[i].left_addr[k] = 0;
      neigh[i].right_addr[k] = 0;
    }
    srand((unsigned int)time(NULL)+info[i].player_number);
    if(i == 0){
      int left = num_players-1;
      neigh[i].left = left;
      memcpy(neigh[i].left_addr,info[left].addr,strlen(info[left].addr));
      neigh[i].left_port = rand()%40000+2000;
      if(neigh[i].left_port == port_number){
	neigh[i].left_port ++;
      }
      info[i].neigh = &neigh[i];
      //printf("player %d left_addr is %s\n", i, neigh[i].left_addr);
    }
    else{
      int left = info[i].player_number - 1;
      neigh[i].left = left;
      memcpy(neigh[i].left_addr,info[left].addr,strlen(info[left].addr));
      neigh[i].left_port = rand()%40000+2000;
      if(neigh[i].left_port == port_number){
	neigh[i].left_port ++;
      }
      info[i].neigh = &neigh[i];
      //printf("player %d left_addr is %s\n", i, neigh[i].left_addr);
    }
  }
  for(int i = 0; i<num_players; i++){
    if(i == num_players -1){
      int right = 0;
      neigh[i].right = right;
      memcpy(neigh[i].right_addr, info[right].addr, strlen(info[right].addr));
      neigh[i].right_port = neigh[right].left_port;
      //printf("player %d right_addr is %s\n", i, neigh[i].right_addr);
    }
    else{
      int right = info[i].player_number + 1;
      neigh[i].right = right;
      memcpy(neigh[i].right_addr, info[right].addr, strlen(info[right].addr));
      neigh[i].right_port = neigh[right].left_port;
      //printf("player %d right_addr is %s\n", i, neigh[i].right_addr);
    }
  }

  
  /* Send neighbor info to players */
  for(int i = 0; i<num_players; i++){
    send(info[i].socket, &neigh[i], sizeof(neigh[i]), 0);
  }
  for(int i = 0; i<num_players; i++){
    int Accing = 0;
    recv(info[i].socket, &Accing, sizeof(int), 0);
    //printf("receive %d is ready\n", Accing);
    if(i == num_players-1){
      send(info[0].socket, &Accing, sizeof(int), 0);
    }
    else{
      send(info[i+1].socket, &Accing, sizeof(int), 0);
    }
  }
    

  char NReady[9];  
  if(recv(info[0].socket, NReady, 9, 0)== -1){
    //printf("receiv\n");
    exit(1);
  }
  //printf("Check if step by step: %s\n", NReady);
  //printf("Ring built up from player 0\n");
  /* prepare potato */
  struct potato p;
  memset(&p, 0, sizeof(p));
  p.hops = hops;
  p.k = 0;
  p.GameOver = 0;

  /* sending potato to player X */
  if(hops == 0){
    p.GameOver = 1;
    for(int k = 0; k < num_players; k++){
      send(info[k].socket, &p, sizeof(p), 0);
      close(info[k].socket);
    }
    //printf("hops == 0, GameOver\n");
    return 0;
  }
  else{
    srand((unsigned int)time(NULL));
    int first_player = rand()%num_players;
    send(info[first_player].socket, &p, sizeof(p), 0);
    printf("Ready to start the game, sending potato to player %d\n", first_player);
  }
  /* waiting for ending potato */
  fd_set master_set;    // master file descriptor list
  fd_set read_set;  // temp file descriptor list for select()
  int fdmax = 0;
  FD_ZERO(&master_set);    // clear the master and temp sets
  FD_ZERO(&read_set);

  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 500000;
  
  for(int i = 0; i<num_players; i++){ 
    FD_SET(info[i].socket, &master_set);
    if(info[i].socket > fdmax){
      fdmax = info[i].socket;
    }
  }
  for(;;){
    read_set = master_set; // copy it
    if(select(fdmax+1, &read_set, NULL, NULL, NULL) == -1){
      perror("select");
      exit(1);
    }
    for(int i = 0; i <= fdmax; i++){
      if(FD_ISSET(i, &read_set)){ // we got one.
	int nbytes;
	if((nbytes = recv(i, &p, sizeof(p), 0)) <= 0){
	  // got error or connection closed by client
	  close(i); // bye!
	  FD_CLR(i, &master_set); // remove from master set
	}
	else{
	  // we got some data from a client
	  if(p.hops == 0){
	    //change GameOver
	    p.GameOver = 1;
	    //send potato to master
	    for(int k = 0; k < num_players; k++){
	      send(info[k].socket, &p, sizeof(p), 0);
	      close(info[k].socket);
	    }
	    //print trace
	    printf("Trace of potato:\n");
	    for(int k = 0; k < p.k; k++){
	      if(k == p.k - 1){
		printf("%d\n", p.trace[k]);
	      }
	      else{
		printf("%d,", p.trace[k]);
	      }
	    }
	    //printf("Game Over, Good Night.\n");
	    freeaddrinfo(host_info_list);
	    return 0; 
	  }
	  else{
	    //add name to trace
	    printf("Wrong potato\n");
	    return 0;
	  }
	}
      }
    }
  }
  return 0;
}
