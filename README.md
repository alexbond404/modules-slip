# slip

Module to pack/unpack SLIP stream.

SLIP is used to migrate between stream and packets.

There are some rules for SLIP stream:
1. Packet starts and ends with 0xC0 symbol
2. 0xC0 symbols in data are replaced with 0xDB 0xDC
3. 0xDB symbols in data are replaced with 0xDB 0xDD


## Using

### Context. Init
Mostly all functions required a context `slip_struct*` to store state. Before any use context should be defined and initialized with `slip_init`:

```
slip_struct slip;
uint8_t buf[1024];

slip_init(&slip, buf, sizeof(buf));
```

`buf` us a buffer to store decoded packet. So, size of buf should be not smaller than the biggest possible packet.  
Note: `buf` will contain decoded packet for further user processing.

### Receive
When you need to process the next portion of received bytes call `slip_proc`:

```
int len_recv = slip_proc(&slip, buf_rx, len_rx);
```
where `buf_rx` is buffer and `len_rx` is amount of bytes to process.  
Function returns:
- positive value (len of packet) if some packet was decoded;
- 0 if packet is not fully received;
- negative value in case of error.

If there were several packets in `buf_rx`, extra call of `slip_recv` is required.  
Generally, receive engine may look like this:

```
int len_recv = slip_proc(&slip, buf_rx, len_rx);
while (len_recv > 0)
{
    /* user process packet in buf here */

    len_recv = slip_recv(&slip);
}
```
Here packets are decoded one by one from `buf_rx`.  
Note: during this process `buf_rx` should exist and remains unchanged!

### Reset

For some reason you may want to reset current receiving state. This may be done with `slip_reset` function:

```
slip_reset(&slip);
```

### Send
To send packet with `slip_send` no context is required. Sending is done by on-the-fly encoding and sending data.

```
slip_send(buf, len, send_func);
```
In this example buffer `buf` of length `len` will be encoded and sent with `send_func` which called every SLIP_SEND_CHUNK encoded bytes.


## slip_def.h
To config module you should use `slip_defs.h` file which must define the following:
- SLIP_SEND_CHUNK - with this maximum number of bytes encoded packet will be devided.
- SLIP_RECV_TIMEOUT - if more than this amount of ticks passed between `slip_proc` than buffer will be reset (receive timeout reset)
- GET_TICK_COUNT() - should return number of ticks to calculate timeout. If timeout is not required, may be 0.


## Unit tests
Unit tests are located in tests dir. They may be compiled with help of Google Test framework.
