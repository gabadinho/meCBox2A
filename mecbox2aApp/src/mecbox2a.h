#ifndef _CBOX2A_H_
#define _CBOX2A_H_

#ifdef __cplusplus
#include "asynPortDriver.h"


//#define P_CB2A_CONTROLLER_String "CB2A_CONTROLLER"
#define P_CB2A_SENSOR_1_String   "CB2A_SENSOR_1"
#define P_CB2A_SENSOR_2_String   "CB2A_SENSOR_2"

class cbox2aDriver: public asynPortDriver {
public:
    cbox2aDriver(const char *portName, const char *asynPortName, int sensor1Type, int sensor2Type);
    virtual ~cbox2aDriver() {}

    virtual void report(FILE *fp, int level);

    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    //virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    //virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);

    void cbox2aTask();  // This should be private but is called from C function

protected:
    int sensor1Type;
    int sensor2Type;

    //asynUser *pasynUserController_;

    /*
    asynUser *pasynUserCBox;
    char *pasynPortName;
    */

    //int P_CB2A_CONTROLLER;
    int P_CB2A_SENSOR_1;
    int P_CB2A_SENSOR_2;

};

#endif // __cplusplus
#endif // _CBOX2A_H_

