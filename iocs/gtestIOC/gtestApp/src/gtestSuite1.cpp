#include <gtest/gtest.h>

#include "mecbox2a.h"



const unsigned char STANDARD_DATA_FRAME[] = {
0x53, 0x41, 0x45, 0x4d,

0x6a, 0xed, 0x24, 0x00,
0xc5, 0x47, 0x37, 0x01,

0x05, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x00,

0x08, 0x00,

0x01, 0x00, 0x52, 0xc2,

0xad, 0x09,

0x1b, 0x74,
0x00, 0x00,
0xc0, 0xff,
0x03, 0x00 
};

const unsigned char SHIFTED_DATA_FRAME[] = {
0x41, 0x45, 0x4d,

0x6a, 0xed, 0x24, 0x00,
0xc5, 0x47, 0x37, 0x01,

0x05, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x00,

0x08, 0x00,

0x01, 0x00, 0x52, 0xc2,

0xad, 0x09,

0x1b, 0x74,
0x00, 0x00,
0xc0, 0xff,
0x03, 0x00,

0x53
};

const unsigned char TWO_DATA_FRAMES[] = {
0x53, 0x41, 0x45, 0x4d, 
0x6a, 0xed, 0x24, 0x00,
0xc5, 0x47, 0x37, 0x01,
0x05, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x00,
0x08, 0x00,
0x01, 0x00, 0x54, 0xc2,
0xad, 0x09,
0x1b, 0x74,
0x00, 0x00,
0xc0, 0xff,
0x03, 0x00,
0x53, 0x41, 0x45, 0x4d,
0x6a, 0xed, 0x24, 0x00,
0xc5, 0x47, 0x37, 0x01,
0x05, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x00,
0x08, 0x00,
0x01, 0x00, 0x55, 0xc2,
0xad, 0x09,
0x1b, 0x74,
0x00, 0x00,
0xc0, 0xff,
0x03, 0x00 
};

const unsigned char RING_BUFFER[] = {
0,   1,  2,  3,  4,  5,  6,  7,  8,  9,
10, 11, 12, 13, 14, 15, 16, 17, 18, 19
};

const unsigned char READ_BUFFER[] = {
100, 101, 102, 103, 104, 105, 106, 107
};

const unsigned char RING_BUFFER_AFTER[] = {
103, 104, 105, 106, 107,  5,  6,   7,   8,   9,
 10,  11,  12,  13,  14, 15, 16, 100, 101, 102
};



TEST(ExtractionTest, NullDataFrame) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(nullptr, &data);

    ASSERT_EQ(false, res);
}

TEST(ExtractionTest, NullStorage) {
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, nullptr);

    ASSERT_EQ(false, res);
}

TEST(ExtractionTest, CheckSerialNumber) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, &data);

    ASSERT_EQ(true, res);
    ASSERT_EQ(20400069, data.serial_number);
}

TEST(ExtractionTest, CheckFlags1) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, &data);

    ASSERT_EQ(true, res);
    ASSERT_EQ(1073741829, data.flags1);
}

TEST(ExtractionTest, CheckFrameCounter) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, &data);

    ASSERT_EQ(true, res);
    ASSERT_EQ(3260153857, data.frame_counter);
}

TEST(ExtractionTest, CheckSensor1) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, &data);

    ASSERT_EQ(true, res);
    ASSERT_EQ(0, data.state1);
    ASSERT_EQ(29723, data.sensor1);
}

TEST(ExtractionTest, CheckSensor2) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(STANDARD_DATA_FRAME, &data);

    ASSERT_EQ(true, res);
    ASSERT_EQ(3, data.state2);
    ASSERT_EQ(65472, data.sensor2);
}

TEST(ExtractionTest, CheckCutDataFrame) {
    struct extractedData data;
    bool res = cbox2aDriver::extractData(SHIFTED_DATA_FRAME, &data);

    ASSERT_EQ(false, res);
}

TEST(PreambleTest, CheckCutDataFrame) {
    const unsigned char *res = cbox2aDriver::findPreamble(SHIFTED_DATA_FRAME, sizeof(SHIFTED_DATA_FRAME));
    ASSERT_EQ(NULL, res);
}

TEST(PreambleTest, CheckLastOfTwoDataFrames) {
    const unsigned char *res = cbox2aDriver::findPreamble(TWO_DATA_FRAMES, sizeof(TWO_DATA_FRAMES));
    ASSERT_EQ(&(TWO_DATA_FRAMES[FRAME_SIZE]), res);
}

TEST(RingBufferTest, WrapAroundTest) {
    size_t next_write = 17;
    unsigned char ring_buffer[sizeof(RING_BUFFER)];
    memcpy(ring_buffer, RING_BUFFER, sizeof(RING_BUFFER));

    cbox2aDriver::ringBufferAppend(ring_buffer, sizeof(RING_BUFFER), &next_write, READ_BUFFER, sizeof(READ_BUFFER));
    int res = memcmp(RING_BUFFER_AFTER, ring_buffer, sizeof(RING_BUFFER_AFTER));
    ASSERT_EQ(0, res);
}

TEST(RingBufferTest, WrapAroundTestIndex) {
    size_t next_write = 17;
    unsigned char ring_buffer[sizeof(RING_BUFFER)];
    memcpy(ring_buffer, RING_BUFFER, sizeof(RING_BUFFER));

    cbox2aDriver::ringBufferAppend(ring_buffer, sizeof(RING_BUFFER), &next_write, READ_BUFFER, sizeof(READ_BUFFER));
    ASSERT_EQ(5, next_write);
}

