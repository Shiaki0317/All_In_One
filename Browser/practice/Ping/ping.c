#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

struct __attribute__((packed)) ICMPMessage {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
};

uint16_t CalcChecksum(void *buf, size_t start, size_t end) {
  // https://tools.ietf.org/html/rfc1071
  uint16_t *p = buf;
  uint32_t sum = 0;
  for (size_t i = start; i < end; i+=2) {
    sum += ((uint16_t)p[i + 0]) << 8 | p[i+1];
  }
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  sum = -sum;
  return ((sum >> 8) & 0xff) | ((sum & 0xff) << 8);
}

int main(int argc, char **argv) {
  char *ping_target_ip_addr;
  struct sockaddr_in addr;
  int soc;
  struct ICMPMessage icmp;
  int n;
  uint8_t recv_buf[256];
  socklen_t addr_size;
  int recv_len;
  struct ICMPMessage* recv_icmp;

  if (argc != 2) {
    printf("Usage: %s <ip addr>\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  ping_target_ip_addr = argv[1];
  printf("Ping to %s...\n", ping_target_ip_addr);

  // Create ICMP socket
  // https://lwn.net/Articles/422330/
  soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (soc < 0) {
    perror("socket() failed\n");
    exit(EXIT_FAILURE);
  }

  // Create sockaddr_in
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ping_target_ip_addr);

  // Send ICMP
  memset(&icmp, 0, sizeof(icmp));
  icmp.type = 8; /* Echo Request */
  icmp.checksum = CalcChecksum(&icmp, 0, sizeof(icmp));
  n = sendto(soc, &icmp, sizeof(icmp), 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n < 1) {
    perror("sendto() failed\n");
    exit(EXIT_FAILURE);
  }

  // Recieve reply
  recv_len = recvfrom(soc, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr, &addr_size);
  if (recv_len < 1) {
    perror("recvfrom() failed\n");
    exit(EXIT_FAILURE);
  }

  printf("recvfrom returned: %d\n", recv_len);
  for (int i = 0; i < recv_len; i++) {
    printf("%2x ", recv_buf[i]);
    printf((i & 0xF) == 0xF ? "\n" : " ");
  }
  printf("\n");

  recv_icmp = (struct ICMPMessage*)(recv_buf);
  printf("ICMP packet recieved from %s ICMP Type = %d\n", ping_target_ip_addr, recv_icmp->type);

  close(soc);

  return 0;
}