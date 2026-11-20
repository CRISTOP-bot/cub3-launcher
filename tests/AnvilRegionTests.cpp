// Synthetic fixtures for the clean-room Anvil container reader; no game assets.
#include "engine/AnvilRegion.h"

#include <QtTest>
#include <cstdint>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
using cub3::engine::anvil::kSectorBytes;

void put32(Bytes& out, std::size_t offset, std::uint32_t value) {
    out[offset] = static_cast<std::uint8_t>(value >> 24U);
    out[offset + 1] = static_cast<std::uint8_t>(value >> 16U);
    out[offset + 2] = static_cast<std::uint8_t>(value >> 8U);
    out[offset + 3] = static_cast<std::uint8_t>(value);
}

Bytes emptyRegion(std::size_t sectors = 3) { return Bytes(sectors * kSectorBytes, 0); }

Bytes oneChunk(std::uint8_t marker = 2) {
    Bytes bytes = emptyRegion();
    put32(bytes, 0, (2U << 8U) | 1U); // local (0,0), sector 2, one sector
    put32(bytes, kSectorBytes, 1700000000U);
    put32(bytes, 2 * kSectorBytes, 4U); // marker plus three payload bytes
    bytes[2 * kSectorBytes + 4] = marker;
    bytes[2 * kSectorBytes + 5] = 0xaa;
    bytes[2 * kSectorBytes + 6] = 0xbb;
    bytes[2 * kSectorBytes + 7] = 0xcc;
    return bytes;
}
}

class AnvilRegionTests final : public QObject {
    Q_OBJECT
private slots:
    void readsLocationTimestampAndPayload() {
        const auto region = cub3::engine::anvil::Region::parse(oneChunk());
        QVERIFY(region.chunkAt(0, 0).has_value());
        const auto& chunk = *region.chunkAt(0, 0);
        QCOMPARE(chunk.localX, 0);
        QCOMPARE(chunk.localZ, 0);
        QCOMPARE(chunk.timestamp, std::int32_t{1700000000});
        QCOMPARE(chunk.compression, std::uint8_t{2});
        QVERIFY(!chunk.external);
        QCOMPARE(chunk.payload, (Bytes{0xaa, 0xbb, 0xcc}));
        QVERIFY(!region.chunkAt(31, 31).has_value());
    }

    void supportsExternalChunkMarker() {
        auto bytes = oneChunk(0x82);
        put32(bytes, 2 * kSectorBytes, 1U);
        const auto region = cub3::engine::anvil::Region::parse(bytes);
        const auto& chunk = *region.chunkAt(0, 0);
        QVERIFY(chunk.external);
        QCOMPARE(chunk.compression, std::uint8_t{2});
        QVERIFY(chunk.payload.empty());
    }

    void rejectsShortUnalignedAndOutOfBoundsRegions() {
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(Bytes(100)), cub3::engine::anvil::RegionError);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(Bytes(9000)), cub3::engine::anvil::RegionError);
        auto bytes = emptyRegion();
        put32(bytes, 0, (3U << 8U) | 1U);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(bytes), cub3::engine::anvil::RegionError);
    }

    void rejectsOverlappingAndMalformedRecords() {
        auto overlap = emptyRegion(4);
        put32(overlap, 0, (2U << 8U) | 2U);
        put32(overlap, 4, (3U << 8U) | 1U);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(overlap), cub3::engine::anvil::RegionError);

        auto badLength = oneChunk();
        put32(badLength, 2 * kSectorBytes, 0U);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(badLength), cub3::engine::anvil::RegionError);

        auto badExternal = oneChunk(0x82);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::anvil::Region::parse(badExternal), cub3::engine::anvil::RegionError);
    }

    void rejectsInvalidChunkCoordinates() {
        const auto region = cub3::engine::anvil::Region::parse(emptyRegion());
        QVERIFY_EXCEPTION_THROWN(region.chunkAt(-1, 0), cub3::engine::anvil::RegionError);
        QVERIFY_EXCEPTION_THROWN(region.chunkAt(32, 0), cub3::engine::anvil::RegionError);
    }
};

QTEST_GUILESS_MAIN(AnvilRegionTests)
#include "AnvilRegionTests.moc"
