#ifndef _SENDPATTERN_H
#define _SENDPATTERN_H

#include <sys/types.h>

// TODO should be network byte order
struct send_pattern {
  int32_t packet_size;
  int32_t burst_size;
  int32_t burst_interval;
  int32_t num_bursts;
};

struct send_pattern_request {
  uint16_t port;
  struct send_pattern pattern;
};

static const int32_t RESPONSE_FINGERPRINT = 0xf334ffca;

struct response_label {
  int32_t fingerprint;
  int32_t response_id;
  int32_t burst_id;
  int32_t sequence_id;
};

#endif
