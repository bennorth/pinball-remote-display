#include <stdio.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "channels.h"

struct Decoder
{
    static constexpr size_t pixels_per_row = 128;
    static constexpr size_t rows_per_frame = 32;
    static constexpr size_t pixels_per_frame = rows_per_frame * pixels_per_row;
    static constexpr size_t samples_per_microsecond = 4;
    static constexpr size_t microseconds_per_row = 256;
    static constexpr size_t microseconds_per_frame
        = rows_per_frame * microseconds_per_row;
    static constexpr size_t nominal_samples_per_frame
        = microseconds_per_frame * samples_per_microsecond;

    static constexpr size_t microseconds_phase_zero_from_vsync_rising_edge = 64;

    // Space for two and a bit frames.
    static constexpr size_t buffer_size
        = 2 * nominal_samples_per_frame + nominal_samples_per_frame / 4;

    Decoder() : sample_buffer(buffer_size)
        {}

    void read_into_buffer(size_t n_bytes);
    size_t samples_from_microseconds(size_t us) const;
    std::vector<size_t> detect_rising_edges(size_t data_length) const;
    std::vector<size_t> detect_falling_edges(size_t data_length) const;

    ////////////////////////////////////////////////////////////////////////////////////////

    // TODO: This could be made more self-contained.  Currently each user of the
    // buffer has to remember how many samples they've read into the buffer.  It
    // would be better if there was a more-intelligent 'buffer' object which
    // remembered how many samples it contained.
    //
    std::vector<uint8_t> sample_buffer;

    // How many logic analyzer samples we think currently make up one display
    // frame.  Because the pinball machine clock and the logic analyzer clock
    // are not locked, this can wander with respect to the nominal value.
    //
    size_t estimated_frame_period = nominal_samples_per_frame;

    // Define the start of a frame as being half-way through the 128us clock-low
    // stretch preceding the first row of that frame.  This happens 64us after
    // the rising edge of the ROW_DATA / vsync pulse.  We always attempt to read
    // samples up to that point.  The 'estimated frame phase' is how many
    // samples past the nominal frame start we think we most recently read.
    //
    ssize_t estimated_frame_phase = 0;

    // Whether we think we're successfully locked onto a cycle of reading a
    // frame of samples and decoding it.
    //
    bool locked = false;
};


void Decoder::read_into_buffer(size_t n_bytes)
{
    size_t n_read = fread(sample_buffer.data(), 1, n_bytes, stdin);
    if (n_read != n_bytes)
    {
        std::ostringstream oss;
        oss << "short read; requested " << n_bytes
            << " but got " << n_read;
        throw std::runtime_error(oss.str());
    }
}

std::vector<size_t> Decoder::detect_rising_edges(size_t data_length) const
{
    std::vector<size_t> rising_edge_idxs;

    for (size_t i = 1; i != data_length; ++i) {
        bool this_1 = channels::test_vsync(sample_buffer[i]);
        bool prev_0 = ( ! channels::test_vsync(sample_buffer[i - 1]));
        if (prev_0 && this_1)
            rising_edge_idxs.push_back(i);
    }

    return rising_edge_idxs;
}

std::vector<size_t> Decoder::detect_falling_edges(size_t data_length) const
{
    std::vector<size_t> falling_edge_idxs;

    for (size_t i = 1; i != data_length; ++i) {
        bool this_0 = ( ! channels::test_vsync(sample_buffer[i]));
        bool prev_1 = channels::test_vsync(sample_buffer[i - 1]);
        if (this_0 && prev_1)
            falling_edge_idxs.push_back(i);
    }

    return falling_edge_idxs;
}

size_t Decoder::samples_from_microseconds(size_t us) const
{
    return us * estimated_frame_period / microseconds_per_frame;
}
