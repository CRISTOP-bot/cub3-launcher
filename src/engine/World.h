// CUB3 - Independent native voxel-engine foundations
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cub3::engine {

inline constexpr int kChunkWidth = 16;
inline constexpr int kChunkDepth = 16;
inline constexpr int kChunkHeight = 128;

enum class BlockType : std::uint8_t { Air = 0, Grass = 1, Dirt = 2, Stone = 3 };

/// Deterministic, dependency-free voxel chunk. Independent prototype;
/// it does not claim Minecraft parity or read Minecraft resources.
class Chunk final {
public:
    explicit Chunk(std::uint64_t seed = 0, int chunkX = 0, int chunkZ = 0);
    [[nodiscard]] BlockType blockAt(int localX, int y, int localZ) const;
    [[nodiscard]] bool setBlock(int localX, int y, int localZ, BlockType block);
    [[nodiscard]] int surfaceHeight(int localX, int localZ) const;
private:
    [[nodiscard]] static constexpr bool contains(int x, int y, int z) {
        return x >= 0 && x < kChunkWidth && y >= 0 && y < kChunkHeight
            && z >= 0 && z < kChunkDepth;
    }
    [[nodiscard]] static std::uint64_t coordinateHash(std::uint64_t seed,
                                                       std::int64_t x,
                                                       std::int64_t z);
    [[nodiscard]] static constexpr std::size_t index(int x, int y, int z) {
        return (static_cast<std::size_t>(y) * kChunkDepth
                + static_cast<std::size_t>(z)) * kChunkWidth
            + static_cast<std::size_t>(x);
    }
    std::array<BlockType, kChunkWidth * kChunkDepth * kChunkHeight> m_blocks{};
    std::array<std::uint8_t, kChunkWidth * kChunkDepth> m_surface{};
};

struct BlockCoordinates final { int chunkX; int chunkZ; int localX; int localZ; };
[[nodiscard]] BlockCoordinates splitBlockCoordinates(int x, int z);

} // namespace cub3::engine
