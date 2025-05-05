#include <stdio.h>
#include <string.h>
#include "emulator.h"
#include "sr.h"

#define RTT 15.0
#define SEQSPACE 8
#define WINDOW_SIZE 4

int base_A = 0;
int nextseqnum_A = 0;
struct pkt send_buffer[SEQSPACE];
int acked[SEQSPACE];

int base_B = 0;
int expected_seqnum_B = 0;
struct pkt recv_buffer[SEQSPACE];
int recv_buffer_valid[SEQSPACE];

int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    int i;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (i = 0; i < 20; i++) {
        checksum += (int)(packet.payload[i]);
    }
    return checksum;
}

void A_output(struct msg message) {
    int i;
    if ((nextseqnum_A + SEQSPACE - base_A) % SEQSPACE < WINDOW_SIZE) {
        struct pkt packet;
        packet.seqnum = nextseqnum_A;
        packet.acknum = 0;
        for (i = 0; i < 20; i++) {
            packet.payload[i] = message.data[i];
        }
        packet.checksum = ComputeChecksum(packet);
        send_buffer[nextseqnum_A % SEQSPACE] = packet;
        acked[nextseqnum_A % SEQSPACE] = 0;
        tolayer3(0, packet);
        if (base_A == nextseqnum_A) {
            starttimer(0, RTT);
        }
        nextseqnum_A = (nextseqnum_A + 1) % SEQSPACE;
    }
}

void A_input(struct pkt packet) {
    int acknum;
    if (ComputeChecksum(packet) != packet.checksum) {
        return;
    }
    acknum = packet.acknum;
    if (!acked[acknum % SEQSPACE]) {
        acked[acknum % SEQSPACE] = 1;
        if (acknum == base_A) {
            stoptimer(0);
            while (acked[base_A]) {
                base_A = (base_A + 1) % SEQSPACE;
            }
            if (base_A != nextseqnum_A) {
                starttimer(0, RTT);
            }
        }
    }
}

void A_timerinterrupt() {
    int i;
    for (i = 0; i < SEQSPACE; i++) {
        if (!acked[i] && (base_A <= i && i < (base_A + WINDOW_SIZE))) {
            tolayer3(0, send_buffer[i]);
            starttimer(0, RTT);
            break; /* 只重发一个，模拟独立计时器 */
        }
    }
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
    int seqnum;
    int checksum;
    int i;
    struct pkt ack_pkt;

    seqnum = packet.seqnum;
    checksum = ComputeChecksum(packet);

    if (checksum == packet.checksum) {
        if (!recv_buffer_valid[seqnum % SEQSPACE]) {
            recv_buffer_valid[seqnum % SEQSPACE] = 1;
            recv_buffer[seqnum % SEQSPACE] = packet;
            while (recv_buffer_valid[expected_seqnum_B % SEQSPACE]) {
                tolayer5(1, recv_buffer[expected_seqnum_B % SEQSPACE].payload);
                recv_buffer_valid[expected_seqnum_B % SEQSPACE] = 0;
                expected_seqnum_B = (expected_seqnum_B + 1) % SEQSPACE;
            }
        }
    }

    ack_pkt.seqnum = 0;
    ack_pkt.acknum = seqnum;
    for (i = 0; i < 20; i++) {
        ack_pkt.payload[i] = 0;
    }
    ack_pkt.checksum = ComputeChecksum(ack_pkt);
    tolayer3(1, ack_pkt);
}

void B_init() {
    int i;
    base_B = 0;
    expected_seqnum_B = 0;
    for (i = 0; i < SEQSPACE; i++) {
        recv_buffer_valid[i] = 0;
    }
}

void B_output(struct msg message) {
    /* Not needed for receiver */
}

void B_timerinterrupt(void) {
    /* Not used for receiver */
}
