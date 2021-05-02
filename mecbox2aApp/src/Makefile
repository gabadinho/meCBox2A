TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += mecbox2a

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install mecbox2a.dbd into <top>/dbd
DBD += mecbox2a.dbd

# specify all source files to be compiled and added to the library
mecbox2a_SRCS += mecbox2a.cpp
#INC += mecbox2a.h

#mecbox2a_LIBS += asyn
mecbox2a_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

