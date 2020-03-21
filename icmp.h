#include <inttypes.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netinet/ip.h>

#define PACKETS_PER_TTL 3
#define SEC_FOR_ANSWER 1

// return value: 1 on host reached, -1 on error, 0 otherwise
int receive_packets(int sockfd, uint16_t id, uint16_t seq, struct timeval time_until);

// return value: -1 on error, 0 otherwise
int send_echo_packets(int sockfd, uint16_t id, uint16_t seq, int ttl, struct sockaddr *recipient);
