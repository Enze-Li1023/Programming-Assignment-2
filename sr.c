// sr.c - Fixed implementation of Selective Repeat (SR) protocol for basic test
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "sr.h"

#define RTT 20.0
#define SEQSPACE 8

int base_A = 0;
int nextseq_A = 0;
struct pkt send_buffer[SEQSPACE];
bool acked[SEQSPACE];

int expected_seqnum_B = 0;
struct pkt recv_buffer[SEQSPACE];
bool received[SEQSPACE];

int packets_resent = 0;
int delivered_msgs = 0;

int ComputeChecksum(struct pkt packet) {
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    return checksum;
}

bool InWindow(int base, int seqnum) {
    return ((seqnum >= base && seqnum < base + SEQSPACE / 2) ||
            (base + SEQSPACE / 2 >= SEQSPACE && seqnum < (base + SEQSPACE / 2) % SEQSPACE));
}

void A_output(struct msg message) {
    if (!InWindow(base_A, nextseq_A)) {
        printf("A: Window full, message dropped\n");
        return;
    }
    struct pkt packet;
    packet.seqnum = nextseq_A;
    packet.acknum = 0;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = ComputeChecksum(packet);

    send_buffer[nextseq_A] = packet;
    acked[nextseq_A] = false;

    printf("A: Sent packet %d\n", packet.seqnum);
    tolayer3(A, packet);

    if (base_A == nextseq_A) {
        starttimer(A, RTT);
    }

    nextseq_A = (nextseq_A + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    int checksum = ComputeChecksum(packet);
    if (checksum != packet.checksum || !InWindow(base_A, packet.acknum)) {
        return;
    }

    printf("A: Received ACK %d\n", packet.acknum);
    acked[packet.acknum] = true;

    while (acked[base_A]) {
        acked[base_A] = false;
        base_A = (base_A + 1) % SEQSPACE;
    }

    if (base_A == nextseq_A) {
        stoptimer(A);
    } else {
        stoptimer(A);
        starttimer(A, RTT);
    }
}

void A_timerinterrupt() {
    printf("A: Timeout, resend packet %d\n", base_A);
    tolayer3(A, send_buffer[base_A]);
    starttimer(A, RTT);
    packets_resent++;
}

void A_init() {
    base_A = 0;
    nextseq_A = 0;
    for (int i = 0; i < SEQSPACE; i++) {
        acked[i] = false;
    }
}

void B_input(struct pkt packet) {
    int checksum = ComputeChecksum(packet);
    if (checksum != packet.checksum || !InWindow(expected_seqnum_B, packet.seqnum)) {
        printf("B: Corrupted packet, discarded\n");
        return;
    }

    if (received[packet.seqnum]) {
        // resend ACK
        struct pkt ack_pkt;
        ack_pkt.seqnum = 0;
        ack_pkt.acknum = packet.seqnum;
        memset(ack_pkt.payload, 0, 20);
        ack_pkt.checksum = ComputeChecksum(ack_pkt);
        tolayer3(B, ack_pkt);
        return;
    }

    recv_buffer[packet.seqnum] = packet;
    received[packet.seqnum] = true;
    printf("B: Stored packet %d\n", packet.seqnum);

    while (received[expected_seqnum_B]) {
        tolayer5(B, recv_buffer[expected_seqnum_B].payload);
        received[expected_seqnum_B] = false;
        delivered_msgs++;
        expected_seqnum_B = (expected_seqnum_B + 1) % SEQSPACE;
    }

    struct pkt ack_pkt;
    ack_pkt.seqnum = 0;
    ack_pkt.acknum = packet.seqnum;
    memset(ack_pkt.payload, 0, 20);
    ack_pkt.checksum = ComputeChecksum(ack_pkt);
    printf("B: Sent ACK %d\n", ack_pkt.acknum);
    tolayer3(B, ack_pkt);
}

void B_init() {
    expected_seqnum_B = 0;
    for (int i = 0; i < SEQSPACE; i++) {
        received[i] = false;
    }
}