#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


/**
 *This code is a packet sniffer that captures ICMP packets, specifically ICMP
 * echo request packets, and sends an ICMP echo reply packet in response. The
 * main function starts by finding all available network devices and printing
 * them to the console. The user is then prompted to select one of the devices
 * for sniffing. The selected device is then opened and a filter is applied to
 * capture only ICMP packets. The pcap_loop function is then used to
 * continuously capture packets and pass them to the process_packet function for
 * processing. The process_packet function checks if the captured packet is an
 * ICMP echo request packet, and if it is, it prints a message to the console
 * and creates an ICMP echo reply packet. The reply packet is then sent using
 * the send_spoof function. The program continues to capture and process packets
 * in a loop until the program is closed.
 */

/**
 * Calculates the checksum for the given data.
 * @param p_address Pointer to the data.
 * @param len Length of the data.
 * @return The calculated checksum.
 */
unsigned short checksum(unsigned short *p_address, int len);

/**
 * Sends a spoofed IP packet.
 * @param p_ip_header Pointer to the IP header of the packet to send.
 */
void send_spoof(struct iphdr *p_ip_header);

/**
 * Callback function for processing captured packets.
 * @param args User arguments (not used).
 * @param header Packet header.
 * @param buffer Packet data.
 */
void process_packet(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer);

int main() {

  char err_buf[PCAP_ERRBUF_SIZE], *device, devs[100][100], *filter = "icmp";
  struct bpf_program filter_exp;
  bpf_u_int32 net, mask;
  pcap_if_t *p_all_devs, *dev;
  pcap_t *handle;
  int count = 1, n;

  // First get the list of available devices
  printf("Finding available devices ... \n");
  if (pcap_findalldevs(&p_all_devs, err_buf)) {
    printf("Error finding devices : %s", err_buf);
    exit(1);
  }

  // Print the available devices
  printf("Available Devices:\n");
  for (dev = p_all_devs; dev != NULL; dev = dev->next) {
    printf("%d. %s - %s\n", count, dev->name, dev->description);
    if (dev->name != NULL) {
      strcpy(devs[count], dev->name);
    }
    count++;
  }

  // Ask user which device to sniff
  printf("Enter the number of the device you want to sniff : ");
  scanf("%d", &n);

  // Open the device for sniffing
  printf("\nOpening device for sniffing ... \n");

  device = devs[n];

  if (pcap_lookupnet(device, &net, &mask, err_buf) == -1) {
    mask = 0;
    net = 0;
  }

  handle = pcap_open_live(device, BUFSIZ, 1, 1000, err_buf);
  if (handle == NULL) {
    fprintf(stderr, "Error opening device %s: %s\n", device, err_buf);
    return -1;
  }

  if (pcap_compile(handle, &filter_exp, filter, 0, net) == -1) {
    fprintf(stderr, "Error opening device %s: %s\n", filter,
            pcap_geterr(handle));
    return -1;
  }

  if (pcap_setfilter(handle, &filter_exp) == -1) {
    fprintf(stderr, "Error compiling filter %s: %s\n", filter,
            pcap_geterr(handle));
    return -1;
  }

  pcap_loop(handle, -1, process_packet, NULL);
  pcap_close(handle);
  pcap_freecode(&filter_exp);

  return 0;
}

void process_packet(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer) {

  struct iphdr *p_ip_hdr = (struct iphdr *)(buffer + 14);
  struct icmphdr *p_icmp_hdr =
      (struct icmphdr *)(buffer + 14 + sizeof(struct iphdr));

  if (p_icmp_hdr->type == 8) {
    printf("Received ICMP Echo Request...\n");
    char pong[1500];
    memset(pong, 0, 1500);
    struct iphdr *ip_header = (struct iphdr *)(pong + 14);
    struct icmphdr *icmp_header =
        ((struct icmphdr *)(pong + 14 + sizeof(struct iphdr)));
    ip_header->daddr = p_ip_hdr->saddr;
    ip_header->saddr = p_ip_hdr->daddr;
    ip_header->ihl = p_ip_hdr->ihl;
    ip_header->check = p_ip_hdr->check;
    ip_header->id = p_ip_hdr->id;
    ip_header->version = p_ip_hdr->version;
    ip_header->frag_off = p_ip_hdr->frag_off;
    ip_header->frag_off = p_ip_hdr->frag_off;
    ip_header->version = p_ip_hdr->version;
    ip_header->protocol = p_ip_hdr->protocol;
    ip_header->tos = p_ip_hdr->tos;
    ip_header->ttl = p_ip_hdr->ttl;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));

    icmp_header->code = p_icmp_hdr->code;
    icmp_header->type = ICMP_ECHOREPLY;
    icmp_header->un.echo.id = p_icmp_hdr->un.echo.id;
    icmp_header->un.echo.sequence = p_icmp_hdr->un.echo.sequence;
    icmp_header->checksum =
        checksum((unsigned short *)icmp_header, sizeof(struct icmphdr));

    send_spoof(ip_header);
  }
}

void send_spoof(struct iphdr *p_ip_header) {
  struct sockaddr_in dest_info;
  int opt_val = 1;
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

  setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt_val, sizeof(opt_val));

  dest_info.sin_family = AF_INET;
  dest_info.sin_addr.s_addr = p_ip_header->daddr;

  if (sendto(sock, p_ip_header, ntohs(p_ip_header->tot_len), 0,
             (struct sockaddr *)&dest_info, sizeof(dest_info)) < 0) {
    perror("Error sending packet");
  } else {
    printf("Sent ICMP Echo Reply.\n");
  }
  close(sock);
}

// Function to calculate the checksum for an input buffer
unsigned short checksum(unsigned short *p_address, int len) {
  int i = len;
  int sum = 0;
  unsigned short *w = p_address;
  unsigned short answer = 0;

  while (i > 1) {
    sum += *w++;
    i -= 2;
  }
  if (i == 1) {
    *((unsigned char *)&answer) = *((unsigned char *)w);
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;

  return answer;
}