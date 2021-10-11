/*
FILENAME...   mecbox2a.cpp
USAGE...      Asyn driver support for the Micro-Epsilon C-Box/2A laser distance sensor

Jose G.C. Gabadinho
August 2021
*/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "mecbox2a.h"

#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <alarm.h>

#include "asynOctet.h"



#define DEVICE_TIMEOUT 10
#define TASK_DISCONNECTED_SLEEP 1.0

#define BUFFER_SIZE 3600



static const char *driverName = "meCBox2A";

static void cbox2aTaskC(void *drvPvt);



/*
Micro-Epsilon C-Box/2A frame description when paired with optoNCDT-1420:

0x53 0x41 0x45 0x4D              # preamble = SAEM

0x?? 0x?? 0x?? 0x??              # order number
0x?? 0x?? 0x?? 0x??              # serial number

0x?? 0x?? 0x?? 0x??              # flags 1
0x?? 0x?? 0x?? 0x??              # flags 2

0x?? 0x??                        # number of frames
0x?? 0x?? 0x?? 0x??              # frame counter

0x?? 0x??                        # ?

0x?? 0x??                        # sensor 1 value
0x?? 0x??                        # sensor 1 state

0x?? 0x??                        # sensor 2 value
0x?? 0x??                        # sensor 2 state

Total = 36 bytes
*/

/*
Simulation of a serial port:

http://epics.isis.stfc.ac.uk/doxygen/lvDCOM.old/html/lvDCOMDriver_8h_source.html
http://epics.isis.stfc.ac.uk/doxygen/lvDCOM.old/html/lvDCOMDriver_8cpp_source.html
https://stackoverflow.com/questions/52187/virtual-serial-port-for-linux

$ socat -d -d pty,raw,echo=0 pty,raw,echo=0
*/



/** Constructor for the cbox2aDriver class.
  *
  * \param[in] portName The name of the asyn port driver to be created
  * \param[in] asynPortName The name of the asyn communication port to the device
  */
cbox2aDriver::cbox2aDriver(const char *portName, const char *asynPortName):
    asynPortDriver(portName, 
                   1, /* maxAddr */ 
                   asynInt32Mask | asynFloat64Mask | asynDrvUserMask, /* Interface mask */
                   asynInt32Mask | asynFloat64Mask,  /* Interrupt mask */
                   0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                   1, /* Autoconnect */
                   0, /* Default priority */
                   0) /* Default stack size*/ {
    const char* function_name = "cbox2aDriver";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:%s Creating Micro Epsilon CBox 2A %s to asyn %s\n", driverName, function_name, portName, asynPortName);

    this->pasynPortName = epicsStrDup(asynPortName);

    createParam(P_CB2A_FLAGS1_String,  asynParamInt32, &P_CB2A_FLAGS1);
    createParam(P_CB2A_SERIALNR_String, asynParamInt32, &P_CB2A_SERIALNR);
    createParam(P_CB2A_FRAMECOUNTER_String, asynParamFloat64, &P_CB2A_FRAMECOUNTER);

    createParam(P_CB2A_TYPE_1_String,   asynParamInt32, &P_CB2A_TYPE_1);
    createParam(P_CB2A_RANGE_1_String,  asynParamInt32, &P_CB2A_RANGE_1);
    createParam(P_CB2A_SENSOR_1_String, asynParamInt32, &P_CB2A_SENSOR_1);
    createParam(P_CB2A_STATE_1_String,  asynParamInt32, &P_CB2A_STATE_1);

    createParam(P_CB2A_TYPE_2_String,   asynParamInt32, &P_CB2A_TYPE_2);
    createParam(P_CB2A_RANGE_2_String,  asynParamInt32, &P_CB2A_RANGE_2);
    createParam(P_CB2A_SENSOR_2_String, asynParamInt32, &P_CB2A_SENSOR_2);
    createParam(P_CB2A_STATE_2_String,  asynParamInt32, &P_CB2A_STATE_2);
 
    epicsThreadCreate("cbox2aTask", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), (EPICSTHREADFUNC)cbox2aTaskC, (void *)this);
}



/** Reports on status of the driver.
  *
  * \param[in] fp The file pointer on which report information will be written
  * \param[in] level The level of report detail desired
  */
void cbox2aDriver::report(FILE *fp, int level) {
    asynPortDriver::report(fp, level);
}



/** Called when asyn clients call pasynInt32->write().
  * Updates the sensor range record depending on its type.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  *
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. 
  *
  * \return Status of asyn library calls
  */
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



/** Debug method to print the state of the buffer holding the data received from the device.
  *
  * \param[in] buffer     Buffer of bytes
  * \param[in] buffer_len Valid number of read bytes
  */
void cbox2aDriver::bufferDebugPrint(unsigned char *buffer, size_t buffer_len) {
    size_t count, base=0;

    printf("--- start --- len=%ld\n", buffer_len);
    while (buffer_len > 0) {
        count = 10;
        if (count > buffer_len) {
            count = buffer_len;
        }

        for (unsigned int i=0; i<count; ++i) {
            printf("%02x ", buffer[base+i] & 0xFF);
        }
        printf("\n");

        base += count;
        buffer_len -= count;
    }
    printf("--- end--- \n");
}



/** Thread function to handle communication to the device.
  *
  * Exits immediatelly if cannot connect to the device, otherwise enter an infinite loop expecting data from the device.
  * Data frames are appended to a ring buffer.
  * After reading data from the device, if there's enough data for a frame, it scans the last part of the ring buffer,
  * searching for the last frame. If preamble is found, extracts the data and injects it on the records.
  */
void cbox2aDriver::cbox2aTask() {
    static const char *function_name = "cbox2aTask";
    asynStatus status;
    asynUser *pasynUser;
    asynInterface *pasynInterface;
    asynOctet *pasynOctet;
    void *octetPvt;

	int is_connected;
    size_t bytes_read;
    int eom_reason;
    bool got_timeout;

    unsigned char *buffer;
    unsigned char frame_buffer[FRAME_SIZE];

    size_t buffer_next_write_idx;

    struct extractedData extracted_data;
    const unsigned char *start_of_data;
    bool data_valid;

    buffer = (unsigned char *)malloc(BUFFER_SIZE);
	if (!buffer) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: failed to allocate buffer memory for %s\n", driverName, function_name, this->pasynPortName);
        return;
    }

    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->timeout = DEVICE_TIMEOUT;
    status = pasynManager->connectDevice(pasynUser, this->pasynPortName, 0);
	if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: connectDevice failed for %s status=%d\n", driverName, function_name, this->pasynPortName, status);
        return;
    }
    pasynInterface = pasynManager->findInterface(pasynUser, asynOctetType, 1);
    if (!pasynInterface) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: findInterface failed for asynOctet, status=%d\n", driverName, function_name, status);
        return;
    }

    pasynOctet = (asynOctet *)pasynInterface->pinterface;
    octetPvt = pasynInterface->drvPvt;

    buffer_next_write_idx = 0; // Index of where to write the next byte coming from the device
    while (true) {
        pasynManager->isConnected(pasynUser, &is_connected);
		if (is_connected) {
			pasynManager->lockPort(pasynUser);
            status = pasynOctet->read(octetPvt, pasynUser, (char *)frame_buffer, FRAME_SIZE, &bytes_read, &eom_reason);

            // Copy frame to buffer
            if (bytes_read == 0) {
                got_timeout = true;
                pasynUser->auxStatus = asynTimeout;

            } else {
                got_timeout = false;
                pasynUser->auxStatus = asynSuccess;

                if (bytes_read != FRAME_SIZE) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: read %ld bytes from %s, was expecting %d (data frame misconfigured perhaps?)\n", driverName, function_name, bytes_read, this->pasynPortName, FRAME_SIZE);
                }
                ringBufferAppend(buffer, BUFFER_SIZE, &buffer_next_write_idx, frame_buffer, bytes_read);
            }

            // For debug purposes: print content of buffer
            //bufferDebugPrint(buffer, buffer_next_write_idx);

            // Scan current state of buffer for preamble
            start_of_data = findPreamble(buffer, buffer_next_write_idx);
            data_valid = extractData(start_of_data, &extracted_data);

			pasynManager->unlockPort(pasynUser);

			lock();

            if (got_timeout) {
                invalidateEverything();
            } else if (data_valid) {
                // Inject extracted values into records, drop all alarms
    			setIntegerParam(P_CB2A_FLAGS1, extracted_data.flags1);
                setParamAlarmStatus(P_CB2A_FLAGS1, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_FLAGS1, epicsSevNone);

    			setIntegerParam(P_CB2A_SERIALNR, extracted_data.serial_number);
                setParamAlarmStatus(P_CB2A_SERIALNR, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_SERIALNR, epicsSevNone);

                setDoubleParam(P_CB2A_FRAMECOUNTER, extracted_data.frame_counter);
                setParamAlarmStatus(P_CB2A_FRAMECOUNTER, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_FRAMECOUNTER, epicsSevNone);

    			setIntegerParam(P_CB2A_SENSOR_1, extracted_data.sensor1);
                setParamAlarmStatus(P_CB2A_SENSOR_1, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_SENSOR_1, epicsSevNone);
    			setIntegerParam(P_CB2A_STATE_1, extracted_data.state1);
                setParamAlarmStatus(P_CB2A_STATE_1, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_STATE_1, epicsSevNone);

    			setIntegerParam(P_CB2A_SENSOR_2, extracted_data.sensor2);
                setParamAlarmStatus(P_CB2A_SENSOR_2, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_SENSOR_2, epicsSevNone);
    			setIntegerParam(P_CB2A_STATE_2, extracted_data.state2);
                setParamAlarmStatus(P_CB2A_STATE_2, epicsAlarmNone);
                setParamAlarmSeverity(P_CB2A_STATE_2, epicsSevNone);

                // Reset buffer parameters
                buffer_next_write_idx = 0;
            }

			callParamCallbacks();
			unlock();
		} else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: isConnected failed for %s\n", driverName, function_name, this->pasynPortName);

            lock();
            invalidateEverything();
            callParamCallbacks();
            unlock();

			epicsThreadSleep(TASK_DISCONNECTED_SLEEP);

            // Reset buffer parameters
            buffer_next_write_idx = 0;
		}
    }
}



/** Debug method to print the state of the buffer holding the data received from the device.
  *
  * \param[in] ring_buffer Ring buffer holding the data read from the device, until scanned
  * \param[in] ring_buffer_size Ring buffer size
  * \param[in,out] ring_buffer_next_write_idx Valid bytes inside ring_buffer; updated after data copied from read_buffer to ring_buffer
  * \param[in] read_buffer Buffer of bytes read from the device
  * \param[in] bytes_read Valid number of read bytes
  */
void cbox2aDriver::ringBufferAppend(unsigned char *ring_buffer, size_t ring_buffer_size, size_t *ring_buffer_next_write_idx, const unsigned char *read_buffer, size_t bytes_read) {
    size_t buffer_available, buffer_remain;

    *ring_buffer_next_write_idx = (*ring_buffer_next_write_idx) % ring_buffer_size; // Make sure we're in the proper range

    buffer_available = ring_buffer_size - *ring_buffer_next_write_idx;

    if (bytes_read <= buffer_available) {
        memcpy(&(ring_buffer[*ring_buffer_next_write_idx]), read_buffer, bytes_read);

        // Update where to write next data
        *ring_buffer_next_write_idx = (*ring_buffer_next_write_idx) + bytes_read;
    } else {
        memcpy(&(ring_buffer[*ring_buffer_next_write_idx]), read_buffer, buffer_available);
        buffer_remain = bytes_read - buffer_available;
        memcpy(ring_buffer, &(read_buffer[buffer_available]), buffer_remain);

        // Update where to write next data
        *ring_buffer_next_write_idx = buffer_remain;
    }
}



/** Finds the preamble from the data protocol from a buffer of characters read from the device.
  *
  * \param[in] buffer     Buffer of bytes
  * \param[in] buffer_len Valid number of read bytes
  *
  * \return Pointer to the start of the data frame, or NULL if preamble not found
  */
const unsigned char * cbox2aDriver::findPreamble(const unsigned char *buffer, size_t buffer_len) {
    const unsigned char *preamble_ptr = NULL;
    size_t buffer_read_idx, buffer_preamble_idx, preamble_lookout;

    if (buffer_len >= FRAME_SIZE) {
        buffer_read_idx = buffer_len - 2*FRAME_SIZE + 1;
        if (buffer_read_idx < 0) {
            buffer_read_idx = 0;
        }

        preamble_lookout = 0;
        for (buffer_preamble_idx=buffer_read_idx; buffer_preamble_idx<buffer_len; ++buffer_preamble_idx) {
            if (buffer[buffer_preamble_idx] == CBOX2A_PREAMBLE[preamble_lookout]) {
                preamble_lookout++;
            } else {
                preamble_lookout = 0;
            }

            if (preamble_lookout == PREAMBLE_LENGTH) {
                preamble_ptr= &(buffer[buffer_preamble_idx - PREAMBLE_LENGTH + 1]);
                break;
            }
        }
    }

    return preamble_ptr;
}



/** Extracts the values from the data frame and injects them in the given structure.
  *
  * \param[in] buffer   Data frame buffer (must start with SAEM and be at least 38 bytes)
  * \param[in] storage  Where to put the extracted values
  *
  * \return false if storage is NULL or if buffer invalid, otherwise true
  */
bool cbox2aDriver::extractData(const unsigned char *buffer, struct extractedData *storage) {
    unsigned int *raw32;
    unsigned short *raw16;

    if (!buffer) {
        return false;
    }
    if (!storage) {
        return false;
    }
    if (memcmp(buffer, CBOX2A_PREAMBLE, PREAMBLE_LENGTH) != 0) {
        return false;
    }

    // Extract serial number
    raw32 = (unsigned int *)&(buffer[DATAFRAME_SERIALNR_OFFSET]);
    storage->serial_number = *raw32;

    // Extract flags 1
    raw32 = (unsigned int *)&(buffer[DATAFRAME_FLAGS1_OFFSET]);
    storage->flags1 = *raw32;

    // Extract frame counter
    raw32 = (unsigned int *)&(buffer[DATAFRAME_FRAMECOUNTER_OFFSET]);
    storage->frame_counter = *raw32;

    // Extract sensor 1 value
    raw16 = (unsigned short *)&(buffer[DATAFRAME_SENSOR1_OFFSET]);
    storage->sensor1 = *raw16;

    // Extract sensor 1 state
    raw16 = (unsigned short *)&(buffer[DATAFRAME_STATE1_OFFSET]);
    storage->state1 = *raw16;

    // Extract sensor 2 value
    raw16 = (unsigned short *)&(buffer[DATAFRAME_SENSOR2_OFFSET]);
    storage->sensor2 = *raw16;

    // Extract sensor 2 state
    raw16 = (unsigned short *)&(buffer[DATAFRAME_STATE2_OFFSET]);
    storage->state2 = *raw16;

    return true;
}



/** Sets all asyn parameters to 0 and raise alarm to invalid and communication problems.
  */
void cbox2aDriver::invalidateEverything() {
    // Reset everything to 0 and raise alarms
	setIntegerParam(P_CB2A_FLAGS1, 0);
    setParamAlarmStatus(P_CB2A_FLAGS1, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_FLAGS1, epicsSevInvalid);

    setIntegerParam(P_CB2A_SERIALNR, 0);
    setParamAlarmStatus(P_CB2A_SERIALNR, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_SERIALNR, epicsSevInvalid);

    setDoubleParam(P_CB2A_FRAMECOUNTER, 0.0);
    setParamAlarmStatus(P_CB2A_FRAMECOUNTER, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_FRAMECOUNTER, epicsSevInvalid);

    setIntegerParam(P_CB2A_SENSOR_1, 0);
    setParamAlarmStatus(P_CB2A_SENSOR_1, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_SENSOR_1, epicsSevInvalid);
    setIntegerParam(P_CB2A_STATE_1, 0);
    setParamAlarmStatus(P_CB2A_STATE_1, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_STATE_1, epicsSevInvalid);

	setIntegerParam(P_CB2A_SENSOR_2, 0);
    setParamAlarmStatus(P_CB2A_SENSOR_2, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_SENSOR_2, epicsSevInvalid);
	setIntegerParam(P_CB2A_STATE_2, 0);
    setParamAlarmStatus(P_CB2A_STATE_2, epicsAlarmComm);
    setParamAlarmSeverity(P_CB2A_STATE_2, epicsSevInvalid);
}



/** EPICS iocsh callable function to call constructor for the cbox2aDriver class.
  *
  * \param[in] portName The name of the asyn port driver to be created.
  */
static void cbox2aTaskC(void *drvPvt) {
    cbox2aDriver *pController = (cbox2aDriver*)drvPvt;
    pController->cbox2aTask();
}



extern "C" {

/** EPICS iocsh callable function to call constructor for the cbox2aDriver class.
  *
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] asynPortName The name of the asyn communication port to the device.
  *
  * \return Always asynSuccess
  */
int cbox2aDriverConfigure(const char *portName, const char *asynPortName) {
    new cbox2aDriver(portName, asynPortName);
    return asynSuccess;
}



/** Code for iocsh registration */
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

