#include "sr.h"
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

int base_A;
int nextseqnum_A;
int expected_seqnum_B;
int window_size = WINDOWSIZE;
int acked[SEQSPACE];
struct pkt send_buffer[SEQSPACE];
struct pkt recv_buffer[SEQSPACE];

int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    int i;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (i = 0; i < 20; i++) {
        checksum += (int)packet.payload[i];
    }
    return checksum;
}

int is_corrupted(struct pkt packet) {
    return ComputeChecksum(packet) != packet.checksum;
}

int in_window(int seqnum, int base, int size) {
    return (seqnum >= base && seqnum < base + size) || 
           (base + size >= SEQSPACE && seqnum < (base + size) % SEQSPACE);
}

void A_output(struct msg message) {
    if ((nextseqnum_A + SEQSPACE - base_A) % SEQSPACE >= window_size) {
        return;
    }

    struct pkt packet;
    int i;

    packet.seqnum = nextseqnum_A;
    packet.acknum = 0;
    for (i = 0; i < 20; i++) {
        packet.payload[i] = message.data[i];
    }
    packet.checksum = ComputeChecksum(packet);

    send_buffer[nextseqnum_A] = packet;
    tolayer3(0, packet);

    if (base_A == nextseqnum_A) {
        starttimer(0, RTT);
    }

    nextseqnum_A = (nextseqnum_A + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    int acknum = packet.acknum;

    if (is_corrupted(packet) || !in_window(acknum, base_A, window_size)) {
        return;
    }

    acked[acknum] = TRUE;

    while (acked[base_A]) {
        acked[base_A] = FALSE;
        base_A = (base_A + 1) % SEQSPACE;
    }

    if (base_A == nextseqnum_A) {
        stoptimer(0);
    } else {
        starttimer(0, RTT);
    }
}

void A_timerinterrupt() {
    int i;
    for (i = 0; i < window_size; i++) {
        int seq = (base_A + i) % SEQSPACE;
        if (!acked[seq]) {
            tolayer3(0, send_buffer[seq]);
        }
    }
    starttimer(0, RTT);
}

void A_init() {
    base_A = 0;
    nextseqnum_A = 0;
    memset(acked, 0, sizeof(acked));
}

void B_input(struct pkt packet) {
    int seqnum = packet.seqnum;
    int i;

    if (is_corrupted(packet)) {
        return;
    }

    if (in_window(seqnum, expected_seqnum_B, window_size)) {
        recv_buffer[seqnum] = packet;

        struct pkt ack_pkt;
        ack_pkt.seqnum = 0;
        ack_pkt.acknum = seqnum;
        ack_pkt.checksum = ComputeChecksum(ack_pkt);
        tolayer3(1, ack_pkt);
    }

    while (recv_buffer[expected_seqnum_B].checksum != 0 &&
           !is_corrupted(recv_buffer[expected_seqnum_B])) {
        tolayer5(1, recv_buffer[expected_seqnum_B].payload);
        memset(&recv_buffer[expected_seqnum_B], 0, sizeof(struct pkt));
        expected_seqnum_B = (expected_seqnum_B + 1) % SEQSPACE;
    }
}

void B_init() {
    expected_seqnum_B = 0;
    memset(recv_buffer, 0, sizeof(recv_buffer));
}
