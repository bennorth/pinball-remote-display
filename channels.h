#ifndef CHANNELS_H_
#define CHANNELS_H_

#include <cstdint>

// Rows occur once every 256us, but not at one pixel per 2us.  There
// is a burst of 128 cycles of a 1MHz dot-clock, then a 128us
// solid-low period of the dot-clock.

struct channels {
    // 'DBLANK' in schematics.  Mostly high; goes low for around 40us
    // every row, with the pulse starting 16us before the end of a
    // row, and (therefore) extending 24us after the end of each row.
    //
    static constexpr uint8_t hsync = 1;

    // 'ROW_DATA' in schematics.  Mostly low; goes high for 256us,
    // with the rising edge coincident with the end of the last row of
    // a frame, and the falling edge coincident with the end of the
    // first row of the next frame.
    //
    static constexpr uint8_t vsync = 2;

    // 'DOT_CLOCK' in schematics.  Bursts of 128 cycles of 1MHz clock
    // as in summary at top.  Rising edge indicates when to read pixel
    // data.
    //
    static constexpr uint8_t clk = 4;

    // 'SERIAL_DATA' in schematics.  To be sampled at rising edge of
    // 'clk'.  Seems not fully stable between those sampling points
    // but sampling it at first sample that clock is high appears
    // reliable in practice.
    //
    static constexpr uint8_t data = 8;
};

#endif  // CHANNELS_H_
