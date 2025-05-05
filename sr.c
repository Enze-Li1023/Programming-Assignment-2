#include <stdio.h>
#include <string.h>
#include "sr.h"

/* A-side variables */
int base_A = 0;
int nextseqnum_A = 0;
struct pkt send_buffer[SEQSPACE];
int acked[SEQSPACE];

/* B-side variables */
int expected_seqnum_B = 0;
struct pkt recv_buffer[SEQSPACE];
int received[SEQSPACE];

int ComputeChecksum(struct pkt packet) {
    int checksum = packet.seqnum + packet.acknum;
    int i;
    for (i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    return checksum;
}

void A_output(struct msg message) {
    if (nextseqnum_A < base_A + SEQSPACE) {
        struct pkt packet;
        packet.seqnum = nextseqnum_A;
        packet.acknum = -1;
        memcpy(packet.payload, message.data, 20);
        packet.checksum = ComputeChecksum(packet);

        send_buffer[nextseqnum_A % SEQSPACE] = packet;
        acked[nextseqnum_A % SEQSPACE] = 0;

        tolayer3(0, packet);
        if (base_A == nextseqnum_A) {
            starttimer(0, RTT);
        }
        nextseqnum_A++;
    }
}

void A_input(struct pkt packet) {
    int checksum = ComputeChecksum(packet);
    if (checksum == packet.checksum && packet.acknum >= base_A && packet.acknum < nextseqnum_A) {
        acked[packet.acknum % SEQSPACE] = 1;
        while (acked[base_A % SEQSPACE] && base_A < nextseqnum_A) {
            base_A++;
        }
        if (base_A == nextseqnum_A) {
            stoptimer(0);
        } else {
            starttimer(0, RTT);
        }
    }
}

void A_timerinterrupt() {
    int i;
    for (i = base_A; i < nextseqnum_A; i++) {
        tolayer3(0, send_buffer[i % SEQSPACE]);
    }
    starttimer(0, RTT);
}

void A_init() {
    int i;
    base_A = 0;
    nextseqnum_A = 0;
    for (i = 0; i < SEQSPACE; i++) {
        acked[i] = 0;
    }
}

void B_input(struct pkt packet) {
    int checksum = ComputeChecksum(packet);
    if (checksum == packet.checksum) {
        if (packet.seqnum >= expected_seqnum_B && packet.seqnum < expected_seqnum_B + SEQSPACE) {
            recv_buffer[packet.seqnum % SEQSPACE] = packet;
            received[packet.seqnum % SEQSPACE] = 1;

            while (received[expected_seqnum_B % SEQSPACE]) {
                tolayer5(1, recv_buffer[expected_seqnum_B % SEQSPACE].payload);
                received[expected_seqnum_B % SEQSPACE] = 0;
                expected_seqnum_B++;
            }
        }

        struct pkt ack_pkt;
        ack_pkt.seqnum = 0;
        ack_pkt.acknum = packet.seqnum;
        memset(ack_pkt.payload, 0, 20);
        ack_pkt.checksum = ComputeChecksum(ack_pkt);
        tolayer3(1, ack_pkt);
    }
}

void B_init() {
    int i;
    expected_seqnum_B = 0;
    for (i = 0; i < SEQSPACE; i++) {
        received[i] = 0;
    }
}