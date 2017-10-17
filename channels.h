/*
  Copyright 2017 Ben North

  This file is part of "Pinball Remote Display".

  "Pinball Remote Display" is free software: you can redistribute it and/or
  modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  "Pinball Remote Display" is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  "Pinball Remote Display".  If not, see <http://www.gnu.org/licenses/>.
*/

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

    static bool test_hsync(uint8_t x) { return !!(x & hsync); }
    static bool test_vsync(uint8_t x) { return !!(x & vsync); }
    static bool test_clk(uint8_t x) { return !!(x & clk); }
    static bool test_data(uint8_t x) { return !!(x & data); }
};

#endif  // CHANNELS_H_
