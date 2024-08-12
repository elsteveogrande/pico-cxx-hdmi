#pragma once

#include <cstdint>

struct Pixel10 {
    uint16_t b: 10;
};
static_assert(sizeof(Pixel10) == 2);

struct Pixel20 {
    uint32_t b: 20;
};
static_assert(sizeof(Pixel20) == 4);

constexpr Pixel10 pxClock {0b0000011111};

// 5.2.2.1 Video Guard Band
constexpr Pixel10 pxVideoGuard2 {0b1011001100};
constexpr Pixel10 pxVideoGuard1 {0b0100110011};
constexpr Pixel10 pxVideoGuard0 {0b1011001100};

extern Pixel20 pxLevels[64];

extern Pixel10 pxControl[4];

extern Pixel10 pxTERC4[16];

struct Buffer {
    Pixel20 pxs[80];

    void set(int index, Pixel10 p10) {
        if (index & 1) {
            pxs[index >> 1].b |= (uint32_t(p10.b)) << 10;
        } else {
            pxs[index >> 1].b = uint32_t(p10.b);
        }
    }

    void set(int index, Pixel20 p20) {
        pxs[index >> 1] = p20;
    }

    void set2(int index, Pixel20 p20) {
        pxs[index] = p20;
    }
};

struct Buffers {
    Buffer* ch[3];
};
