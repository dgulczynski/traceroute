#include "icmp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>

int is_valid_ipaddr(const char *ip_addr) {
    struct sockaddr_in temp;
    return inet_pton(AF_INET, ip_addr, &temp.sin_addr) != 0;
}

int receive_packets(int sockfd, uint16_t id, uint16_t seq, struct timeval time_until) {
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];
    char sender_ip_str[PACKETS_PER_TTL][16];
    float time_sum = 0.f;
    struct timeval time_left;
    struct timeval time_now;

    int i = 0;
    int host_reached = 0;

    while (i < PACKETS_PER_TTL && !gettimeofday(&time_now, NULL) && timercmp(&time_now, &time_until, <) == 1) {
        timersub(&time_until, &time_now, &time_left);
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time_left, sizeof(time_left)) < 0) {
            perror(strerror(errno));
            return EXIT_FAILURE;
        }

        ssize_t packet_len = recvfrom(sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr *)&sender, &sender_len);
        assert(gettimeofday(&time_now, NULL) == 0);

        if (packet_len < 0) {
            if (errno == EWOULDBLOCK) {
                printf(i == 0 ? "*\n" : "???\n");
            } else {
                fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
            }
            return host_reached;
        }
        assert(gettimeofday(&time_now, NULL) == 0);

        struct iphdr *ip_header = (struct iphdr *)buffer;
        inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str[i], 20U);

        struct icmphdr *icmp_header = (struct icmphdr *)(buffer + 4 * ip_header->ihl);

        if (icmp_header->type == ICMP_TIME_EXCEEDED) {
            ip_header = (struct iphdr *)(icmp_header + 1);
            icmp_header = (struct icmphdr *)((uint8_t *)ip_header + 4 * ip_header->ihl);
        } else if (icmp_header->type != ICMP_ECHOREPLY) {
            continue;
        }

        if (icmp_header->un.echo.id != id || icmp_header->un.echo.sequence != seq)
            continue;

        time_sum += (time_now.tv_sec - time_until.tv_sec + 1) * 1000 + (time_now.tv_usec - time_until.tv_usec) / 1000.f;

        if (icmp_header->type == ICMP_ECHOREPLY) {
            host_reached = 1;
        }

        int should_print_ip = 1;
        for (int j = 0; j < i; j++)
            should_print_ip = 1 && strcmp(sender_ip_str[i], sender_ip_str[j]);
        if (should_print_ip)
            printf("%-16s", sender_ip_str[i]);

        i++;
    }
    printf("%.0fms\n", time_sum / PACKETS_PER_TTL);
    return host_reached;
}

static u_int16_t compute_icmp_checksum(const void *buff, size_t length) {
    u_int32_t sum;
    const u_int16_t *ptr = buff;
    assert(length % 2 == 0);
    for (sum = 0; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16) + (sum & 0xffff);
    return (u_int16_t)(~(sum + (sum >> 16)));
}

ssize_t send_echo_packets(int sockfd, uint16_t id, uint16_t seq, int ttl, const char *ip_addr) {
    struct icmphdr header;
    header.type = ICMP_ECHO;
    header.code = 0;
    header.un.echo.id = id;
    header.un.echo.sequence = seq;
    header.checksum = 0;
    header.checksum = compute_icmp_checksum(&header, sizeof(header));

    struct sockaddr_in recipient;
    bzero(&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &recipient.sin_addr);

    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    ssize_t bytes_sent = 0;
    for (int i = 0; i < PACKETS_PER_TTL; i++)
        bytes_sent += sendto(sockfd, &header, sizeof(header), 0, (struct sockaddr *)&recipient, sizeof(recipient));
    return bytes_sent;
}