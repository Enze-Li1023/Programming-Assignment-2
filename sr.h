#ifndef SR_H
#define SR_H

/* 不要重新定义 struct msg 和 struct pkt */

/* A-side functions */
void A_init(void);
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);

/* B-side functions */
void B_init(void);
void B_input(struct pkt packet);
void B_output(struct msg message);  // 如果你有实现此函数

#endif
