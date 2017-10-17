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

#include <vector>
#include <cstdint>
#include <stdexcept>

#include <stdio.h>

static const int frame_width = 128;
static const int frame_height = 32;
static const int frame_n_pixels = frame_width * frame_height;
static const int output_pad = 2;
static const int output_width = frame_width + 2 * output_pad;
static const int output_height = frame_height + 2 * output_pad;
static const int dot_size = 6;

static const int dot_pattern[] = {
      0,  64, 128, 128,  64,   0,
     64, 192, 256, 256, 192,  64,
    128, 256, 256, 256, 256, 128,
    128, 256, 256, 256, 256, 128,
     64, 192, 256, 256, 192,  64,
      0,  64, 128, 128,  64,   0 };

int main(int c, char** v) {
    std::vector<uint8_t> gray_buffer(frame_n_pixels);
    std::vector<uint8_t> rgb_buffer(dot_size * output_width * dot_size * output_height * 3);
    const size_t rgb_stride = dot_size * dot_size * 3 * output_width;

    static const int r_dot = atoi(v[1]);
    static const int g_dot = atoi(v[2]);
    static const int b_dot = atoi(v[3]);

    while (true) {
        size_t n_read = fread(gray_buffer.data(), 1, frame_n_pixels, stdin);
        if (n_read != frame_n_pixels)
            throw std::runtime_error("short read");

        for (size_t src_row = 0; src_row != frame_height; ++src_row) {
            for (size_t src_col = 0; src_col != frame_width; ++src_col) {
                size_t src_idx = src_row * frame_width + src_col;
                uint8_t r_full = gray_buffer[src_idx] * r_dot / 256;
                uint8_t g_full = gray_buffer[src_idx] * g_dot / 256;
                uint8_t b_full = gray_buffer[src_idx] * b_dot / 256;

                const int* p_pattern = &dot_pattern[0];
                for (size_t dr = 0; dr != dot_size; dr++) {
                    for (size_t dc = 0; dc != dot_size; dc++) {
                        size_t dst_idx = ((output_pad + src_row) * rgb_stride
                                          + dr * dot_size * output_width * 3
                                          + (output_pad + src_col) * dot_size * 3
                                          + dc * 3);
                        rgb_buffer[dst_idx] = r_full * (*p_pattern) / 256;
                        rgb_buffer[dst_idx+1] = g_full * (*p_pattern) / 256;
                        rgb_buffer[dst_idx+2] = b_full * (*p_pattern++) / 256;
                    }
                }
            }
        }

        fwrite(static_cast<void*>(rgb_buffer.data()), 1, rgb_buffer.size(), stdout);
    }
}
