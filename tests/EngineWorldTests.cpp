// CUB3 - Native voxel-world core tests
#include "engine/World.h"
#include <QtTest>

class EngineWorldTests final : public QObject {
    Q_OBJECT
private slots:
    void generatedTerrainIsDeterministicForSeedAndCoordinates() {
        const cub3::engine::Chunk first(0xC0B3, -2, 7);
        const cub3::engine::Chunk second(0xC0B3, -2, 7);
        for (int z = 0; z < cub3::engine::kChunkDepth; ++z) {
            for (int x = 0; x < cub3::engine::kChunkWidth; ++x) {
                QCOMPARE(first.surfaceHeight(x, z), second.surfaceHeight(x, z));
                QVERIFY(first.surfaceHeight(x, z) >= 40);
                QVERIFY(first.surfaceHeight(x, z) < 57);
                QVERIFY(first.blockAt(x, first.surfaceHeight(x, z), z) == cub3::engine::BlockType::Grass);
                QVERIFY(first.blockAt(x, 0, z) == cub3::engine::BlockType::Stone);
            }
        }
    }
    void blocksCanBeReadAndChangedWithinBounds() {
        cub3::engine::Chunk chunk(9, 0, 0);
        QVERIFY(chunk.setBlock(3, 100, 4, cub3::engine::BlockType::Stone));
        QVERIFY(chunk.blockAt(3, 100, 4) == cub3::engine::BlockType::Stone);
        QVERIFY(!chunk.setBlock(-1, 0, 0, cub3::engine::BlockType::Dirt));
        QCOMPARE(chunk.blockAt(-1, 0, 0), cub3::engine::BlockType::Air);
        QCOMPARE(chunk.blockAt(0, cub3::engine::kChunkHeight, 0), cub3::engine::BlockType::Air);
        QCOMPARE(chunk.surfaceHeight(-1, 0), -1);
    }
    void negativeWorldCoordinatesUseFloorChunkDivision() {
        using cub3::engine::splitBlockCoordinates;
        const auto leftEdge = splitBlockCoordinates(-1, -16);
        QCOMPARE(leftEdge.chunkX, -1); QCOMPARE(leftEdge.localX, 15);
        QCOMPARE(leftEdge.chunkZ, -1); QCOMPARE(leftEdge.localZ, 0);
        const auto boundary = splitBlockCoordinates(-16, -17);
        QCOMPARE(boundary.chunkX, -1); QCOMPARE(boundary.localX, 0);
        QCOMPARE(boundary.chunkZ, -2); QCOMPARE(boundary.localZ, 15);
    }
    void surfaceHeightTracksEditsAndEmptyColumns() {
        cub3::engine::Chunk chunk(9, 0, 0);
        const int originalSurface = chunk.surfaceHeight(3, 4);
        QVERIFY(chunk.setBlock(3, originalSurface, 4, cub3::engine::BlockType::Air));
        QCOMPARE(chunk.surfaceHeight(3, 4), originalSurface - 1);
        QVERIFY(chunk.setBlock(3, originalSurface - 1, 4, cub3::engine::BlockType::Air));
        QVERIFY(chunk.setBlock(3, originalSurface - 2, 4, cub3::engine::BlockType::Air));
        QVERIFY(chunk.setBlock(3, originalSurface - 3, 4, cub3::engine::BlockType::Air));
        QVERIFY(chunk.setBlock(3, originalSurface - 4, 4, cub3::engine::BlockType::Air));
        QCOMPARE(chunk.surfaceHeight(3, 4), originalSurface - 5);
        for (int y = originalSurface - 5; y >= 0; --y)
            QVERIFY(chunk.setBlock(3, y, 4, cub3::engine::BlockType::Air));
        QCOMPARE(chunk.surfaceHeight(3, 4), -1);
        QVERIFY(chunk.setBlock(3, 20, 4, cub3::engine::BlockType::Dirt));
        QCOMPARE(chunk.surfaceHeight(3, 4), 20);
        QVERIFY(chunk.setBlock(3, 10, 4, cub3::engine::BlockType::Stone));
        QCOMPARE(chunk.surfaceHeight(3, 4), 20);
    }
    void worldLoadsAndEditsChunksAtGlobalCoordinates() {
        using namespace cub3::engine;
        World world(0xC0B3);
        QCOMPARE(world.loadedChunkCount(), std::size_t{0});
        QCOMPARE(world.blockAt(0, -1, 0), BlockType::Air);
        QCOMPARE(world.loadedChunkCount(), std::size_t{0});

        (void)world.blockAt(-1, 50, 0);
        const Chunk* leftChunk = world.findChunk(-1, 0);
        QVERIFY(leftChunk != nullptr);
        QCOMPARE(world.blockAt(-1, leftChunk->surfaceHeight(15, 0), 0), BlockType::Grass);
        QCOMPARE(world.loadedChunkCount(), std::size_t{1});

        QVERIFY(world.setBlock(16, 100, -1, BlockType::Stone));
        QCOMPARE(world.blockAt(16, 100, -1), BlockType::Stone);
        QVERIFY(world.findChunk(1, -1) != nullptr);
        QCOMPARE(world.loadedChunkCount(), std::size_t{2});
        QCOMPARE(world.blockAt(15, 100, -1), BlockType::Air);
        QCOMPARE(world.loadedChunkCount(), std::size_t{3});

        QVERIFY(world.unloadChunk(1, -1));
        QVERIFY(world.findChunk(1, -1) == nullptr);
        QVERIFY(!world.unloadChunk(1, -1));
        QCOMPARE(world.blockAt(16, 100, -1), BlockType::Air);
        QCOMPARE(world.loadedChunkCount(), std::size_t{3});
    }
};
QTEST_GUILESS_MAIN(EngineWorldTests)
#include "EngineWorldTests.moc"
