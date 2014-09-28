#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sendpattern.h"

int main(int argc, char* argv[]) {
  int portno = 7231;
  struct sockaddr_in serv_addr;
  struct send_pattern_request spr;
  int n;

  spr.port = 2389;
  spr.pattern.packet_size = 1500;
  spr.pattern.burst_size = 30;
  spr.pattern.burst_interval = 1000000;
  spr.pattern.num_bursts = 5;

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);

  if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
    printf("\n inet_pton error occured\n");
    exit(1);
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR Connect Failed");
    exit(1);
  }

  n = send(sockfd, &spr, sizeof(spr), 0);
  if (n < 0) {
    perror("ERROR sending");
    exit(1);
  }

  // get the response code
  int32_t response_code;
  n = recvfrom(sockfd, &response_code, sizeof(response_code), 0, NULL, NULL);
  if (n < 0) {
    perror("ERROR receiving");
    exit(1);
  }

  printf("received response code %d\n", response_code);

  return 0;
}