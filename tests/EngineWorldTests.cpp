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
        QVERIFY(chunk.blockAt(-1, 0, 0) == cub3::engine::BlockType::Air);
        QVERIFY(chunk.blockAt(0, cub3::engine::kChunkHeight, 0) == cub3::engine::BlockType::Air);
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
};
QTEST_GUILESS_MAIN(EngineWorldTests)
#include "EngineWorldTests.moc"
