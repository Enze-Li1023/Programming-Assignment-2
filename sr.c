#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "emulator.h"
#include "sr.h"

#define RTT 16.0
#define WINDOWSIZE 6
#define SEQSPACE 7
#define NOTINUSE -1

/* 计算校验和 */
int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++)
        checksum += packet.payload[i];
    return checksum;
}

/* 检查是否被破坏 */
bool IsCorrupted(struct pkt packet) {
    return packet.checksum != ComputeChecksum(packet);
}

/* 序号是否在窗口中 */
bool InWindow(int base, int seqnum) {
    return ((seqnum - base + SEQSPACE) % SEQSPACE) < WINDOWSIZE;
}

/* ========== A端（发送方）实现 ========== */
static struct pkt send_buffer[SEQSPACE];
static bool acked[SEQSPACE];
static bool timer_running[SEQSPACE];
static int base_A, nextseq_A;

void A_output(struct msg message) {
    if (!InWindow(base_A, nextseq_A)) {
        if (TRACE > 0)
            printf("A: Window full, message dropped\n");
        window_full++;
        return;
    }

    struct pkt packet;
    packet.seqnum = nextseq_A;
    packet.acknum = NOTINUSE;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = ComputeChecksum(packet);

    send_buffer[nextseq_A] = packet;
    acked[nextseq_A] = false;

    tolayer3(A, packet);
    if (!timer_running[nextseq_A]) {
        starttimer(A, RTT);
        timer_running[nextseq_A] = true;
    }

    if (TRACE > 0)
        printf("A: Sent packet %d\n", nextseq_A);

    nextseq_A = (nextseq_A + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    if (IsCorrupted(packet)) {
        if (TRACE > 0)
            printf("A: Corrupted ACK %d ignored\n", packet.acknum);
        return;
    }

    int acknum = packet.acknum;
    if (!acked[acknum]) {
        acked[acknum] = true;
        new_ACKs++;
        stoptimer(A);
        timer_running[acknum] = false;

        if (TRACE > 0)
            printf("A: Received ACK %d\n", acknum);

        while (acked[base_A]) {
            acked[base_A] = false;
            timer_running[base_A] = false;
            base_A = (base_A + 1) % SEQSPACE;
        }

        for (int i = 0; i < SEQSPACE; i++) {
            int idx = (base_A + i) % SEQSPACE;
            if (!acked[idx] && timer_running[idx]) {
                starttimer(A, RTT);
                break;
            }
        }
    }
}

void A_timerinterrupt(void) {
    for (int i = 0; i < SEQSPACE; i++) {
        if (InWindow(base_A, i) && !acked[i]) {
            tolayer3(A, send_buffer[i]);
            packets_resent++;
            starttimer(A, RTT);
            timer_running[i] = true;
            if (TRACE > 0)
                printf("A: Timeout, resend packet %d\n", i);
            break; // 只重发一个，模拟独立计时器
        }
    }
}

void A_init(void) {
    base_A = 0;
    nextseq_A = 0;
    for (int i = 0; i < SEQSPACE; i++) {
        acked[i] = false;
        timer_running[i] = false;
    }
}

/* ========== B端（接收方）实现 ========== */
static struct pkt recv_buffer[SEQSPACE];
static bool received[SEQSPACE];
static int expected_B;

void B_input(struct pkt packet) {
    if (IsCorrupted(packet)) {
        if (TRACE > 0)
            printf("B: Corrupted packet, discarded\n");
        return;
    }

    int seqnum = packet.seqnum;
    if (!InWindow(expected_B, seqnum)) {
        if (TRACE > 0)
            printf("B: Packet %d out of window, ignored\n", seqnum);
    } else {
        if (!received[seqnum]) {
            recv_buffer[seqnum] = packet;
            received[seqnum] = true;
            if (TRACE > 0)
                printf("B: Stored packet %d\n", seqnum);
        }

        // 交付按序数据
        while (received[expected_B]) {
            received[expected_B] = false;  // ✅ 提前标记为“已交付”，防止重复进入此循环
            tolayer5(B, recv_buffer[expected_B].payload);
            packets_received++;
            expected_B = (expected_B + 1) % SEQSPACE;
        }
               
    }

    // 发送ACK
    struct pkt ack_pkt;
    ack_pkt.seqnum = 0;
    ack_pkt.acknum = packet.seqnum;
    memset(ack_pkt.payload, 0, 20);
    ack_pkt.checksum = ComputeChecksum(ack_pkt);
    tolayer3(B, ack_pkt);

    if (TRACE > 0)
        printf("B: Sent ACK %d\n", packet.seqnum);
}

void B_init(void) {
    expected_B = 0;
    for (int i = 0; i < SEQSPACE; i++)
        received[i] = false;
}

void B_output(struct msg message) {}
void B_timerinterrupt(void) {}
