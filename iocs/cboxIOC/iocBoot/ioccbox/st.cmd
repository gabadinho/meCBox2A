#!../../bin/linux-x86_64/cbox

#- You may have to change cbox to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/cbox.dbd"
cbox_registerRecordDeviceDriver pdbbase

drvAsynSerialPortConfigure("SOCAT_PORT", "/dev/pts/3")
# Commands to /dev/pts/4 get forwarded to /dev/pts/3

cbox2aDriverConfigure("CBOX2A", "SOCAT_PORT")

## Load record instances
dbLoadRecords("${MECBOX2A}/db/microepsilon_cbox2a.template", "P=MICROEPSILON:,R=CBOX2A,PORT=CBOX2A,SCAN=Passive,ADDR=0,OMAX=0,IMAX=0")

cd "${TOP}/iocBoot/${IOC}"
iocInit
