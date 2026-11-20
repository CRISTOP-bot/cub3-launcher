// Bounded clean-room reader for the Java Edition Anvil region container.
#include "AnvilRegion.h"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <string>
#include <utility>
#include <zlib.h>

namespace cub3::engine::anvil {
namespace {

std::uint32_t u32be(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U)
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U)
        | static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::size_t locationIndex(int x, int z) {
    if (x < 0 || x >= kRegionSide || z < 0 || z >= kRegionSide)
        throw RegionError("chunk coordinates are outside the 32x32 region");
    return static_cast<std::size_t>(x + z * kRegionSide);
}

} // namespace

Region Region::parse(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < kHeaderBytes)
        throw RegionError("region is shorter than its 8 KiB header");
    if (bytes.size() % kSectorBytes != 0)
        throw RegionError("region size is not a whole number of 4 KiB sectors");

    const std::size_t sectorCount = bytes.size() / kSectorBytes;
    std::vector<bool> occupied(sectorCount, false);
    occupied[0] = true;
    occupied[1] = true;
    Region region;

    for (std::size_t i = 0; i < region.m_chunks.size(); ++i) {
        const std::size_t headerOffset = i * 4;
        const std::uint32_t location = u32be(bytes, headerOffset);
        const std::size_t sectorOffset = static_cast<std::size_t>(location >> 8U);
        const std::size_t sectors = static_cast<std::size_t>(location & 0xffU);
        if (sectorOffset == 0 && sectors == 0)
            continue;
        if (sectorOffset < 2 || sectors == 0)
            throw RegionError("invalid chunk location entry");
        if (sectorOffset >= sectorCount || sectors > sectorCount - sectorOffset)
            throw RegionError("chunk location points beyond the region file");
        for (std::size_t sector = sectorOffset; sector < sectorOffset + sectors; ++sector) {
            if (occupied[sector])
                throw RegionError("chunk location entries overlap");
            occupied[sector] = true;
        }

        const std::size_t recordOffset = sectorOffset * kSectorBytes;
        const std::uint32_t recordLength = u32be(bytes, recordOffset);
        const std::size_t allocatedBytes = sectors * kSectorBytes;
        if (recordLength < 1 || recordLength > allocatedBytes - 4)
            throw RegionError("chunk record length is outside its allocated sectors");
        if (recordLength > bytes.size() - recordOffset - 4)
            throw RegionError("truncated chunk record");

        const std::uint8_t marker = bytes[recordOffset + 4];
        const bool external = (marker & 0x80U) != 0;
        Chunk chunk;
        chunk.localX = static_cast<int>(i % kRegionSide);
        chunk.localZ = static_cast<int>(i / kRegionSide);
        chunk.timestamp = std::bit_cast<std::int32_t>(u32be(bytes, kSectorBytes + headerOffset));
        chunk.compression = static_cast<std::uint8_t>(marker & 0x7fU);
        chunk.external = external;
        if (external) {
            if (recordLength != 1)
                throw RegionError("external chunk record must not contain an inline payload");
        } else {
            const std::size_t payloadLength = static_cast<std::size_t>(recordLength - 1);
            const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(recordOffset + 5);
            chunk.payload.assign(begin, begin + static_cast<std::ptrdiff_t>(payloadLength));
        }
        region.m_chunks[i] = std::move(chunk);
    }
    return region;
}

const std::optional<Chunk>& Region::chunkAt(int localX, int localZ) const {
    return m_chunks[locationIndex(localX, localZ)];
}

std::vector<std::uint8_t> decompressPayload(const Chunk& chunk, std::size_t maxOutputBytes) {
    if (chunk.external)
        throw RegionError("external chunk payload is not available in this reader");
    if (maxOutputBytes == 0)
        throw RegionError("decompressed output limit must be greater than zero");
    if (chunk.compression == 3) {
        if (chunk.payload.size() > maxOutputBytes)
            throw RegionError("uncompressed chunk exceeds configured output limit");
        return chunk.payload;
    }
    if (chunk.compression != 1 && chunk.compression != 2)
        throw RegionError("unsupported Anvil compression type (supported: gzip, zlib, none)");
    if (chunk.payload.empty())
        throw RegionError("compressed chunk payload is empty");

    z_stream stream{};
    const int windowBits = chunk.compression == 1 ? MAX_WBITS + 16 : MAX_WBITS;
    if (inflateInit2(&stream, windowBits) != Z_OK)
        throw RegionError("could not initialize zlib decompressor");

    struct InflateGuard final {
        z_stream* stream;
        ~InflateGuard() { inflateEnd(stream); }
    } guard{&stream};

    std::vector<std::uint8_t> output;
    constexpr std::size_t kOutputBlockBytes = 16U * 1024U;
    std::array<std::uint8_t, kOutputBlockBytes> block{};
    std::size_t inputOffset = 0;
    int status = Z_OK;
    while (status != Z_STREAM_END) {
        if (stream.avail_in == 0 && inputOffset < chunk.payload.size()) {
            const std::size_t count = std::min<std::size_t>(
                chunk.payload.size() - inputOffset, std::numeric_limits<uInt>::max());
            stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(
                chunk.payload.data() + inputOffset));
            stream.avail_in = static_cast<uInt>(count);
            inputOffset += count;
        }
        stream.next_out = block.data();
        stream.avail_out = static_cast<uInt>(block.size());
        status = inflate(&stream, Z_NO_FLUSH);
        const std::size_t produced = block.size() - stream.avail_out;
        if (produced > maxOutputBytes - output.size())
            throw RegionError("decompressed chunk exceeds configured output limit");
        output.insert(output.end(), block.begin(), block.begin() + static_cast<std::ptrdiff_t>(produced));
        if (status == Z_STREAM_END)
            break;
        if (status != Z_OK)
            throw RegionError("compressed chunk is corrupt or truncated");
        if (produced == 0 && stream.avail_in == 0 && inputOffset == chunk.payload.size())
            throw RegionError("compressed chunk ended before the stream terminator");
    }
    if (stream.avail_in != 0 || inputOffset != chunk.payload.size())
        throw RegionError("trailing bytes after compressed chunk stream");
    return output;
}

} // namespace cub3::engine::anvil
