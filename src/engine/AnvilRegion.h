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

struct Chunk final {
    int localX{0};
    int localZ{0};
    std::int32_t timestamp{0};
    // Compression id is preserved as stored (the high bit is the external-file flag).
    std::uint8_t compression{0};
    bool external{false};
    // Payload bytes excluding the four-byte length and compression marker.
    // For external chunks, the payload is stored in the corresponding .mcc file.
    std::vector<std::uint8_t> payload;
};

class RegionError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Parse and validate an in-memory .mca region container. This reads its header
/// and chunk records but deliberately does not decompress payloads or parse NBT.
class Region final {
public:
    [[nodiscard]] static Region parse(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] const std::optional<Chunk>& chunkAt(int localX, int localZ) const;

private:
    std::array<std::optional<Chunk>, kRegionSide * kRegionSide> m_chunks{};
};

} // namespace cub3::engine::anvil
