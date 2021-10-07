# Micro-Epsilon CBox/2A laser-controller support

# Configure asyn network device
drvAsynIPPortConfigure("CBOXCTRL", "192.168.100.10:1024")

# Configure CBox/2A driver to device
cbox2aDriverConfigure("CBOX", "CBOXCTRL")

# Load records
dbLoadTemplate("cbox.substitutions")

