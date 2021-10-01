# Base image
FROM amd64/debian:oldstable

# Copy sed-patch files into image
COPY *.sed /tmp/

# Package dependencies
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        libreadline-dev \
        wget \
        re2c \
        vim \
        git-all

# Download EPICS, unpack, build it
RUN wget -P /tmp/ https://epics-controls.org/download/base/base-7.0.5.tar.gz && \
    mkdir -p /usr/local/epics && \
    tar xfz /tmp/base-7.0.5.tar.gz -C /usr/local/epics && \
    ln -s /usr/local/epics/base-7.0.5 /usr/local/epics/base && \
    cd /usr/local/epics/base && make

# Download synApps, unpack, patch configuration, build it
RUN wget -P /tmp/ https://epics.anl.gov/bcda/synApps/tar/synApps_6_1.tar.gz && \
    mkdir -p /usr/local/epics/modules && \
    tar xfz /tmp/synApps_6_1.tar.gz -C /usr/local/epics/modules && \
    ln -s /usr/local/epics/modules/synApps_6_1 /usr/local/epics/modules/synApps && \
    sed -i -f /tmp/synapps_release.sed /usr/local/epics/modules/synApps/support/configure/RELEASE && \
    cd /usr/local/epics/modules/synApps/support && make release && \
    sed -i -f /tmp/asyn_release.sed /usr/local/epics/modules/synApps/support/asyn-R4-36/configure/RELEASE && \
    sed -i -f /tmp/calc_release.sed /usr/local/epics/modules/synApps/support/calc-R3-7-3/configure/RELEASE && \
    sed -i -f /tmp/sscan_release.sed /usr/local/epics/modules/synApps/support/sscan-R2-11-3/configure/RELEASE && \
    cd /usr/local/epics/modules/synApps/support && make

# Download latest Micro-Epsilon CBox/2A driver, patch configuration
RUN cd /usr/local/epics && git clone https://github.com/gabadinho/meCBox2A.git && \
    sed -i -f /tmp/epicsbase_release.sed /usr/local/epics/meCBox2A/configure/RELEASE

# Patch IOC configuration
RUN sed -i -f /tmp/cbox_release.sed /usr/local/epics/meCBox2A/iocs/cboxIOC/configure/RELEASE && \
    sed -i -f /tmp/ip_cboxcmd.sed /usr/local/epics/meCBox2A/iocs/cboxIOC/iocBoot/ioccbox/cbox.cmd

# Build driver and IOC
RUN cd /usr/local/epics/meCBox2A && make && \
    cd /usr/local/epics/meCBox2A/iocs/cboxIOC && make

# Start IOC
CMD cd /usr/local/epics/meCBox2A/iocs/cboxIOC/iocBoot/ioccbox && ../../bin/linux-x86_64/cbox st.cmd

