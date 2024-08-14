#include <picobase/Boot.h>
#include <picobase/Clocks.h>
#include <picobase/Voltage.h>

namespace sys {

// Basic RP2040 and ARM startup stuff

// Instantiate register blocks
VRegAndReset gVRegAndReset {};
SysPLL gSysPLL {};
Clock0 gClock0 {};

void start() {
    // Boost voltage a little, from 1.1v to 1.2v, since we're about to overclock
    gVRegAndReset.vreg().voltageSel() = VRegAndReset::VReg::VoltageSel::k1v20;

    // Set clock to 252MHz.  This will be our pixel clock as well

    u32* x = (u32*)0x42424242;
    u32 c = 0;
    while (true) { *x = c++; }

    __builtin_unreachable();  // `start` doesn't return
}

}  // namespace sys
