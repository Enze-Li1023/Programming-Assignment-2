#ifndef SR_H
#define SR_H

#define SEQSPACE 8
#define RTT 15.0

struct msg {
    char data[20];
};

struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

// A-side functions
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt();
void A_init();

// B-side functions
void B_input(struct pkt packet);
void B_output(struct msg message);
void B_init();

#endif
