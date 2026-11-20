// CUB3 - Independent native voxel-engine foundations
#include "World.h"

namespace cub3::engine {
namespace {
constexpr std::uint64_t mix(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}
constexpr int floorDiv(int value, int divisor) {
    const int quotient = value / divisor;
    const int remainder = value % divisor;
    return quotient - (remainder < 0 ? 1 : 0);
}
}

Chunk::Chunk(std::uint64_t seed, int chunkX, int chunkZ) {
    for (int z = 0; z < kChunkDepth; ++z) {
        for (int x = 0; x < kChunkWidth; ++x) {
            const std::int64_t worldX = static_cast<std::int64_t>(chunkX) * kChunkWidth + x;
            const std::int64_t worldZ = static_cast<std::int64_t>(chunkZ) * kChunkDepth + z;
            const int height = 40 + static_cast<int>(coordinateHash(seed, worldX, worldZ) % 17U);
            m_surface[static_cast<std::size_t>(z * kChunkWidth + x)] =
                static_cast<std::uint8_t>(height);
            for (int y = 0; y <= height; ++y) {
                const BlockType block = y == height ? BlockType::Grass
                    : y >= height - 3 ? BlockType::Dirt : BlockType::Stone;
                m_blocks[index(x, y, z)] = block;
            }
        }
    }
}

std::uint64_t Chunk::coordinateHash(std::uint64_t seed,
                                    std::int64_t x,
                                    std::int64_t z) {
    const auto ux = static_cast<std::uint64_t>(x);
    const auto uz = static_cast<std::uint64_t>(z);
    return mix(seed ^ mix(ux + 0x632be59bd9b4e019ULL)
                     ^ mix(uz + 0x8cb92baa3f3d8dd7ULL));
}

BlockType Chunk::blockAt(int localX, int y, int localZ) const {
    if (!contains(localX, y, localZ))
        return BlockType::Air;
    return m_blocks[index(localX, y, localZ)];
}

bool Chunk::setBlock(int localX, int y, int localZ, BlockType block) {
    if (!contains(localX, y, localZ))
        return false;

    m_blocks[index(localX, y, localZ)] = block;
    const std::size_t column = static_cast<std::size_t>(localZ * kChunkWidth + localX);
    const int oldSurface = m_surface[column] == 255 ? -1 : m_surface[column];
    if (block != BlockType::Air) {
        if (y > oldSurface)
            m_surface[column] = static_cast<std::uint8_t>(y);
    } else if (y == oldSurface) {
        int newSurface = y - 1;
        while (newSurface >= 0 && m_blocks[index(localX, newSurface, localZ)] == BlockType::Air)
            --newSurface;
        m_surface[column] = newSurface < 0 ? 255 : static_cast<std::uint8_t>(newSurface);
    }
    return true;
}

int Chunk::surfaceHeight(int localX, int localZ) const {
    if (localX < 0 || localX >= kChunkWidth
        || localZ < 0 || localZ >= kChunkDepth)
        return -1;
    const std::uint8_t height = m_surface[static_cast<std::size_t>(localZ * kChunkWidth + localX)];
    return height == 255 ? -1 : static_cast<int>(height);
}

BlockCoordinates splitBlockCoordinates(int x, int z) {
    const int chunkX = floorDiv(x, kChunkWidth);
    const int chunkZ = floorDiv(z, kChunkDepth);
    return {chunkX, chunkZ, x - chunkX * kChunkWidth, z - chunkZ * kChunkDepth};
}

std::size_t World::ChunkPositionHash::operator()(ChunkPosition position) const noexcept {
    const std::uint64_t packed = (static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(position.x)) << 32U)
        | static_cast<std::uint32_t>(position.z);
    return static_cast<std::size_t>(mix(packed));
}

Chunk& World::ensureChunk(int chunkX, int chunkZ) {
    const ChunkPosition position{chunkX, chunkZ};
    const auto [iterator, inserted] = m_chunks.try_emplace(position, m_seed, chunkX, chunkZ);
    (void)inserted;
    return iterator->second;
}

BlockType World::blockAt(int worldX, int y, int worldZ) {
    if (y < 0 || y >= kChunkHeight)
        return BlockType::Air;
    const BlockCoordinates coordinates = splitBlockCoordinates(worldX, worldZ);
    return ensureChunk(coordinates.chunkX, coordinates.chunkZ)
        .blockAt(coordinates.localX, y, coordinates.localZ);
}

bool World::setBlock(int worldX, int y, int worldZ, BlockType block) {
    if (y < 0 || y >= kChunkHeight)
        return false;
    const BlockCoordinates coordinates = splitBlockCoordinates(worldX, worldZ);
    return ensureChunk(coordinates.chunkX, coordinates.chunkZ)
        .setBlock(coordinates.localX, y, coordinates.localZ, block);
}

const Chunk* World::findChunk(int chunkX, int chunkZ) const noexcept {
    const auto iterator = m_chunks.find(ChunkPosition{chunkX, chunkZ});
    return iterator == m_chunks.end() ? nullptr : &iterator->second;
}

bool World::unloadChunk(int chunkX, int chunkZ) {
    return m_chunks.erase(ChunkPosition{chunkX, chunkZ}) != 0;
}

} // namespace cub3::engine
