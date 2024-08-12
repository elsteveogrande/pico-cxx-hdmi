#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/dma.h>
#include <hardware/i2c.h>
#include <hardware/pio.h>

#include "boards/pico.h"
#include "font5x7.h"
#include "pico/time.h"

constexpr uint8_t oledAddress = 0x78 >> 1;
extern char textBuffer[8 * 20];

// https://github.com/Harbys/pico-ssd1306/blob/master/ssd1306.h
enum OLEDCommand : uint8_t {
    CONTRAST = 0x81,
    DISPLAYALL_ON_RESUME = 0xA4,
    DISPLAYALL_ON = 0xA5,
    INVERTED_OFF = 0xA6,
    INVERTED_ON = 0xA7,
    DISPLAY_OFF = 0xAE,
    DISPLAY_ON = 0xAF,
    DISPLAYOFFSET = 0xD3,
    COMPINS = 0xDA,
    VCOMDETECT = 0xDB,
    DISPLAYCLOCKDIV = 0xD5,
    PRECHARGE = 0xD9,
    MULTIPLEX = 0xA8,
    LOWCOLUMN = 0x00,
    HIGHCOLUMN = 0x10,
    STARTLINE = 0x40,
    MEMORYMODE = 0x20,
    MEMORYMODE_HORZONTAL = 0x00,
    MEMORYMODE_VERTICAL = 0x01,
    MEMORYMODE_PAGE = 0x10,
    COLUMNADDR = 0x21,
    PAGEADDR = 0x22,
    COM_REMAP_OFF = 0xC0,
    COM_REMAP_ON = 0xC8,
    CLUMN_REMAP_OFF = 0xA0,
    CLUMN_REMAP_ON = 0xA1,
    CHARGEPUMP = 0x8D,
    EXTERNALVCC = 0x1,
    SWITCHCAPVCC = 0x2,
};

inline int oledCommandByte(uint8_t x) {
    uint8_t bytes[2] = {0x00, x};
    return i2c_write_blocking(i2c1, oledAddress, bytes, sizeof(bytes), false);
}

inline void oledCommand(OLEDCommand cmd) {
    oledCommandByte(cmd);
}

inline void oledCommand(OLEDCommand cmd, uint8_t param) {
    oledCommandByte(cmd);
    oledCommandByte(param);
}

inline void oledCommand(OLEDCommand cmd, uint8_t p1, uint8_t p2) {
    oledCommandByte(cmd);
    oledCommandByte(p1);
    oledCommandByte(p2);
}

inline void oledRedraw() {
    oledCommand(PAGEADDR, 0, 7);
    oledCommand(COLUMNADDR, 4, 123);
    oledCommand(STARTLINE);
    uint8_t buffer[121];
    buffer[0] = 0x40;
    for (int row = 0; row < 7; ++row) {
        int i = 1;
        for (int col = 0; col < 20; ++col) {
            int j = col + (row * 20);
            char c = textBuffer[j];
            if (c < 0x20 || c > 0x6f) { c = 0x20; }
            c -= 0x20;
            for (int line = 0; line < 6; line++) {
                buffer[i++] = font[line + (6 * c)];
            }
        }
        i2c_write_blocking(i2c1, oledAddress, buffer, sizeof(buffer), false);
    }
}

inline void print(char const* str) {
    // memcpy(textBuffer, textBuffer + 20, 140);
    strncpy(textBuffer, str, 20);
    oledRedraw();
}

inline void print(uint64_t x) {
    char buf[21] = "                    ";
    for (int p = 19; p >= 0; --p, x /= 10) {
        buf[p] = (x || p == 19)
            ? char(x % 10) + '0'
            : ' ';
    }
    print(buf);
    oledRedraw();
}

inline void initDisplay() {
    i2c_init(i2c1, 1000000);
    gpio_set_function(14, gpio_function::GPIO_FUNC_I2C);
    gpio_set_function(15, gpio_function::GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    int ackCount = 16;
    int attempts = 0;
    while (ackCount) {
        sleep_ms(50);
        uint8_t bytes[1] = {0x00};
        if (i2c_write_blocking(i2c1, oledAddress, bytes, sizeof(bytes), false) == 1) {
            --ackCount;
        }
        ++attempts;
        gpio_put(PICO_DEFAULT_LED_PIN, 1 & attempts);
    }

    oledCommand(DISPLAY_OFF);
    oledCommand(DISPLAYCLOCKDIV, 128);
    oledCommand(CONTRAST, 0xc0);
    oledCommand(INVERTED_ON);
    oledCommand(MULTIPLEX, 0x3f);
    oledCommand(DISPLAYOFFSET, 0x00);
    oledCommand(PRECHARGE, 0x22);
    oledCommand(COMPINS, 0x12);
    oledCommand(VCOMDETECT, 0x40);
    oledCommand(CHARGEPUMP, 0x14);
    oledCommand(MEMORYMODE, 0 /*horizontal*/);
    oledCommand(DISPLAYALL_ON_RESUME);
    oledCommand(DISPLAY_ON);

    memset(textBuffer, 0x20, sizeof(textBuffer));
    oledRedraw();
}
