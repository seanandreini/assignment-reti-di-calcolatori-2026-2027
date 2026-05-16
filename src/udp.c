//librerie standard
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

// librerire per socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// others
#include "../lib/argparse/argparse.h"

#define MAX_MESSAGE_SIZE 1024
#define JAN_1970 2208988800UL 

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

void client_udp(const char* ip_addr, int port, int num_packets){
  int sockfd;
  struct sockaddr_in server_addr;
  struct packet packet;
  socklen_t addr_len = sizeof(server_addr);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Error creating socket.\n");
    exit(-1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);

  printf("Connected to %s:%d (UDP)\n", ip_addr, port);

  printf("Type message to send to server: \n");

  fgets(packet.message, sizeof(packet.message), stdin);
  packet.message[strcspn(packet.message, "\r\n")] = 0;

  for(int i=0; i<num_packets; i++){
    packet.ts = get_rfc8877_timestamp();
    if(sendto(sockfd, &packet, sizeof(struct packet), 0, (const struct sockaddr *) &server_addr, addr_len) == -1){
      perror("Error sending message to server.\n");
    }

    int n = recvfrom(sockfd, &packet, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &addr_len);
    if(n > 0){
      struct timeval now;
      gettimeofday(&now, NULL);
      
      double rtt = (double)(now.tv_sec + JAN_1970 - ntohl(packet.ts.seconds)) * 1000.0;
      double fraction_now = (double)now.tv_usec / 1000.0;
      double fraction_packet = ((double)ntohl(packet.ts.fraction) * 1000.0 / 4294967296ULL);

      rtt += (fraction_now - fraction_packet);

      printf("[UDP ECHO]: %s. RTT: %.3f\n", packet.message, rtt);
    }
  }

  close(sockfd);
  printf("Client closed.\n");
  exit(0);  
}

void server_udp(int port){
  int sockfd;
  struct sockaddr_in server_addr, client_addr;
  struct packet packet;
  socklen_t client_addr_size = sizeof(client_addr);

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    printf("Error creating socket.\n");
    exit(-1);
  }
  printf("UDP Socket created successfully.\n");

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(sockfd, (const struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
    printf("Error binding socket.\n");
    close(sockfd);
    exit(-1);
  }
  
  printf("UDP Server is listening on port %d.\n", port);

  while(1){
    int bytes_received = recvfrom(sockfd, &packet, sizeof(struct packet), 0, (struct sockaddr*) &client_addr, &client_addr_size);

    if(bytes_received < 0){
      printf("Error in receiving from client.\n");
    }
    else{
      printf("[UDP RECEIVED] from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), packet.message);

      if(sendto(sockfd, &packet, sizeof(struct packet), 0, (struct sockaddr*) &client_addr, client_addr_size) == -1){
        perror("Error sending echo.");
      }
    }
  }
}

static const char *const usages[] = {
  "bin/udp [options] [[--] args]",
  "bin/udp [options]",
  NULL
};

int main(int argc, const char **argv){
  int port = 12345;
  int listening_mode = 0;
  int num_packets = 1;
  const char* host = "127.0.0.1";

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

  if (listening_mode) {
    server_udp(port);
  } else {
    client_udp(host, port, num_packets);
  }

  return 0;
}
