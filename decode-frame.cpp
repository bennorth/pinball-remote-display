#include <stdio.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <vector>
#include <stdexcept>

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

    ////////////////////////////////////////////////////////////////////////////////////////

    // TODO: This could be made more self-contained.  Currently each user of the
    // buffer has to remember how many samples they've read into the buffer.  It
    // would be better if there was a more-intelligent 'buffer' object which
    // remembered how many samples it contained.
    //
    std::vector<uint8_t> sample_buffer;
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
