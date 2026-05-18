//librerie standard
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// socket libraries (different for windows and unix)
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define CLOSE_SOCKET(s) closesocket(s)
  #define SLEEP(s) Sleep((s)*1000)
#else
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netinet/tcp.h>
  #include <unistd.h>
  typedef int socket_t;
  #define INVALID_SOCKET -1
  #define CLOSE_SOCKET(s) close(s)
  #define SLEEP(s) sleep(s)
#endif

// others
#include "../lib/argparse/argparse.h"

#define MAX_MESSAGE_SIZE 1024
#define JAN_1970 2208988800UL // detto da intelligenza artificiale

#ifdef _WIN32
// uso intelligenza artificiale per implementare gettimeofday su windows, che non ce l'ha
int gettimeofday(struct timeval* tv, void* tz) {
  static LARGE_INTEGER freq = {0};
  static LARGE_INTEGER start_qpc = {0};
  static uint64_t start_epoch_us = 0;

  if (freq.QuadPart == 0) {
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start_qpc);
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    t -= 116444736000000000ULL;
    start_epoch_us = t / 10;
  }

  LARGE_INTEGER now_qpc;
  QueryPerformanceCounter(&now_qpc);
  uint64_t elapsed_us = (now_qpc.QuadPart - start_qpc.QuadPart) * 1000000ULL / freq.QuadPart;
  uint64_t now_us = start_epoch_us + elapsed_us;

  tv->tv_sec  = (long)(now_us / 1000000);
  tv->tv_usec = (long)(now_us % 1000000);
  return 0;
}
#endif

// struct for timestamp of packet
struct ntp_timestamp{
  uint32_t seconds;
  uint32_t fraction;
};

// rfc8877 timestamp (fatto con intelligenza artificiale)
struct ntp_timestamp get_rfc8877_timestamp() {
  struct timeval tv;
  struct ntp_timestamp ts;

  gettimeofday(&tv, NULL);

  ts.seconds = htonl(tv.tv_sec + JAN_1970);

  uint64_t frac = (uint64_t)tv.tv_usec * 4294967296ULL / 1000000ULL;
  ts.fraction = htonl((uint32_t)frac);

  return ts;
};

// struct for packet
struct packet{
  struct ntp_timestamp ts;
  char message[MAX_MESSAGE_SIZE];
};

// tcp client function
void client_tcp(const char* ip_addr, int port, int num_packets, int nonagle){
  socket_t clientfd = INVALID_SOCKET;
  int result_code;
  struct sockaddr_in server_addr;
  struct packet packet;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);

  do{
    if (clientfd != INVALID_SOCKET) CLOSE_SOCKET(clientfd);
    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    result_code = connect(clientfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(result_code == -1){
      perror("Error connecting to server.\n");
      SLEEP(1);
    }
  } while(result_code == -1);

  setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &nonagle, sizeof(nonagle));

  printf("Connected to %s:%d\n (TCP)", ip_addr, port);

  printf("Type message to send to server: \n");

  fgets(packet.message, sizeof(packet.message), stdin);
  packet.message[strcspn(packet.message, "\r\n")] = 0;

  for(int i=0; i<num_packets; i++){
    packet.ts = get_rfc8877_timestamp();
    if(send(clientfd, &packet, sizeof(struct packet), 0) == -1){
      perror("Error sending message to server.\n");
    }

    struct timeval timeout = {2, 0};
    struct packet rec_packet;
    setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int n = recv(clientfd, &rec_packet, sizeof(struct packet), 0);
    
    if(n > 0){
      struct timeval now;
      gettimeofday(&now, NULL);

      double t_sent = (double)ntohl(rec_packet.ts.seconds) 
                    + (double)ntohl(rec_packet.ts.fraction) / 4294967296.0;
      double t_now  = (double)(now.tv_sec + JAN_1970) 
                    + (double)now.tv_usec / 1e6;

      double rtt = (t_now - t_sent) * 1000.0; // millisecondi
      printf("%s. RTT: %.3f ms\n", rec_packet.message, rtt);
    }
  }

  CLOSE_SOCKET(clientfd);
  printf("Connection closed.\n");
  exit(0);  
}

// udp client function
void client_udp(const char* ip_addr, int port, int num_packets){
  socket_t sockfd;
  struct sockaddr_in server_addr;
  struct packet packet;
  socklen_t addr_len = sizeof(server_addr);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Error creating socket.\n");
    exit(-1);
  }

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

    struct packet rec_packet;
    struct timeval timeout = {2, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int n = recvfrom(sockfd, &rec_packet, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &addr_len);
    if(n > 0){
      struct timeval now;
      gettimeofday(&now, NULL);

      double t_sent = (double)ntohl(rec_packet.ts.seconds) 
                    + (double)ntohl(rec_packet.ts.fraction) / 4294967296.0;
      double t_now  = (double)(now.tv_sec + JAN_1970) 
                    + (double)now.tv_usec / 1e6;

      double rtt = (t_now - t_sent) * 1000.0; // millisecondi
      printf("%s. RTT: %.3f ms\n", rec_packet.message, rtt);
    }
  }

  CLOSE_SOCKET(sockfd);
  printf("Client closed.\n");
  exit(0);  
}

void server_udp(int port){
  socket_t sockfd;
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
    CLOSE_SOCKET(sockfd);
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

      char original[MAX_MESSAGE_SIZE]="";
      strncpy(original, packet.message, MAX_MESSAGE_SIZE);
      snprintf(packet.message, MAX_MESSAGE_SIZE, "[SERVER ECHO] %s", original);
      if(sendto(sockfd, &packet, sizeof(struct packet), 0, (struct sockaddr*) &client_addr, client_addr_size) == -1){
        perror("Error sending echo.");
      }
    }
  }
}

// tcp server function
void server_tcp(int port){
  socket_t socketfd, clientfd;
  struct sockaddr_in server_addr, client_addr;

  if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    printf("Error creating socket.\n");
    exit(-1);
  }
  printf("TCP Socket created successfully.\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(socketfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
    printf("Error binding socket.\n");
    CLOSE_SOCKET(socketfd);
    exit(-1);
  }
  
  if(listen(socketfd, 1) == -1){
    printf("Error listening to port.\n");
    CLOSE_SOCKET(socketfd);
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
          printf("[TCP RECEIVED]: %s\n", packet.message);

          char original[MAX_MESSAGE_SIZE];
          strncpy(original, packet.message, MAX_MESSAGE_SIZE);
          snprintf(packet.message, MAX_MESSAGE_SIZE, "[SERVER ECHO] %s", original);
          if(send(clientfd, &packet, sizeof(struct packet), 0) == -1){
            perror("Error sending echo.");
          }
        }
      }

      printf("Connection closed from %s:%d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
      CLOSE_SOCKET(clientfd);
    } 
  }
}

// options for argparse library
static const char *const usages[] = {
  "bin/main [options] [[--] args]",
  "bin/main [options]",
  NULL
};

int main(int argc, const char **argv){
  // windows socket initialization
  #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
      printf("WSAStartup failed.\n");
      return -1;
    }
  #endif

  int port = 12345;
  int listening_mode = 0;
  int udp_mode = 0;
  int num_packets = 1;
  int nonagle = 0;
  const char* host = NULL;

  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("basic options"),
    OPT_BOOLEAN('l', "listen", &listening_mode, "run in listening mode (server)", NULL, 0, 0),
    OPT_BOOLEAN('u', "udp", &udp_mode, "run in UDP mode", NULL, 0, 0),
    OPT_BOOLEAN('N', "nonagle", &nonagle, "disabilitate nagle's algorithm", NULL, 0, 0),
    OPT_INTEGER('p', "port", &port, "port of connection", NULL, 0, 0),
    OPT_INTEGER('n', "num_packets", &num_packets, "number of packets to send", NULL, 0, 0),
    OPT_STRING('H', "host", &host, "ip address", NULL, 0, 0),
    OPT_END()
  };
  
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argc = argparse_parse(&argparse, argc, argv);

  if(listening_mode){
    if(udp_mode){
      server_udp(port);
    }
    else{
      server_tcp(port);
    }
  }
  else{
    if(host == NULL){
      printf("Host is required in client mode.\n");
      exit(-1);
    }

    if(udp_mode){
      client_udp(host, port, num_packets);
    }
    else{
      client_tcp(host, port, num_packets, nonagle);
    }
  }

  // windows socket cleanup
  #ifdef _WIN32
    WSACleanup();
  #endif
  return 0;
}