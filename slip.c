#include "../slip_defs.h"
#include "slip.h"


#define SLIP_SEP        0xC0
#define SLIP_ESC        0xDB
#define SLIP_ESC_SEP    0xDC
#define SLIP_ESC_ESC    0xDD


void slip_init(slip_struct *pSlip, uint8_t *buf, uint16_t len)
{
    pSlip->buf = buf;
    pSlip->usBufLen = len;

    pSlip->usBufPos = 0;

    pSlip->fSlipSOP = 0;
    pSlip->fSlipESC = 0;

    pSlip->buf_unproc = NULL;
}

void slip_reset(slip_struct *pSlip)
{
    pSlip->usBufPos = 0;

    pSlip->fSlipSOP = 0;
    pSlip->fSlipESC = 0;

    pSlip->buf_unproc = NULL;
}

int slip_proc(slip_struct *pSlip, uint8_t *buf, uint16_t len)
{
    int res = 0;

    // check for timeout
    if (((uint32_t)(GET_TICK_COUNT()) - pSlip->ulRxTime) > SLIP_RECV_TIMEOUT)
    {
        slip_reset(pSlip);
    }

    // process the whole buffer
    for (uint16_t i = 0; i < len; i++)
    {
        if (!pSlip->fSlipSOP)
        {
            if (SLIP_SEP == buf[i])
            {
                pSlip->fSlipSOP = 1;
                pSlip->usBufPos = 0;
            }
        }
        else
        {
            if (SLIP_SEP == buf[i])
            {
                if (pSlip->usBufPos)
                {
                    // return only with non-empty packets
                    pSlip->fSlipSOP = 0;
                    res = pSlip->usBufPos;

                    // save pointer to unprocced part of the buf and exit
                    i++;
                    if (i < len)
                    {
                        pSlip->buf_unproc = &buf[i];
                        pSlip->usBufLenUnproc = len - i;
                    }
                    break;
                }
            }
            else
            {
                uint8_t ch;

                // already found ESC?
                if (pSlip->fSlipESC)
                {
                    pSlip->fSlipESC = 0;
                    if (SLIP_ESC_SEP == buf[i])
                    {
                        ch = SLIP_SEP;
                    }
                    else if (SLIP_ESC_ESC == buf[i])
                    {
                        ch = SLIP_ESC;
                    }
                    else
                    {
                        res = SLIP_ERR_ESC_SEQ;
                        slip_reset(pSlip);

                        // save pointer to unprocced part of the buf and exit
                        i++;
                        if (i < len)
                        {
                            pSlip->buf_unproc = &buf[i];
                            pSlip->usBufLenUnproc = len - i;
                        }
                        break;
                    }
                }
                else
                {
                    // ESC?
                    if (SLIP_ESC == buf[i])
                    {
                        pSlip->fSlipESC = 1;
                        continue;
                    }

                    ch = buf[i];
                }

                if (pSlip->usBufPos < pSlip->usBufLen)
                {
                    pSlip->buf[pSlip->usBufPos++] = ch;
                }
                else
                {
                    // an error occured (overflow)
                    res = SLIP_ERR_OVERFLOW;
                    slip_reset(pSlip);

                    // save pointer to unprocced part of the buf and exit
                    i++;
                    if (i < len)
                    {
                        pSlip->buf_unproc = &buf[i];
                        pSlip->usBufLenUnproc = len - i;
                    }
                    break;
                }
            }
        }
    }

    // update rx timestamp
    pSlip->ulRxTime = GET_TICK_COUNT();

    return res;
}

int slip_recv(slip_struct *pSlip)
{
    if (pSlip->buf_unproc)
    {
        uint8_t *ptr = pSlip->buf_unproc;
        pSlip->buf_unproc = NULL;
        return slip_proc(pSlip, ptr, pSlip->usBufLenUnproc);
    }
    else
    {
        return 0;
    }
}

int slip_send(uint8_t *buf, uint16_t len, int (*send_func)(uint8_t *buf, uint16_t len))
{
    int res = 0;
    uint8_t buf_send[SLIP_SEND_CHUNK];
    uint16_t pos_w = 0;
    uint16_t pos_r = 0;

    // packet start symbol
    buf_send[pos_w++] = SLIP_SEP;

    // packet payload
    while ((pos_r < len) && (res == 0))
    {
        if (SLIP_SEP == buf[pos_r])
        {
            buf_send[pos_w++] = SLIP_ESC;
            buf_send[pos_w++] = SLIP_ESC_SEP;
        }
        else if (SLIP_ESC == buf[pos_r])
        {
            buf_send[pos_w++] = SLIP_ESC;
            buf_send[pos_w++] = SLIP_ESC_ESC;
        }
        else
        {
            buf_send[pos_w++] = buf[pos_r];
        }
        pos_r++;

        // check for possible overflow
        if (pos_w + 2 > SLIP_SEND_CHUNK)
        {
            res = send_func(buf_send, pos_w);
            pos_w = 0;
        }
    }

    // packet finish symbol
    if ((res == 0) && (pos_w + 1 > SLIP_SEND_CHUNK))
    {
        res = send_func(buf_send, pos_w);
        pos_w = 0;
    }
    if (res == 0)
    {
        buf_send[pos_w++] = SLIP_SEP;
        res = send_func(buf_send, pos_w);
    }

    return res;
}
