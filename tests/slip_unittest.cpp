#include "gtest/gtest.h"

#define GTEST
#include "slip.c"


static uint32_t ulTickCount = 0;


class SlipRecvTest : public ::testing::Test {
  protected:
    slip_struct slip;
    uint8_t buf[8];

    void SetUp() override {
        ulTickCount = 0;
        slip_init(&slip, buf, sizeof(buf));
    }
};

// Test basic receive
TEST_F(SlipRecvTest, TestRecvUnique) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03", 3), 0);

    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);
}

// Test empty packet
TEST_F(SlipRecvTest, TestRecvEmpty) {
    uint8_t buf[] = "\xC0\xC0\x01\x02\x03\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03", 3), 0);

    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);
}

// Test basic receive with escapes
TEST_F(SlipRecvTest, TestRecvEscapesSOPUnique) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xdb\xdc\x04\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 5);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03\xC0\x04", 5), 0);
    
    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);
}

// Test basic receive with escapes
TEST_F(SlipRecvTest, TestRecvEscapesESCUnique) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xdb\xdd\x04\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 5);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03\xdb\x04", 5), 0);
    
    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);
}

// Test basic receive with escapes
TEST_F(SlipRecvTest, TestRecvEscapesBothUnique) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xdb\xdc\x04\xdb\xdd\x05\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 7);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03\xc0\x04\xdb\x05", 7), 0);

    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);
}

// Test receive error
TEST_F(SlipRecvTest, TestRecvError) {
    uint8_t buf_ovfl[] = "\xC0\x01\x02\x03\x04\x05\x06\x07\x08\x09\xC0";
    uint8_t buf_err_seq[] = "\xC0\x01\x02\xDB\xDA\xC0";

    int len = slip_proc(&slip, buf_ovfl, sizeof(buf_ovfl)-1);
    ASSERT_EQ(len, SLIP_ERR_OVERFLOW);
    len = slip_proc(&slip, buf_err_seq, sizeof(buf_err_seq)-1);
    ASSERT_EQ(len, SLIP_ERR_ESC_SEQ);
}

// Test receive several
TEST_F(SlipRecvTest, TestRecvSeveral) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xC0\xC0\x05\x06\x07\x08\xC0";

    int len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03", 3), 0);

    len = slip_recv(&slip);
    ASSERT_EQ(len, 4);
    ASSERT_EQ(memcmp(this->buf, "\x05\x06\x07\x08", 4), 0);
}

// Test receive timeout
TEST_F(SlipRecvTest, TestRecvTimeout) {
    uint8_t buf[] = "\xC0\x01\x02\x03\xC0";
    const uint8_t half = (sizeof(buf)-1) / 2;

    // receive partically
    ulTickCount = SLIP_RECV_TIMEOUT;
    int len = slip_proc(&slip, buf, half);
    ASSERT_EQ(len, 0);
    ulTickCount += SLIP_RECV_TIMEOUT/2;
    len = slip_proc(&slip, &buf[half], sizeof(buf)-1 - half);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03", 3), 0);

    len = slip_recv(&slip);
    ASSERT_EQ(len, 0);

    // recevive the same with timeout
    len = slip_proc(&slip, buf, half);
    ASSERT_EQ(len, 0);
    ulTickCount += SLIP_RECV_TIMEOUT+1;
    len = slip_proc(&slip, &buf[half], sizeof(buf)-1 - half);
    ASSERT_EQ(len, 0);

    // receive normally
    ulTickCount += SLIP_RECV_TIMEOUT/2;
    len = slip_proc(&slip, buf, sizeof(buf)-1);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(memcmp(this->buf, "\x01\x02\x03", 3), 0);
}


uint8_t buf_send[256];
uint16_t buf_send_pos = 0;
int send_data(uint8_t *buf, uint16_t len)
{
    memcpy(&buf_send[buf_send_pos], buf, len);
    buf_send_pos += len;
    return 0;
}

// Test send
TEST(SlipSendTest, TestSend) {
    uint8_t buf_basic[] = "\x01\x02\x03";
    uint8_t buf_esc[] = "\x01\xC0\xC0\x02\xDB\x03";

    // basic send without esc symbols
    buf_send_pos = 0;
    int res = slip_send(buf_basic, sizeof(buf_basic)-1, send_data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(buf_send_pos, 2+3);
    ASSERT_EQ(memcmp(buf_send, "\xC0\x01\x02\x03\xC0", 5), 0);

    // send with esc symbols inside
    buf_send_pos = 0;
    res = slip_send(buf_esc, sizeof(buf_esc)-1, send_data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(buf_send_pos, 2+6+3);
    ASSERT_EQ(memcmp(buf_send, "\xC0\x01\xDB\xDC\xDB\xDC\x02\xDB\xDD\x03\xC0", 11), 0);
}


uint32_t GetTickCount()
{
    return ulTickCount;
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
