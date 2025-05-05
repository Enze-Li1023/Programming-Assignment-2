#include "sr.h"
#include <string.h>

int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    int i;
    checksum += packet.seqnum + packet.acknum;
    for (i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    return checksum;
}

/* A side */

int base_A = 0;
int nextseqnum_A = 0;
struct pkt window_A[SEQSPACE];
int window_A_valid[SEQSPACE];

void A_output(struct msg message) {
    if ((nextseqnum_A + SEQSPACE - base_A) % SEQSPACE >= WINDOWSIZE) {
        printf("A_output: Window is full, dropping message.\n");
        return;
    }

    struct pkt packet;
    packet.seqnum = nextseqnum_A;
    packet.acknum = 0;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = ComputeChecksum(packet);

    window_A[packet.seqnum % SEQSPACE] = packet;
    window_A_valid[packet.seqnum % SEQSPACE] = 1;

    tolayer3(0, packet);
    if (base_A == nextseqnum_A) {
        starttimer(0, RTT);
    }

    nextseqnum_A = (nextseqnum_A + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    if (ComputeChecksum(packet) != packet.checksum) {
        return;
    }

    int acknum = packet.acknum;
    if ((base_A <= acknum && acknum < nextseqnum_A) ||
        (nextseqnum_A < base_A && (acknum < nextseqnum_A || acknum >= base_A))) {

        window_A_valid[acknum % SEQSPACE] = 0;

        while (!window_A_valid[base_A % SEQSPACE] &&
               base_A != nextseqnum_A) {
            base_A = (base_A + 1) % SEQSPACE;
        }

        if (base_A == nextseqnum_A) {
            stoptimer(0);
        } else {
            starttimer(0, RTT);
        }
    }
}

void A_timerinterrupt(void) {
    int i;
    for (i = 0; i < SEQSPACE; i++) {
        if (window_A_valid[i]) {
            tolayer3(0, window_A[i]);
            break; /* simulate independent timers */
        }
    }
    starttimer(0, RTT);
}

void A_init(void) {
    int i;
    base_A = 0;
    nextseqnum_A = 0;
    for (i = 0; i < SEQSPACE; i++) {
        window_A_valid[i] = 0;
    }
}

/* B side */

int expected_seqnum_B = 0;
struct pkt buffer_B[SEQSPACE];
int buffer_B_valid[SEQSPACE];

void B_input(struct pkt packet) {
    if (ComputeChecksum(packet) != packet.checksum) {
        return;
    }

    int seqnum = packet.seqnum;
    if (seqnum == expected_seqnum_B) {
        tolayer5(1, packet.payload);
        buffer_B_valid[seqnum % SEQSPACE] = 0;
        expected_seqnum_B = (expected_seqnum_B + 1) % SEQSPACE;

        while (buffer_B_valid[expected_seqnum_B % SEQSPACE]) {
            tolayer5(1, buffer_B[expected_seqnum_B % SEQSPACE].payload);
            buffer_B_valid[expected_seqnum_B % SEQSPACE] = 0;
            expected_seqnum_B = (expected_seqnum_B + 1) % SEQSPACE;
        }
    } else if ((expected_seqnum_B < seqnum && seqnum < expected_seqnum_B + WINDOWSIZE) ||
               (expected_seqnum_B + WINDOWSIZE >= SEQSPACE &&
                ((seqnum < (expected_seqnum_B + WINDOWSIZE) % SEQSPACE) || seqnum >= expected_seqnum_B))) {
        buffer_B[seqnum % SEQSPACE] = packet;
        buffer_B_valid[seqnum % SEQSPACE] = 1;
    }

    struct pkt ack_pkt;
    ack_pkt.seqnum = 0;
    ack_pkt.acknum = seqnum;
    memset(ack_pkt.payload, 0, 20);
    ack_pkt.checksum = ComputeChecksum(ack_pkt);
    tolayer3(1, ack_pkt);
}

void B_output(struct msg message) {
    /* B doesn’t send messages */
}

void B_timerinterrupt(void) {
    /* No timer on B side */
}

void B_init(void) {
    int i;
    expected_seqnum_B = 0;
    for (i = 0; i < SEQSPACE; i++) {
        buffer_B_valid[i] = 0;
    }
}
