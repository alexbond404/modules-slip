#ifndef __SLIP_DEFS_H
#define __SLIP_DEFS_H


#ifdef GTEST
 #include <stdint.h>
 #include <string.h>

 #define SLIP_SEND_CHUNK            4
 #define SLIP_RECV_TIMEOUT          500

 uint32_t GetTickCount();
 #define GET_TICK_COUNT()           GetTickCount()
#else
 #include <linux/types.h>

 #define SLIP_SEND_CHUNK            64
 #define SLIP_RECV_TIMEOUT          500
 #define GET_TICK_COUNT()           0
#endif //GTEST

#endif //__SLIP_DEFS_H