#include "mecbox2a.h"

#include <stdio.h>

#include "iocsh.h"
#include "epicsExport.h"
#include "epicsString.h"
#include "epicsThread.h"
#include "alarm.h"

#include "asynOctet.h"

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



/** Constructor for the cbox2aDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] asynPortName The name of the asyn communication port to the device. */
cbox2aDriver::cbox2aDriver(const char *portName, const char *asynPortName):
    asynPortDriver(portName, 
                   1, /* maxAddr */ 
                   asynInt32Mask | asynFloat64Mask  | asynDrvUserMask, /* Interface mask */
                   asynInt32Mask | asynFloat64Mask ,  /* Interrupt mask */
                   0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                   1, /* Autoconnect */
                   0, /* Default priority */
                   0) /* Default stack size*/ {
    const char* function_name = "cbox2aDriver";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:%s Creating Micro Epsilon CBox 2A %s to asyn %s\n", driverName, function_name, portName, asynPortName);

    pasynPortName = epicsStrDup(asynPortName);

    createParam(P_CB2A_FLAGSLO_String,  asynParamInt32, &P_CB2A_FLAGSLO);
    createParam(P_CB2A_SERIALNR_String, asynParamInt32, &P_CB2A_SERIALNR);

    createParam(P_CB2A_TYPE_1_String,   asynParamInt32, &P_CB2A_TYPE_1);
    createParam(P_CB2A_RANGE_1_String,  asynParamInt32, &P_CB2A_RANGE_1);
    createParam(P_CB2A_SENSOR_1_String, asynParamInt32, &P_CB2A_SENSOR_1);

    createParam(P_CB2A_TYPE_2_String,   asynParamInt32, &P_CB2A_TYPE_2);
    createParam(P_CB2A_RANGE_2_String,  asynParamInt32, &P_CB2A_RANGE_2);
    createParam(P_CB2A_SENSOR_2_String, asynParamInt32, &P_CB2A_SENSOR_2);
 
    epicsThreadCreate("cbox2aTask", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), (EPICSTHREADFUNC)cbox2aTaskC, (void *)this);
}



/** Wrapper for the asynPortDriver report() method.
  */
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
  * Updates the sensor range record depending on its type.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. 
  * \return Status of asyn library calls, */
asynStatus cbox2aDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status;
    const char *param_name;
    const char* function_name = "writeInt32";
    epicsInt32 sensor_range;

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setIntegerParam(function, value);
	if (status == asynSuccess) {
		/* Fetch the parameter string name for possible use in debugging */
		getParamName(function, &param_name);

		if (function == P_CB2A_TYPE_1) {
			getIntegerParam(P_CB2A_TYPE_1, &sensor_range);
			setIntegerParam(P_CB2A_RANGE_1, sensor_range);
		} else if (function == P_CB2A_TYPE_2) {
			getIntegerParam(P_CB2A_TYPE_2, &sensor_range);
			setIntegerParam(P_CB2A_RANGE_2, sensor_range);
		}
    
		/* Do callbacks so higher layers see changes */
		status = (asynStatus)callParamCallbacks();    
		if (status) {
			epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, "%s:%s: status=%d, function=%d, name=%s, value=%d", driverName, function_name, status, function, param_name, value);
		} else {
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, "%s:%s: function=%d, name=%s, value=%d\n", driverName, function_name, function, param_name, value);
		}
	}

    return status;
}



/** Thread function to handle communication to the device.
  * DETAILS HERE!!! */
/*
 * IF OVERFLOW, SET RECORD TO epicsSevMajor,epicsAlarmComm and exit thread!!!
 */
void cbox2aDriver::cbox2aTask() {
    static const char *function_name = "cbox2aTask";
    asynStatus status;
    asynUser *pasynUser;
    asynInterface *pasynInterface;
    asynOctet *pasynOctet;
	int is_connected;

    void *octetPvt;
    char charData[256];
    size_t nRequested=10;
    size_t nRead;
    int eomReason;
    int val=0;

    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->timeout = 10; // CREATE CONSTANT FOR THIS!!!
    status = pasynManager->connectDevice(pasynUser, pasynPortName, 0);
	if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: connectDevice failed for %s status=%d\n", driverName, function_name, pasynPortName, status);
        return;
    }
    pasynInterface = pasynManager->findInterface(pasynUser, asynOctetType, 1);
    if (!pasynInterface) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: findInterface failed for asynOctet, status=%d\n", driverName, function_name, status);
        return;
    }

    pasynOctet = (asynOctet *)pasynInterface->pinterface;
    octetPvt = pasynInterface->drvPvt;

    while (true) {
        pasynManager->isConnected(pasynUser, &is_connected);
		if (is_connected) {
			//printf("AAAAAAAA\n");
			pasynManager->lockPort(pasynUser);
			status = pasynOctet->read(octetPvt, pasynUser, charData, nRequested,  &nRead, &eomReason);

			/*
			if (nRead==0) pasynUser->auxStatus=1;
			else  pasynUser->auxStatus=0;
			*/

			pasynManager->unlockPort(pasynUser);

			lock();
			printf("BBBBBBBB %d\n",nRead);
			setIntegerParam(P_CB2A_SENSOR_1, nRead);
			if (nRead==0) {
                setParamAlarmStatus(P_CB2A_SENSOR_1, epicsAlarmRead);
                setParamAlarmSeverity(P_CB2A_SENSOR_1, epicsSevMajor);
			} else {
                setParamAlarmStatus(P_CB2A_SENSOR_1, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_SENSOR_1, epicsSevNone);
			}

			callParamCallbacks();
			unlock();


		} else {
			printf("AAAAAAAA\n");
            lock();
			setIntegerParam(P_CB2A_SENSOR_1, 0);
            setParamAlarmStatus(P_CB2A_SENSOR_1, epicsAlarmComm);
            setParamAlarmSeverity(P_CB2A_SENSOR_1, epicsSevInvalid);
			setIntegerParam(P_CB2A_SENSOR_2, 0);
            setParamAlarmStatus(P_CB2A_SENSOR_2, epicsAlarmComm);
            setParamAlarmSeverity(P_CB2A_SENSOR_2, epicsSevInvalid);
            callParamCallbacks();
            unlock();
			epicsThreadSleep(1.0); // GET CONSTANT FOR THIS!!!
		}
    }
}



/** EPICS iocsh callable function to call constructor for the cbox2aDriver class.
  * \param[in] portName The name of the asyn port driver to be created. */
static void cbox2aTaskC(void *drvPvt) {
    cbox2aDriver *pController = (cbox2aDriver*)drvPvt;
    pController->cbox2aTask();
}



extern "C" {

/** EPICS iocsh callable function to call constructor for the cbox2aDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] asynPortName The name of the asyn communication port to the device.
  * \return Always asynSuccess */
int cbox2aDriverConfigure(const char *portName, const char *asynPortName) {
    new cbox2aDriver(portName, asynPortName);
    return asynSuccess;
}



/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "Port name",      iocshArgString };
static const iocshArg initArg1 = { "Asyn port name", iocshArgString };
static const iocshArg * const initArgs[] = { &initArg0,
                                             &initArg1 };
static const iocshFuncDef initFuncDef = { "cbox2aDriverConfigure", 2, initArgs };
static void initCallFunc(const iocshArgBuf *args) {
    cbox2aDriverConfigure(args[0].sval, args[1].sval);
}

void MicroEpsilonCBox2ARegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(MicroEpsilonCBox2ARegister);

}
