#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/dma.h>
#include <hardware/vreg.h>
#include <pico.h>
#include <pico/multicore.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "boards/pico.h"
#include "build/hdmi.pio.h"
#include "pixel.h"

#define PURE __attribute__((__pure__))

void initGPIOOutput(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void initClock() { set_sys_clock_khz(252000, false); }

void initGPIO() {
    for (int i = 0; i < 32; i++) { initGPIOOutput(i); }
}

void initSM(int sm, unsigned offset) {
    int gpio = sm * 2;
    hdmi_pio_init(pio0, sm, offset, gpio);
    hw_write_masked(&pads_bank0_hw->io[gpio], (0 << PADS_BANK0_GPIO0_DRIVE_LSB),
                    PADS_BANK0_GPIO0_DRIVE_BITS |
                            PADS_BANK0_GPIO0_SLEWFAST_BITS |
                            PADS_BANK0_GPIO0_IE_BITS);
    gpio_set_outover(gpio, GPIO_OVERRIDE_NORMAL);
    hw_write_masked(
            &pads_bank0_hw->io[gpio + 1], (0 << PADS_BANK0_GPIO0_DRIVE_LSB),
            PADS_BANK0_GPIO0_DRIVE_BITS | PADS_BANK0_GPIO0_SLEWFAST_BITS |
                    PADS_BANK0_GPIO0_IE_BITS);
    gpio_set_outover(gpio + 1, GPIO_OVERRIDE_NORMAL);
}

void initPIO() {
    pio_claim_sm_mask(pio0, 0x0f);
    auto offset = pio_add_program(pio0, &hdmi_program);
    initSM(0, offset);
    initSM(1, offset);
    initSM(2, offset);
    initSM(3, offset);
}

void dmaHandler();

int dmaChannels[4];

void initDMA(int sm) {
    int ch = dmaChannels[sm] = dma_claim_unused_channel(true);
    auto dmaConfig = dma_channel_get_default_config(ch);
    channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_32);
    channel_config_set_read_increment(&dmaConfig, true);
    channel_config_set_write_increment(&dmaConfig, false);
    channel_config_set_dreq(&dmaConfig, pio_get_dreq(pio0, sm, true));
    channel_config_set_bswap(&dmaConfig, false);
    dma_channel_configure(ch, &dmaConfig, &pio0_hw->txf[sm], nullptr, 80,
                          false);
    dma_channel_set_irq0_enabled(ch, true);
}

void initDMA() {
    initDMA(0);
    initDMA(1);
    initDMA(2);
    initDMA(3);
}

void display();

uint32_t core1Stack[1024];
__attribute__((__noreturn__)) void main1() {
    irq_set_mask_enabled(0xffffffff, false);
    irq_set_exclusive_handler(DMA_IRQ_0, dmaHandler);
    irq_set_enabled(DMA_IRQ_0, true);
    display();  // do initial trigger of DMA; `dmaHandler` will take over
                // afterwards.
    while (true) { tight_loop_contents(); }
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

// TMDS patterns go into these buffers:

// Clock signal only.  Wastes memory but should be exactly in-pace with the
// other buffers
Buffer clockPattern;

Buffer r0;
Buffer g0;
Buffer b0;

Buffer r1;
Buffer g1;
Buffer b1;

Buffer r2;
Buffer g2;
Buffer b2;

Buffer r3;
Buffer g3;
Buffer b3;

Buffers lists[4] = {
        {&r0, &g0, &b0}, {&r1, &g1, &b1}, {&r2, &g2, &b2}, {&r3, &g3, &b3}};

// Control buffers
Buffer empty;
Buffer vsync;
Buffer hvsync;

Buffer hblankCH0;
Buffer hblankCH1;
Buffer hblankCH2;

void initBuffers() {
    // clock
    for (int i = 0; i < 160; i++) { clockPattern.set(i, pxClock); }

    // control

    for (int i = 0; i < 160; i++) { empty.set(i, pxControl[0]); }
    for (int i = 0; i < 160; i++) { vsync.set(i, pxControl[2]); }

    for (int i = 0; i < 16; i++) { hvsync.set(i, pxControl[2]); }
    for (int i = 16; i < 112; i++) { hvsync.set(i, pxControl[3]); }
    for (int i = 112; i < 160; i++) { hvsync.set(i, pxControl[2]); }

    for (int i = 0; i < 160; i++) { hblankCH1.set(i, pxControl[0]); }
    for (int i = 0; i < 160; i++) { hblankCH2.set(i, pxControl[0]); }

    for (int i = 0; i < 16; i++) { hblankCH0.set(i, pxControl[0]); }
    for (int i = 16; i < 112; i++) { hblankCH0.set(i, pxControl[1]); }
    for (int i = 112; i < 160; i++) { hblankCH0.set(i, pxControl[0]); }
}

unsigned chunk = 0;
unsigned totalChunks = 0;
unsigned line = 0;
unsigned totalLines = 0;
unsigned totalFrames = 0;

Buffers next = {&empty, &empty, &empty};

[[gnu::section(".time_critical.text")]]
void display() {
    dma_channel_set_read_addr(0, next.ch[0]->pxs, false);
    dma_channel_set_read_addr(1, next.ch[1]->pxs, false);
    dma_channel_set_read_addr(2, next.ch[2]->pxs, false);
    dma_channel_set_read_addr(3, clockPattern.pxs, false);
    dma_start_channel_mask(0x0f);
}

void advance();
void update();

[[gnu::section(".time_critical.text")]]
void dmaHandler() {
    // wait for all DMAs to signal they are done
    int x = dma_hw->intr;
    if ((x & 0x0f) == 0x0f) {
        // ack all 4
        dma_hw->ints0 = 0x0f;
        // trigger all DMAs
        display();
        advance();
        update();
    }
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

[[gnu::section(".time_critical.data")]]
uint8_t mono[320] = {
        10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
        10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
        10,   10,   10,   10,   10,   10,   10,   10,

        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24,
        0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34,
        0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f,
        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24,
        0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34,
        0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f,
        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24,
        0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34,
        0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f,
        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24,
        0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34,
        0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f,
        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24,
        0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f, 0x3f, 0x3c, 0x38, 0x34,
        0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x3f,
        0x3f, 0x3c, 0x38, 0x34, 0x30, 0x2c, 0x28, 0x24, 0x24, 0x28, 0x2c, 0x30,
        0x34, 0x38, 0x3c, 0x3f,

        10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
        10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
        10,   10,   10,   10,   10,   10,   10,   10,
};

[[gnu::section(".time_critical.text")]]
void drawNextChunk() {
    int s = chunk * 80;
    for (int d = 0; d < 80; d++) {
        auto v = pxLevels[mono[d + s]];
        next.ch[2]->set2(d, v);
        next.ch[1]->set2(d, v);
        next.ch[0]->set2(d, v);
    }

    // int c = (totalFrames + line) & 0x1ff;
    // auto r = pxLevels[((c >> 0) & 7) << 3];
    // auto g = pxLevels[((c >> 1) & 7) << 3];
    // auto b = pxLevels[((c >> 2) & 7) << 3];
    // for (int i = 0; i < 80; i++) {
    //     next.ch[2]->set2(i, r);
    //     next.ch[1]->set2(i, g);
    //     next.ch[0]->set2(i, b);
    // }
}

[[gnu::section(".time_critical.text")]]
void addVideoPreamble() {
    // 5.2.1.1 Preamble
    // We need to output 8 preamble symbols
    // CH2   CH1   CH0
    // 0 0   0 1   0 0 (no sync bits at this time)
    auto c0 = pxControl[0];
    auto c1 = pxControl[1];
    for (int i = 150; i < 158; i++) {
        hblankCH2.set(i, c0);
        hblankCH1.set(i, c1);
        hblankCH0.set(i, c0);
    }
    // 5.2.2.1 Video Guard Band
    // We need to output 2 guard symbols
    for (int i = 158; i < 160; i++) {
        hblankCH2.set(i, pxVideoGuard2);
        hblankCH1.set(i, pxVideoGuard1);
        hblankCH0.set(i, pxVideoGuard0);
    }
}

[[gnu::section(".time_critical.text")]]
void removeVideoPreamble() {
    auto c0 = pxControl[0];
    for (int i = 150; i < 160; i++) {
        hblankCH2.set(i, c0);
        hblankCH1.set(i, c0);
        hblankCH0.set(i, c0);
    }
}

[[gnu::section(".time_critical.text")]]
void update() {
    if (line < 480 && chunk < 4) {
        if (!(line & 1)) { drawNextChunk(); }
    } else if (chunk == 4) {
        if (line == 524 || line < 479) {
            // The chunk following this one (i.e. beginning of next line) will
            // be video addVideoPreamble();
        } else {
            // Chunk following this one (beginning of next line) doesn't have
            // video removeVideoPreamble();
        }
    }
}

[[gnu::section(".time_critical.text")]]
void advance() {
    // Bump all the relevant counters
    ++totalChunks;
    ++chunk;
    if (chunk == 5) {
        chunk = 0;
        ++totalLines;
        ++line;
        if (line >= 525) {
            line = 0;
            ++totalFrames;
        }
    }

    if (line < 480) {
        if (chunk < 4) {
            next = lists[chunk];
        } else {
            next = {&hblankCH0, &hblankCH1, &hblankCH2};
        }
    } else if (line < 490) {
        if (chunk < 4) {
            next = {&empty, &empty, &empty};
        } else {
            next = {&empty, &empty, &hblankCH0};
        }
    } else if (line < 492) {
        if (chunk < 4) {
            next = {&empty, &empty, &vsync};
        } else {
            next = {&empty, &empty, &hvsync};
        }
    } else {
        if (chunk < 4) {
            next = {&empty, &empty, &empty};
        } else {
            next = {&empty, &empty, &hblankCH0};
        }
    }
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    stdio_init_all();

    // core 1 will handle DMA IRQ; core 0 handles all others
    irq_set_enabled(DMA_IRQ_0, false);

    sleep_ms(1000);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    initClock();

    initGPIO();
    initBuffers();

    initPIO();
    initDMA();

    multicore_launch_core1_with_stack(main1, core1Stack, sizeof(core1Stack));

    double t0 = double(get_absolute_time());

    float fps = 59.94;
    uint32_t foo = 63;
    while (true) {
        for (int i = 0; i < 1000000; i++) {
            float frames = totalFrames;
            float sec = frames / fps;
            foo *= 97;
            mono[32 + (i & 0xff)] =
                    ((foo >> (int(sec) & 31)) & 1) ? 0x00 : 0x3f;
            tight_loop_contents();
        }

        gpio_put(PICO_DEFAULT_LED_PIN, (totalLines >> 12) & 1);
    }

    __builtin_unreachable();
}
