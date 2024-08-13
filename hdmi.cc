#include <picobase/Clocks.h>
#include <picobase/Voltage.h>

VRegAndReset vreg;

[[gnu::noreturn]]
void start() {
    vreg.vreg().voltageSel() = VRegAndReset::VReg::k1v20;
}
