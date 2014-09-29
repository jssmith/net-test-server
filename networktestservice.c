#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sendpattern.h"

int response_id_seq = 1;

int next_response_id() {
  return response_id_seq++;
}

static const int32_t MAX_BURST_SIZE = 1000;
static const int32_t MAX_BURST_INTERVAL = 10000000;
static const int32_t MAX_NUM_BURSTS = 1000;

int send_sequence(const struct sockaddr_in* client_addr, const struct send_pattern* sp) {
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0) {
    perror("ERROR opening datagram socket");
    return -1;
  }

  int32_t packet_size = ntohl(sp->packet_size);
  int32_t burst_size = ntohl(sp->burst_size);
  int32_t burst_interval = ntohl(sp->burst_interval);
  int32_t num_bursts = ntohl(sp->num_bursts);


  // verify configuration
  if (packet_size < sizeof(struct response_label) || packet_size > 65507) {
    fprintf(stderr, "response size %d is out of bounds", packet_size);
    return -1;
  }
  if (burst_size < 1 || burst_size > MAX_BURST_SIZE) {
    fprintf(stderr, "burst size %d is out of bounds", burst_size);
    return -1;
  }
  if (burst_interval < 0 || burst_interval > MAX_BURST_INTERVAL) {
    fprintf(stderr, "burst interval %d is out of bounds", burst_interval);
    return -1;
  }
  if (num_bursts < 1 || num_bursts > MAX_NUM_BURSTS) {
    fprintf(stderr, "num bursts %d is out of bounds", burst_interval);
    return -1;
  }

  int response_id = next_response_id();
  char buffer[packet_size];
  memset(buffer, 0xCA, sizeof(buffer));
  struct response_label* rl = (struct response_label*) &buffer;
  rl->fingerprint = RESPONSE_FINGERPRINT;
  rl->response_id = response_id;

  int n;
  printf("start of send sequence %d\n", response_id);
  for (int b = 0; b < num_bursts; b++) {
    if (b > 0) {
      usleep(burst_interval);
    }
    rl->burst_id = b;
    for (int i = 0; i < burst_size; ++i) {
      rl->sequence_id = i;
      n = sendto(sockfd, buffer, packet_size, 0, (struct sockaddr*) client_addr, sizeof(*client_addr));
      if (n < 0) {
        perror("ERROR sending datagrams");
        exit(1);
      }
    }
  }
  printf("end of send sequence %d\n", response_id);
  return 0;
}

void describe_sequence(const struct send_pattern* sp) {
  printf("packet size: %d\nburst size: %d\nburst interval: %d\nnum bursts: %d\n",
    ntohl(sp->packet_size), ntohl(sp->burst_size),
    ntohl(sp->burst_interval), ntohl(sp->num_bursts));
}

int main(int argc, char* argv[]) {

  int sockfd, newsockfd;
  socklen_t clilen;
  int portno = 7231;

  struct sockaddr_in serv_addr, cli_addr, target_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }
  int optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  while (1) {
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
      perror("ERROR on accept");
      exit(1);
    }

    size_t buff_size = sizeof(struct send_pattern_request);
    char buffer[buff_size];
    memset(buffer, 0, buff_size);
    int n = recvfrom(newsockfd, buffer, buff_size, 0, NULL, NULL);
    if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
    }
    if (n < sizeof(struct send_pattern_request)) {
      fprintf(stderr, "too few bytes received - needed %d but got %d", (int) sizeof(struct send_pattern_request), n);
      exit(1);
    }
    struct send_pattern_request spr;
    memcpy(&spr, &buffer, sizeof(struct send_pattern_request));
    describe_sequence(&(spr.pattern));

    int32_t response_code;
    uint32_t return_port = ntohl(spr.port);
    if (return_port < 1 || return_port > 65535) {
      fprintf(stderr, "invalid return port specified %d", return_port);
      response_code = -2;
    } else {
      // Write network test data pattern
      memset((char *) &target_addr, 0, sizeof(target_addr));
      target_addr.sin_family = AF_INET;
      memcpy(&(target_addr.sin_addr), &(cli_addr.sin_addr), sizeof(cli_addr.sin_addr));
      target_addr.sin_port = htons((uint32_t) return_port);

      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &target_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
      printf("send test data to: %s:%d\n", client_ip, spr.port);

      response_code = send_sequence(&target_addr, &(spr.pattern));;
    }
    response_code = htonl(response_code);
    n = send(newsockfd, &response_code, sizeof(response_code), 0);
    if (n < 0) {
      perror("ERROR writing to socket");
      exit(1);
    }
    close(newsockfd);
  }

  return 0;
}
