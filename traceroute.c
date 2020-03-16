#include "icmp.h"

#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("Let's trace some routes!\n");

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (send_echo_packet(sockfd, 1, 1, "8.8.8.8"))
        receive_packet(sockfd);

    return EXIT_SUCCESS;
}
