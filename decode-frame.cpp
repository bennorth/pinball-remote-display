#include <stdio.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <limits>
#include <algorithm>

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
    { cold_start(); }

    void read_into_buffer(size_t n_bytes);
    void cold_start();
    size_t samples_from_microseconds(size_t us) const;
    std::vector<size_t> detect_rising_edges(size_t data_length) const;
    std::vector<size_t> detect_falling_edges(size_t data_length) const;

    void read_frame_and_adjust_estimates();
    std::vector<uint8_t> frame_from_samples() const;

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

void Decoder::cold_start()
{
    read_into_buffer(buffer_size);
    auto rising_edge_idxs = detect_rising_edges(buffer_size);

    auto n_edges = rising_edge_idxs.size();
    if (n_edges < 2)
    {
        locked = false;
        return;
    }

    estimated_frame_period = (rising_edge_idxs[n_edges - 1]
                              - rising_edge_idxs[n_edges - 2]);

    estimated_frame_phase
        = (buffer_size - (rising_edge_idxs[n_edges - 1]
                          + samples_from_microseconds(
                              microseconds_phase_zero_from_vsync_rising_edge)));

    // Read and throw away the remainder of this frame, bringing us to the
    // phase-zero point.
    read_into_buffer(estimated_frame_period - estimated_frame_phase);
    estimated_frame_phase = 0;

    locked = true;
}

void Decoder::read_frame_and_adjust_estimates()
{
    auto n_samples_to_next_zero_phase = estimated_frame_period - estimated_frame_phase;
    read_into_buffer(n_samples_to_next_zero_phase);
    auto rising_edge_idxs = detect_rising_edges(n_samples_to_next_zero_phase);
    auto falling_edge_idxs = detect_falling_edges(n_samples_to_next_zero_phase);

    if ((not locked)
        || (rising_edge_idxs.size() != 1)
        || (falling_edge_idxs.size() != 1))
    {
        locked = false;
        // Leave estimated_frame_period alone.
        estimated_frame_phase = 0;
    }
    else
    {
        auto rising_edge_idx = rising_edge_idxs[0];
        auto falling_edge_idx = falling_edge_idxs[0];
        auto n_samples_low = rising_edge_idx - falling_edge_idx;
        estimated_frame_period = n_samples_low * 32 / 31;

        estimated_frame_phase
            = (n_samples_to_next_zero_phase
               - (rising_edge_idx
                  + samples_from_microseconds(
                      microseconds_phase_zero_from_vsync_rising_edge)));
    }
}

std::vector<uint8_t> Decoder::frame_from_samples() const
{
    std::vector<uint8_t> pixels;
    pixels.reserve(pixels_per_row * rows_per_frame);

    if (not locked)
    {
        pixels.insert(pixels.end(), pixels_per_frame, 0);
        return pixels;
    }

    bool prev_clk = channels::test_clk(sample_buffer[0]);
    bool prev_hsync = channels::test_hsync(sample_buffer[0]);
    size_t n_pixels_this_row = 0;
    size_t n_rows_this_frame = 0;
    for (size_t i = 1; i != estimated_frame_period; ++i) {
        uint8_t sample = sample_buffer[i];
        bool this_clk = channels::test_clk(sample);
        bool this_hsync = channels::test_hsync(sample);

        if (this_clk && (not prev_clk))
        {
            if (n_pixels_this_row < pixels_per_row
                && n_rows_this_frame < rows_per_frame)
            {
                // 42 is best approximation to 255 / 6.
                pixels.push_back(channels::test_data(sample) ? 42 : 0);
                ++n_pixels_this_row;
            }
        }

        if (this_hsync && (not prev_hsync))
        {
            size_t n_missing_pixels = pixels_per_row - n_pixels_this_row;
            if (n_missing_pixels > 0)
                pixels.insert(pixels.end(), n_missing_pixels, 0);

            n_pixels_this_row = 0;
            ++n_rows_this_frame;
        }

        prev_clk = this_clk;
        prev_hsync = this_hsync;
    }

    size_t n_missing_pixels = pixels_per_frame - pixels.size();
    pixels.insert(pixels.end(), n_missing_pixels, 0);

    return pixels;
}


////////////////////////////////////////////////////////////////////////////////////////

struct NoSignalSource {
    NoSignalSource(const char* fname);

    void next_frame_into(std::vector<uint8_t>& frame_out);
    void reset();

    std::vector<uint8_t> frames;
    size_t n_frames;
    size_t next_frame_idx;
};

NoSignalSource::NoSignalSource(const char* fname)
{
    std::ifstream in_stream {fname, (std::ios::in | std::ios::binary)};
    if (not in_stream)
        throw std::runtime_error("failed to open source file");

    in_stream.ignore(std::numeric_limits<std::streamsize>::max());
    auto n_bytes = in_stream.gcount();
    in_stream.clear();
    in_stream.seekg(0, std::ios_base::beg);

    if (n_bytes % Decoder::pixels_per_frame != 0)
        throw std::runtime_error("non-integral number of frames");

    n_frames = n_bytes / Decoder::pixels_per_frame;
    frames.resize(n_bytes);
    in_stream.read(reinterpret_cast<char*>(frames.data()), n_bytes);

    if (in_stream.gcount() != n_bytes)
        throw std::runtime_error("second read didn't read expected n.bytes");

    reset();
}

void NoSignalSource::next_frame_into(std::vector<uint8_t>& frame_out)
{
    size_t pixel_idx_0 = next_frame_idx * Decoder::pixels_per_frame;

    frame_out.clear();
    frame_out.reserve(Decoder::pixels_per_frame);
    std::copy_n(frames.begin() + pixel_idx_0, Decoder::pixels_per_frame,
                std::back_inserter(frame_out));

    ++next_frame_idx;
    if (next_frame_idx == n_frames)
        next_frame_idx = 0;
}

void NoSignalSource::reset()
{ next_frame_idx = 0; }
