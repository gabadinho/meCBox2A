TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = cbox
# cbox.dbd will be created and installed
DBD += cbox.dbd

# cbox.dbd will be made up from these files:
cbox_DBD += base.dbd

# Include dbd files from all support applications:
cbox_DBD += asyn.dbd
cbox_DBD += drvAsynSerialPort.dbd
cbox_DBD += drvAsynIPPort.dbd
cbox_DBD += mecbox2a.dbd

# Add all the support libraries needed by this IOC
cbox_LIBS += mecbox2a
cbox_LIBS += asyn

# cbox_registerRecordDeviceDriver.cpp derives from cbox.dbd
cbox_SRCS += cbox_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
cbox_SRCS_DEFAULT += cboxMain.cpp
cbox_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#cbox_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
cbox_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

