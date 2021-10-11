/*
FILENAME...   mecbox2a.h
USAGE...      Asyn driver support for the Micro-Epsilon C-Box/2A laser distance sensor

Jose G.C. Gabadinho
August 2021
*/

#ifndef _CBOX2A_H_
#define _CBOX2A_H_

#ifdef __cplusplus

#include <asynPortDriver.h>



#define P_CB2A_FLAGS1_String       "CB2A_FLAGS1"
#define P_CB2A_SERIALNR_String     "CB2A_SERIALNR"
#define P_CB2A_FRAMECOUNTER_String "CB2A_FRAMECOUNTER"

#define P_CB2A_TYPE_1_String   "CB2A_TYPE_1"
#define P_CB2A_RANGE_1_String  "CB2A_RANGE_1"
#define P_CB2A_SENSOR_1_String "CB2A_SENSOR_1"
#define P_CB2A_STATE_1_String  "CB2A_STATE_1"

#define P_CB2A_TYPE_2_String   "CB2A_TYPE_2"
#define P_CB2A_RANGE_2_String  "CB2A_RANGE_2"
#define P_CB2A_SENSOR_2_String "CB2A_SENSOR_2"
#define P_CB2A_STATE_2_String  "CB2A_STATE_2"

const size_t PREAMBLE_LENGTH = 4;
const char CBOX2A_PREAMBLE[] = "SAEM";

const size_t DATAFRAME_PREAMBLE_OFFSET     = 0;
const size_t DATAFRAME_SERIALNR_OFFSET     = 8;
const size_t DATAFRAME_FLAGS1_OFFSET       = 12;
const size_t DATAFRAME_FRAMECOUNTER_OFFSET = 22;

const size_t DATAFRAME_SENSOR1_OFFSET = 28;
const size_t DATAFRAME_STATE1_OFFSET  = 30;

const size_t DATAFRAME_SENSOR2_OFFSET = 32;
const size_t DATAFRAME_STATE2_OFFSET  = 34;

#define FRAME_SIZE 36



struct extractedData {
    unsigned serial_number;
    unsigned flags1;
    unsigned frame_counter;
    unsigned short sensor1;
    unsigned short state1;
    unsigned short sensor2;
    unsigned short state2;
};

class cbox2aDriver: public asynPortDriver {
public:
    cbox2aDriver(const char *portName, const char *asynPortName);
    virtual ~cbox2aDriver() {}

    // These are the methods we override from the base class
    virtual void report(FILE *fp, int level);

    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    // Specific class methods
    void cbox2aTask(); // This should be private but needs to be called from a C function

    static void ringBufferAppend(unsigned char *ring_buffer, size_t ring_buffer_size, size_t *ring_buffer_next_write_idx, const unsigned char *read_buffer, size_t bytes_read);

    static const unsigned char * findPreamble(const unsigned char *buffer, size_t buffer_len);
    static bool extractData(const unsigned char *buffer, struct extractedData *storage);

    void invalidateEverything();

protected:
    void bufferDebugPrint(unsigned char *buffer, size_t buffer_len);

    int P_CB2A_FLAGS1;
    int P_CB2A_SERIALNR;
    int P_CB2A_FRAMECOUNTER;

    int P_CB2A_TYPE_1;
    int P_CB2A_RANGE_1;
    int P_CB2A_SENSOR_1;
    int P_CB2A_STATE_1;

    int P_CB2A_TYPE_2;
    int P_CB2A_RANGE_2;
    int P_CB2A_SENSOR_2;
    int P_CB2A_STATE_2;

private:
    char *pasynPortName;
};

#endif // __cplusplus
#endif // _CBOX2A_H_
