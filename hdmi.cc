#include <picobase/Boot.h>
#include <picobase/Clocks.h>
#include <picobase/Voltage.h>

// Basic RP2040 and ARM startup stuff

// Instantiate register blocks
VRegAndReset vregAndReset;

[[gnu::noreturn]]
void start() {
    // Boost voltage a little, from 1.1v to 1.2v, since we're about to overclock
    vregAndReset.vreg().voltageSel() = VRegAndReset::VReg::VoltageSel::k1v20;

    // Set clock to 252MHz.  This will be our pixel clock as well

    while (true) {}
}
