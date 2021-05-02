#!../../bin/linux-x86_64/cbox

#- You may have to change cbox to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/cbox.dbd"
cbox_registerRecordDeviceDriver pdbbase

drvAsynSerialPortConfigure("FAKETTY", "/dev/pts/4")
cbox2aDriverConfigure("MYPORT", "FAKETTY", 2, 3)

## Load record instances
#dbLoadRecords("db/xxx.db","user=jg")

dbLoadRecords("db/microepsilon_cbox2a.template", "P=P:,R=R,PORT=MYPORT,SCAN=Passive,ADDR=0,OMAX=0,IMAX=0")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=jg"
