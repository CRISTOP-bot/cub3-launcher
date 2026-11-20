// CUB3 native-engine reader for Minecraft Java Anvil region containers.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace cub3::engine::anvil {

inline constexpr std::size_t kSectorBytes = 4096;
inline constexpr std::size_t kHeaderBytes = 2 * kSectorBytes;
inline constexpr int kRegionSide = 32;
inline constexpr std::size_t kDefaultMaxDecompressedBytes = 64U * 1024U * 1024U;

struct Chunk final {
    int localX{0};
    int localZ{0};
    std::int32_t timestamp{0};
    // Compression id is preserved as stored (the high bit is the external-file flag).
    std::uint8_t compression{0};
    bool external{false};
    // Compressed payload bytes excluding the four-byte length and marker.
    // For external chunks, bytes are stored in the corresponding .mcc file.
    std::vector<std::uint8_t> payload;
};

class RegionError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Parse and validate an in-memory .mca region container. This reads its header
/// and chunk records but does not parse NBT or load a world.
class Region final {
public:
    [[nodiscard]] static Region parse(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] const std::optional<Chunk>& chunkAt(int localX, int localZ) const;

private:
    std::array<std::optional<Chunk>, kRegionSide * kRegionSide> m_chunks{};
};

/// Decode chunk data using region compression types 1 (gzip), 2 (zlib), or
/// 3 (uncompressed). LZ4 (4), external-file reads, and NBT parsing are not
/// implemented. Throws RegionError on unsupported, corrupt, or oversized data.
[[nodiscard]] std::vector<std::uint8_t> decompressPayload(
    const Chunk& chunk, std::size_t maxOutputBytes = kDefaultMaxDecompressedBytes);

} // namespace cub3::engine::anvil
