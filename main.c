#include <stdio.h>
#include <stdint.h>
#include <pcap.h>
#include "packet.h"
#include "congestion.h"
#include "sample.h"

int main(int argc, char ** argv) {
    pcap_t *pcap[16];
    struct pcap_pkthdr hdr[16];
    int i;
    argc --;
    for (i = 0; i < argc; i++) {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap[i] = pcap_open_offline(argv[i + 1], errbuf);
    }
    const u_char *buf[16] = {NULL};
    packet_t packet[16];
    int flag = 0;
    int min = 0;
    int pkt_cnt = 0;
    do {
        flag = 0;
        min = -1;
        for (i = 0; i < argc; i++) {
            if (buf[i] == NULL) {
                buf[i] = pcap_next(pcap[i], &hdr[i]);
            }
            if (buf[i] != NULL) {
                flag ++;

                extract_packet(&packet[i], buf[i], hdr->len);
                // printf("i %d\n", i);
                if (packet[i].int_valid == 0) {
                    buf[i] = NULL;
                    flag --;
                }
                if (min < 0) {
                    min = i;
                } else {
                    if (packet[i].int_valid) {
                        if (packet[i].ts.sec < packet[min].ts.sec) {
                            min = i;
                        } else if (packet[i].ts.sec == packet[min].ts.sec) {
                            if (packet[i].ts.nsec < packet[min].ts.nsec) {
                                min = i;
                            }
                        }
                    }
                }
            }
        }
        if (min < 0) {
            break;
        }
        buf[min] = NULL;
        pkt_cnt++;
        record_congestion_event(&packet[min]);
        if (pkt_cnt % 10 == 1) {
            record_sample_event(&packet[min]);
        }
    } while(flag > 0);

    for (i = 0; i < argc; i++) {
        pcap_close(pcap[i]);
    }
    return 0;
}