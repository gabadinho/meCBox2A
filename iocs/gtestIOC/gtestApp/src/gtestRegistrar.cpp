#include <cstdio>

#include <epicsStdio.h>
#include <epicsExport.h>
#include <initHooks.h>
#include <epicsExit.h>

#include <gtest/gtest.h>

void gtestHook(initHookState state) {
    if (state == initHookAtIocBuild) {
        epicsStdoutPrintf("gtestIOC: initializing GoogleTest\n");
        ::testing::InitGoogleTest();
    } else if (state == initHookAfterDatabaseRunning) {
        epicsStdoutPrintf("gtestIOC: running GoogleTest tests\n");
        RUN_ALL_TESTS();
    } else if (state == initHookAfterIocRunning) {
        epicsStdoutPrintf("gtestIOC: exiting...\n");
        epicsExit(0);
    }
}

static void gtestRegister(void) {
    initHookRegister(gtestHook);
}

extern "C" {
    epicsExportRegistrar(gtestRegister);
}

