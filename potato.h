/*
ECE 650 Hw03 Hot Potato

This file contains the code about potato structure

Author: Irene
Date: 02/15/2019
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>

struct player_info{
  int player_number;
  int num_players;
  int socket;
  char addr[256];
  struct neighbor_info *neigh;
};

struct neighbor_info{
  int left;
  int right;
  char left_addr[256];
  char right_addr[256];
  int left_port;
  int right_port;
};

struct potato{
  int hops;
  int trace[512];
  int k;
  int GameOver;
};
