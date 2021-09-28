#include <cstdio>

#include <epicsExport.h>

static void gtestRegister(void) {
    printf("gtestRegister\n");
}

extern "C" {
    epicsExportRegistrar(gtestRegister);
}

