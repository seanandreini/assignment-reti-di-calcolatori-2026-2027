//librerie standard
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

#define MAX_MESSAGE_SIZE 1024
#define JAN_1970 2208988800UL // detto da gemini

struct ntp_timestamp{
  uint32_t seconds;
  uint32_t fraction;
};

//* FATTO CON GEMINI
struct ntp_timestamp get_rfc8877_timestamp() {
  struct timeval tv;
  struct ntp_timestamp ts;

  gettimeofday(&tv, NULL);

  ts.seconds = htonl(tv.tv_sec + JAN_1970);

  uint64_t frac = (uint64_t)tv.tv_usec * 4294967296ULL / 1000000ULL;
  ts.fraction = htonl((uint32_t)frac);

  return ts;
};

struct packet{
  struct ntp_timestamp ts;
  char message[MAX_MESSAGE_SIZE];
};

void client_tcp(const char* ip_addr, int port, int num_packets){
  int clientfd, result_code;
  struct sockaddr_in server_addr;
  struct packet packet;

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

  printf("Type message to send to server: \n");

  fgets(packet.message, sizeof(packet.message), stdin);
  packet.message[strcspn(packet.message, "\r\n")] = 0;

  for(int i=0; i<num_packets; i++){
    packet.ts = get_rfc8877_timestamp();
    if(send(clientfd, &packet, sizeof(struct packet), 0) == -1){
      perror("Error sending message to server.\n");
    }

    int n = recv(clientfd, &packet, sizeof(struct packet), 0);
    if(n > 0){
      struct timeval now;
      gettimeofday(&now, NULL);
      
      //* fatto con gemini
      double rtt = (double)(now.tv_sec + JAN_1970 - ntohl(packet.ts.seconds)) * 1000.0;

      // 2. Differenza tra le frazioni (convertendo i microsecondi di 'now' in millisecondi)
      double fraction_now = (double)now.tv_usec / 1000.0;
      double fraction_packet = ((double)ntohl(packet.ts.fraction) * 1000.0 / 4294967296ULL);

      rtt += (fraction_now - fraction_packet);

      printf("[ECHO]: %s. RTT: %.3f\n", packet.message, rtt);
    }
  }

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

      while(1){
        struct packet packet;
        int bytes_received = recv(clientfd, &packet, sizeof(struct packet), 0);

        if(bytes_received < 0){
          printf("Error in receiveng from client.\n");
        }
        else if(bytes_received == 0){
          break;
        }
        else{
          printf("[RECEIVED]: %s\n", packet.message);

          if(send(clientfd, &packet, sizeof(struct packet), 0) == -1){
            perror("Error sending echo.");
          }
        }
      }

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
  int num_packets = 1;
  const char* host = NULL;

  signal(SIGCHLD, SIG_IGN);

  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("basic options"),
    OPT_BOOLEAN('l', "listen", &listening_mode, "run in listening mode (server)", NULL, 0, 0),
    OPT_INTEGER('p', "port", &port, "port of connection", NULL, 0, 0),
    OPT_INTEGER('n', "num_packets", &num_packets, "number of packets to send", NULL, 0, 0),
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
    client_tcp(host, port, num_packets);
    break;
  
  case 1: // server
    server_tcp(port);
    break;
  }


  printf("%d %d\n", listening_mode, port);

  return 0;
}