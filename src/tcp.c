//librerie standard
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// librerire per socket
/*
#include <winsock2.h>
#include <ws2tcpip.h>
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


// others
#include "../lib/argparse/argparse.h"


void client_tcp(const char* ip_addr, int port){
  int clientfd, result_code;
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);

  do{
    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    result_code = connect(clientfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(result_code == -1){
      perror("Error connecting to server.\n");
      sleep(1);
    }
  } while(result_code == -1);

  printf("Connected to %s:%d\n", ip_addr, port);

  close(clientfd); //! ATTENZIONE A WINDOWS
  printf("Connection closed.\n");
  exit(0);  
}

void server_tcp(int port){
  int socketfd, clientfd;
  struct sockaddr_in server_addr, client_addr;

  if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    printf("Error creating socket.\n");
    exit(-1);
  }
  printf("Socket created successfully.\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(socketfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
    printf("Error binding socket.\n");
    close(socketfd); //! ATTENZIONE A WINDOWS
    exit(-1);
  }
  
  if(listen(socketfd, 1) == -1){
    printf("Error listening to port.\n");
    close(socketfd); //! ATTENZIONE A WINDOWS
    exit(-1);
  }

  printf("Server is listening on port %d.\n", port);

  while(1){
    socklen_t client_addr_size = sizeof(client_addr);
    if((clientfd = accept(socketfd, (struct sockaddr*) &client_addr, &client_addr_size)) == -1){
      printf("Error accepting incoming connection.\n");
    }
    else{
      printf("Connection accepted from %s:%d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

     

      printf("Connection closed from %s:%d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
      close(clientfd); //! ATTENZIONE PER WINDOWS
    }
    
  }

}

static const char *const usages[] = {
  "bin/tcp [options] [[--] args]",
  "bin/tcp [options]",
  NULL
};

int main(int argc, const char **argv){
  int port = 12345;
  int listening_mode = 0;
  const char* host = NULL;

  signal(SIGCHLD, SIG_IGN);

  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("basic options"),
    OPT_BOOLEAN('l', "listen", &listening_mode, "run in listening mode (server)", NULL, 0, 0),
    OPT_INTEGER('p', "port", &port, "port of connection", NULL, 0, 0),
    OPT_STRING('H', "host", &host, "ip address", NULL, 0, 0),
    OPT_END()
  };
  
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);

  argc = argparse_parse(&argparse, argc, argv);

  // printf("%d", listening_mode);

  switch (listening_mode)
  {
  case 0: // client
    client_tcp(host, port);
    break;
  
  case 1: // server
    server_tcp(port);
    break;
  }


  printf("%d %d\n", listening_mode, port);

  return 0;
}