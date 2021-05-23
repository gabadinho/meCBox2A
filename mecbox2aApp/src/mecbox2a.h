#ifndef _CBOX2A_H_
#define _CBOX2A_H_

#ifdef __cplusplus

#include "asynPortDriver.h"



#define P_CB2A_FLAGSLO_String  "CB2A_FLAGSLO"
#define P_CB2A_SERIALNR_String "CB2A_SERIALNR"

#define P_CB2A_TYPE_1_String   "CB2A_TYPE_1"
#define P_CB2A_RANGE_1_String  "CB2A_RANGE_1"
#define P_CB2A_SENSOR_1_String "CB2A_SENSOR_1"

#define P_CB2A_TYPE_2_String   "CB2A_TYPE_2"
#define P_CB2A_RANGE_2_String  "CB2A_RANGE_2"
#define P_CB2A_SENSOR_2_String "CB2A_SENSOR_2"



class cbox2aDriver: public asynPortDriver {
public:
    cbox2aDriver(const char *portName, const char *asynPortName);
    virtual ~cbox2aDriver() {}

    virtual void report(FILE *fp, int level);

    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    //virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    //virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);

    void cbox2aTask();  // This should be private but is called from C function

protected:

    //asynUser *pasynUserController_;

    /*
    asynUser *pasynUserCBox;
    */
    char *pasynPortName;

    int P_CB2A_FLAGSLO;
    int P_CB2A_SERIALNR;

    int P_CB2A_TYPE_1;
    int P_CB2A_RANGE_1;
    int P_CB2A_SENSOR_1;

    int P_CB2A_TYPE_2;
    int P_CB2A_RANGE_2;
    int P_CB2A_SENSOR_2;
};

#endif // __cplusplus
#endif // _CBOX2A_H_
