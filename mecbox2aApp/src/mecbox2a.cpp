#include "mecbox2a.h"

#include <stdio.h>

#include "iocsh.h"
#include "epicsExport.h"
#include <epicsString.h>
#include "epicsThread.h"

//#include "asynOctetSyncIO.h"

#define BUFFER_SIZE 1024
#define FRAME_SIZE  36

/*

socat -d -d pty,raw,echo=0 pty,raw,echo=0

http://epics.isis.stfc.ac.uk/doxygen/lvDCOM.old/html/lvDCOMDriver_8h_source.html
http://epics.isis.stfc.ac.uk/doxygen/lvDCOM.old/html/lvDCOMDriver_8cpp_source.html
https://stackoverflow.com/questions/52187/virtual-serial-port-for-linux

 */

static const char *driverName="meCBox2A";


static void cbox2aTaskC(void *drvPvt);

cbox2aDriver::cbox2aDriver(const char *portName, const char *asynPortName, int sensor1Type, int sensor2Type):
    asynPortDriver(portName, 
                   1, /* maxAddr */ 
                   asynInt32Mask | asynFloat64Mask  | asynDrvUserMask, /* Interface mask */
                   asynInt32Mask | asynFloat64Mask ,  /* Interrupt mask */
                   0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                   1, /* Autoconnect */
                   0, /* Default priority */
                   0) /* Default stack size*/,
    sensor1Type{sensor1Type},
    sensor2Type{sensor2Type} {
    
    asynStatus status;
    
    printf("cbox2aDriver::cbox2aDriver XXX CONSTRUCTOR!!!!!\n");

    pasynPortName = epicsStrDup(asynPortName);


    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Creating Fetura+ controller %s to asyn %s\n", portName, asynPortName);

    //createParam(P_CB2A_CONTROLLER_String, asynParamInt32, &P_CB2A_CONTROLLER);
    createParam(P_CB2A_FLAGSLO_String,  asynParamInt32, &P_CB2A_FLAGSLO);
    createParam(P_CB2A_SERIALNR_String, asynParamInt32, &P_CB2A_SERIALNR);
    createParam(P_CB2A_SENSOR_1_String, asynParamInt32, &P_CB2A_SENSOR_1);
    createParam(P_CB2A_SENSOR_2_String, asynParamInt32, &P_CB2A_SENSOR_2);

/*
    status = pasynOctetSyncIO->connect(asynPortName, 0, &pasynUserController_, NULL);
    if (status) {
        //asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: cannot connect to fetura+ %s controller\n", driverName, functionName, portName);
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "cannot connect to fetura+ %s controller\n",  portName);
    } else {
        printf("OK!!!!!!!!!!!!!!\n");
        pasynOctetSyncIO->setInputEos(pasynUserController_, "", 0);
        pasynOctetSyncIO->setOutputEos(pasynUserController_, "", 0);
    }

  epicsThreadCreate("asynPoller", 
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)asynPollerC, (void *)this);
*/

 
  epicsThreadCreate("cbox2aTask", 
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)cbox2aTaskC, (void *)this);
/*
    printf("PARAMS: %s %s %s\n", CB2A_CONTROLLER_String, CB2A_SENSOR_1_String, CB2A_SENSOR_2_String);
    printf("PARAMS: %d %d %d\n", CB2A_CONTROLLER, CB2A_SENSOR_1, CB2A_SENSOR_2);
    */

}


void cbox2aDriver::report(FILE *fp, int level) {
    asynPortDriver::report(fp, level);
}

/*
asynStatus cbox2aDriver::readInt32(asynUser *pasynUser, epicsInt32 *value) {
    //int function = pasynUser->reason;

    //asynStatus status;
    int function;
    const char *paramName;
    int addr;
    asynStatus status = asynSuccess;
    epicsTimeStamp timeStamp; getTimeStamp(&timeStamp);
    static const char *functionName = "readInt32";
    
    status = parseAsynUser(pasynUser, &function, &addr, &paramName); 
    printf("XXXXXXXX READINT32 %d %d %s\n", function, addr, paramName);
    //status = (asynStatus) getIntegerParam(addr, function, value);


    printf("BLAAAAAAAAAAAAAA READINT32 %d\n", function);
    return asynPortDriver::readInt32(pasynUser, value);
}
*/


/** Called when asyn clients call pasynInt32->write().
  * This function sends a signal to the simTask thread if the value of P_Run has changed.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus cbox2aDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName;
    const char* functionName = "writeInt32";

    printf("AAAAAAAAAAAAAAAAAAAAA\n"); 

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setIntegerParam(function, value);
    
    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);

    printf("AAAAAAA %d %s %d\n", status, paramName, value);

    if (function == P_CB2A_SENSOR_1) {
        printf("P_CB2A_SENSOR_1\n");
    } else if (function == P_CB2A_SENSOR_2) {
        printf("P_CB2A_SENSOR_2\n");
        //setTimePerDiv();
    } else {
        /* All other parameters just get set in parameter list, no need to
         * act on them here */
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%d", 
                  driverName, functionName, status, function, paramName, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%d\n", 
              driverName, functionName, function, paramName, value);
    return status;
}

void cbox2aDriver::cbox2aTask() {
    static const char *functionName = "cbox2aTask";
    asynStatus status;
    asynUser *pasynUser;
    asynInterface *pasynInterface;
    asynOctet *pasynOctet;
    void *octetPvt;
    char charData[256];
    size_t nRequested=10;
    size_t nRead;
    int eomReason;
    int val=0;

    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->timeout = 10;
    status = pasynManager->connectDevice(pasynUser, pasynPortName, 0);

    pasynInterface = pasynManager->findInterface(pasynUser, asynOctetType, 1);
    if(!pasynInterface) {;
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: findInterface failed for asynOctet, status=%d\n",
            driverName, functionName, status);
    }
    pasynOctet = (asynOctet *)pasynInterface->pinterface;
    octetPvt = pasynInterface->drvPvt;

/*
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->timeout = 10;
    //pasynUser->timeout = TetrAMM_TIMEOUT;
    status = pasynManager->connectDevice(pasynUser, pasynPortName, 0);
    if(status!=asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: connectDevice failed, status=%d\n",
            driverName, functionName, status);
    }

    pasynOctetSyncIO->setInputEos(pasynUser, "", 0);
    pasynOctetSyncIO->setOutputEos(pasynUser, "", 0);


    pasynInterface = pasynManager->findInterface(pasynUser, asynOctetType, 1);
    if(!pasynInterface) {;
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: findInterface failed for asynOctet, status=%d\n",
            driverName, functionName, status);
    }
    pasynOctet = (asynOctet *)pasynInterface->pinterface;
    octetPvt = pasynInterface->drvPvt;
*/

    getIntegerParam(P_CB2A_SENSOR_1, &val);
    printf("VAL IS %d\n",val);
    getIntegerParam(P_CB2A_SENSOR_2, &val);
    printf("VAL IS %d\n",val);


    while (1) {

            printf("AAAAAAAA\n");
            pasynManager->lockPort(pasynUser);
            status = pasynOctet->read(octetPvt, pasynUser, charData, nRequested, 
                                      &nRead, &eomReason);

            pasynManager->unlockPort(pasynUser);

			lock();
            printf("BBBBBBBB %d\n",nRead);
            setIntegerParam(P_CB2A_SENSOR_1, nRead);
            callParamCallbacks();
			unlock();

    }

/*
    char buffer[1024];
    size_t read;
    int reason;
    int counter=0;
    while (1) {
        printf("POLLER!!!\n");
        //lock(); // IS THIS SUPPOSED THE LOCK TO BE USED?! OR SHOULD I CREATE A SPECIFIC LOCK FOR pasynUserController_ ?!?!?!
        pasynUserController_->lock();
        pasynOctetSyncIO->read(pasynUserController_, buffer, 1024, 10, &read, &reason);
        
        setIntegerParam(0, CB2A_SENSOR_1, counter++);
        //pAxis->setIntegerParam(motorStatusDone_, 0);

        callParamCallbacks();
        
        pasynUserController_->unlock();
        //unlock();

        printf("%d %c\n",read,buffer[0]);
    }
*/
}


static void cbox2aTaskC(void *drvPvt) {
    cbox2aDriver *pController = (cbox2aDriver*)drvPvt;
    pController->cbox2aTask();
}




extern "C" {

/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int cbox2aDriverConfigure(const char *portName, const char *asynPortName, int sensor1Type, int sensor2Type) {
    new cbox2aDriver(portName, asynPortName, sensor1Type, sensor2Type);
    return asynSuccess;
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "Port name",      iocshArgString };
static const iocshArg initArg1 = { "Asyn port name", iocshArgString };
static const iocshArg initArg2 = { "sensor 1 type",  iocshArgInt };
static const iocshArg initArg3 = { "sensor 2 type",  iocshArgInt };
static const iocshArg * const initArgs[] = { &initArg0,
                                             &initArg1,
                                             &initArg2,
                                             &initArg3 };
static const iocshFuncDef initFuncDef = { "cbox2aDriverConfigure", 4, initArgs };
static void initCallFunc(const iocshArgBuf *args) {
    cbox2aDriverConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
}

void MicroEpsilonCBox2ARegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(MicroEpsilonCBox2ARegister);

}


/*
 * TO DO:
 * - mbbo of sensor types: None, IDL1420-50
 * - ao of range: 50.0
 */
