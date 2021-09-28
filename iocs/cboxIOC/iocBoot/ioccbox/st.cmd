#!../../bin/linux-x86_64/cbox

#- You may have to change cbox to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/cbox.dbd"
cbox_registerRecordDeviceDriver pdbbase

cd "${TOP}/iocBoot/${IOC}"

##
< cbox.cmd

iocInit
