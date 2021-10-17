# meCBox2A
EPICS asyn support for the Micro-Epsilon C-Box/2A laser-sensor controller.

Please refer to the provided example IOC, cboxIOC, for specifics.

### Usage, in short:
1. Create ethernet asyn port:
	```drvAsynSerialPortConfigure("CBOXCTRL", "192.168.100.10:1024")```
2. Configure Micro-Epsilon CBox/2A to above asyn port:
	```cbox2aDriverConfigure("CBOX", "CBOXCTRL")```
3. Load the asyn records:
	```dbLoadTemplate("cbox.substitutions")```

