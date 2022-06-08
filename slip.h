#ifndef __SLIP_H
#define __SLIP_H

#define SLIP_ERR_OVERFLOW           -1
#define SLIP_ERR_ESC_SEQ            -2

typedef struct
{
    uint8_t *buf;
    uint16_t usBufLen;

    uint16_t usBufPos;

    uint8_t fSlipSOP;
    uint8_t fSlipESC;
    uint32_t ulRxTime;

    uint8_t *buf_unproc;
    uint16_t usBufLenUnproc;
} slip_struct;


void slip_init(slip_struct *pSlip, uint8_t *buf, uint16_t len);
void slip_reset(slip_struct *pSlip);
int slip_proc(slip_struct *pSlip, uint8_t *buf, uint16_t len);
int slip_recv(slip_struct *pSlip);

int slip_send(uint8_t *buf, uint16_t len, int (*send_func)(uint8_t *buf, uint16_t len));

#endif //__SLIP_H