#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/**
 * This program captures and analyzes TCP packets on a network.
 * It uses the pcap library to capture packets from a network device.
 * The main function starts by finding all available network devices and
 * printing them to the console. The user is then prompted to select one of the
 * devices for sniffing. The selected device is then opened and a pcap_loop
 * function is used to continuously capture packets and pass them to the
 * process_packet function for processing. The process_packet function checks
 * the IP protocol of the captured packet, and if it is TCP, it calls the
 * got_packet function to process the packet. The got_packet function prints
 * data from the packet to a text file and to the console. The program continues
 * to capture and process packets in a loop until the program is closed.
 */

/**
 * @brief Structure representing a TCP header.
 */
struct tcp_hdr {
  u_int16_t source;
  u_int16_t dest;
  u_int32_t seq;
  u_int32_t ack_seq;
  u_int16_t doff : 4;
  u_int8_t cwr : 1;
  u_int8_t ece : 1;
  u_int8_t urg : 1;
  u_int8_t ack : 1;
  u_int8_t psh : 1;
};

/**
 * @brief Callback function for processing captured packets.
 * @param args User arguments (not used).
 * @param header Packet header.
 * @param buffer Packet data.
 */
void process_packet(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer);

/**
 * @brief Processes a captured packet, extracting and printing TCP header
 * information and data.
 * @param buffer The raw packet buffer.
 * @param size The total size of the captured packet.
 */
void got_packet(const u_char *buffer, int size);

/**
 * @brief Prints data in hex and ASCII format to the output file.
 * @param data Pointer to the data buffer.
 * @param size The size of the data to print.
 */
void print_data(const u_char *data, int size);

/**
 * @brief Prints the IP header details to the output file.
 * @param buffer The raw packet buffer containing the IP header.
 */
void print_ip_header(const u_char *buffer);

/**
 * @brief Prints the Ethernet header details to the output file.
 * @param buffer The raw packet buffer containing the Ethernet header.
 */
void print_ethernet_header(const u_char *buffer);

FILE *p_file;
struct sockaddr_in source, dest;
int tcp = 0, i, j;

int main() {
  pcap_if_t *p_all_devs, *device;
  pcap_t *handle;

  char err_buf[100], *p_dev_name, devs[100][100];
  int count = 1, n;

  // First get the list of available devices
  printf("Finding available devices ... \n");
  if (pcap_findalldevs(&p_all_devs, err_buf)) {
    printf("Error finding devices : %s", err_buf);
    exit(1);
  }

  // Print the available devices
  printf("Available Devices:\n");
  for (device = p_all_devs; device != NULL; device = device->next) {
    printf("%d. %s - %s\n", count, device->name, device->description);
    if (device->name != NULL) {
      strcpy(devs[count], device->name);
    }
    count++;
  }

  // Ask user which device to sniff
  printf("Enter the number of the device you want to sniff : ");
  scanf("%d", &n);
  p_dev_name = devs[n];

  // Open the device for sniffing
  printf("Opening device %s for sniffing ... ", p_dev_name);
  handle = pcap_open_live(p_dev_name, 65536, 1, 0, err_buf);

  if (handle == NULL) {
    fprintf(stderr, "Couldn't open device %s : %s\n", p_dev_name, err_buf);
    exit(1);
  }
  printf("Done\n");

  p_file = fopen("211696521_928596978.txt", "w");
  if (p_file == NULL) {
    printf("Unable to create file.");
  }

  // Put the device in sniff loop
  pcap_loop(handle, -1, process_packet, NULL);

  return 0;
}

void process_packet(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer) {
  int size = header->len;

  // Get the IP Header part of this packet , excluding the ethernet header
  struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

  if (iph->protocol == 6) // Check the Protocol
  {
    tcp++;
    got_packet(buffer, size);
  }
  printf("TCP : %d \r", tcp);
}

void print_ethernet_header(const u_char *buffer) {
  struct ethhdr *eth = (struct ethhdr *)buffer;

  fprintf(p_file, "\n");
  fprintf(p_file, "Ethernet Header\n");
  fprintf(p_file, "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n",
          eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3],
          eth->h_dest[4], eth->h_dest[5]);
  fprintf(p_file, "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n",
          eth->h_source[0], eth->h_source[1], eth->h_source[2],
          eth->h_source[3], eth->h_source[4], eth->h_source[5]);
  fprintf(p_file, "   |-Protocol            : %u \n",
          (unsigned short)eth->h_proto);
}

void print_ip_header(const u_char *buffer) {
  print_ethernet_header(buffer);

  struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

  memset(&source, 0, sizeof(source));
  source.sin_addr.s_addr = iph->saddr;

  memset(&dest, 0, sizeof(dest));
  dest.sin_addr.s_addr = iph->daddr;

  fprintf(p_file, "\n");
  fprintf(p_file, "IP Header\n");
  fprintf(p_file, "   |-IP Version        : %d\n", (unsigned int)iph->version);
  fprintf(p_file, "   |-IP Header Length  : %d Bytes\n",
          ((unsigned int)(iph->ihl)) * 4);
  fprintf(p_file, "   |-TTL               : %d\n", (unsigned int)iph->ttl);
  fprintf(p_file, "   |-Protocol          : %d\n", (unsigned int)iph->protocol);
  fprintf(p_file, "   |-Total Length      : %d\n", ntohs(iph->tot_len));
  fprintf(p_file, "   |-Source IP         : %s\n", inet_ntoa(source.sin_addr));
  fprintf(p_file, "   |-Destination IP    : %s\n", inet_ntoa(dest.sin_addr));
}

void got_packet(const u_char *buffer, int size) {
  unsigned short ip_hdr_len;

  struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
  ip_hdr_len = iph->ihl * 4;

  struct tcp_hdr *p_tcp_hdr =
      (struct tcp_hdr *)(buffer + ip_hdr_len + sizeof(struct ethhdr));

  int header_size = sizeof(struct ethhdr) + ip_hdr_len + p_tcp_hdr->doff * 4;

  // Extract source and destination IP addresses, source and destination ports
  memset(&source, 0, sizeof(source));
  source.sin_addr.s_addr = iph->saddr;
  memset(&dest, 0, sizeof(dest));
  dest.sin_addr.s_addr = iph->daddr;

  // Extract timestamp
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char time_buffer[30];
  strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
  char sec[7];
  printf(sec, ".%06ld", tv.tv_usec);
  strcat(time_buffer, sec);

  fprintf(p_file,
          "\n\n***********************TCP Packet*************************\n");

  print_ip_header(buffer);

  fprintf(p_file, "\n");
  fprintf(p_file, "TCP Header\n");
  fprintf(p_file, "   |-Source Port          : %u\n", ntohs(p_tcp_hdr->source));
  fprintf(p_file, "   |-Destination Port     : %u\n", ntohs(p_tcp_hdr->dest));
  fprintf(p_file, "   |-Sequence Number      : %u\n", ntohl(p_tcp_hdr->seq));
  fprintf(p_file, "   |-Acknowledge Number   : %u\n",
          ntohl(p_tcp_hdr->ack_seq));
  fprintf(p_file, "   |-Header Length        : %d BYTES\n",
          (unsigned int)p_tcp_hdr->doff * 4);
  fprintf(p_file, "   |-Cache Control        : %d\n",
          (unsigned int)p_tcp_hdr->psh);
  fprintf(p_file, "   |-Timestamp            : %s\n", time_buffer);
  fprintf(p_file, "   |-Cache Flag           : %d\n",
          (unsigned int)p_tcp_hdr->cwr);
  fprintf(p_file, "   |-Steps Flag           : %d\n",
          (unsigned int)p_tcp_hdr->ece);
  fprintf(p_file, "   |-Type Flag            : %d\n",
          (unsigned int)p_tcp_hdr->urg);
  fprintf(p_file, "   |-Status Code          : %d\n",
          (unsigned int)p_tcp_hdr->ack);
  fprintf(p_file, "\n");
  fprintf(p_file, "                        DATA Dump                         ");
  fprintf(p_file, "\n");

  fprintf(p_file, "IP Header\n");
  print_data(buffer, ip_hdr_len);

  fprintf(p_file, "TCP Header\n");
  print_data(buffer + ip_hdr_len, p_tcp_hdr->doff * 4);

  fprintf(p_file, "Data Payload\n");
  print_data(buffer + header_size, size - header_size);

  fprintf(p_file,
          "\n###########################################################");
}

void print_data(const u_char *data, int size) {
  for (i = 0; i < size; i++) {
    if (i != 0 && i % 16 == 0) {
      fprintf(p_file, "         ");
      for (j = i - 16; j < i; j++) {
        if (data[j] >= 32 && data[j] <= 128)
          fprintf(p_file, "%c", (unsigned char)data[j]);

        else
          fprintf(p_file, ".");
      }
      fprintf(p_file, "\n");
    }

    if (i % 16 == 0)
      fprintf(p_file, "   ");
    fprintf(p_file, " %02X", (unsigned int)data[i]);

    if (i == size - 1) {
      for (j = 0; j < 15 - i % 16; j++) {
        fprintf(p_file, "   ");
      }

      fprintf(p_file, "         ");

      for (j = i - i % 16; j <= i; j++) {
        if (data[j] >= 32 && data[j] <= 128) {
          fprintf(p_file, "%c", (unsigned char)data[j]);
        } else {
          fprintf(p_file, ".");
        }
      }

      fprintf(p_file, "\n");
    }
  }
}