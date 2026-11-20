// CUB3 native-engine NBT parser tests; generated fixtures contain no game assets.
#include "engine/Nbt.h"

#include <QtTest>
#include <cstdint>

namespace {
using Bytes = std::vector<std::uint8_t>;
using cub3::engine::nbt::Type;

void u8(Bytes& out, std::uint8_t value) { out.push_back(value); }
void u16(Bytes& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8U));
    out.push_back(static_cast<std::uint8_t>(value));
}
void u32(Bytes& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 24U));
    out.push_back(static_cast<std::uint8_t>(value >> 16U));
    out.push_back(static_cast<std::uint8_t>(value >> 8U));
    out.push_back(static_cast<std::uint8_t>(value));
}
void str(Bytes& out, const std::string& value) {
    u16(out, static_cast<std::uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}
void named(Bytes& out, Type type, const std::string& name) {
    u8(out, static_cast<std::uint8_t>(type));
    str(out, name);
}
}

class NativeNbtTests final : public QObject {
    Q_OBJECT
private slots:
    void readsRootIntBigEndian() {
        Bytes bytes;
        named(bytes, Type::Int, "answer");
        u32(bytes, 0x01020304U);
        const auto doc = cub3::engine::nbt::parse(bytes);
        QCOMPARE(QString::fromStdString(doc.name), QStringLiteral("answer"));
        QCOMPARE(doc.root.type, Type::Int);
        QCOMPARE(std::get<std::int32_t>(doc.root.value), std::int32_t{0x01020304});
    }

    void readsCompoundListAndString() {
        Bytes bytes;
        named(bytes, Type::Compound, "root");
        named(bytes, Type::String, "greeting");
        str(bytes, "hello");
        named(bytes, Type::List, "numbers");
        u8(bytes, static_cast<std::uint8_t>(Type::Int));
        u32(bytes, 2);
        u32(bytes, 7);
        u32(bytes, 42);
        u8(bytes, static_cast<std::uint8_t>(Type::End));
        const auto doc = cub3::engine::nbt::parse(bytes);
        const auto& root = std::get<cub3::engine::nbt::Compound>(doc.root.value);
        QCOMPARE(QString::fromStdString(std::get<std::string>(root.at("greeting").value)), QStringLiteral("hello"));
        const auto& numbers = std::get<cub3::engine::nbt::List>(root.at("numbers").value);
        QCOMPARE(numbers.size(), std::size_t{2});
        QCOMPARE(std::get<std::int32_t>(numbers[0].value), std::int32_t{7});
        QCOMPARE(std::get<std::int32_t>(numbers[1].value), std::int32_t{42});
    }

    void rejectsMalformedAndOversizedInput() {
        const auto rejects = [](const Bytes& bytes, cub3::engine::nbt::Limits limits = {}) {
            try { (void)cub3::engine::nbt::parse(bytes, limits); }
            catch (const cub3::engine::nbt::ParseError&) { return true; }
            return false;
        };
        QVERIFY(rejects(Bytes{0}));
        QVERIFY(rejects(Bytes{99, 0, 0}));
        QVERIFY(rejects(Bytes{3, 0, 0, 0}));
        QVERIFY(rejects(Bytes{7, 0, 0, 0xff, 0xff, 0xff, 0xff}));
        Bytes valid;
        named(valid, Type::Int, "x");
        u32(valid, 1);
        cub3::engine::nbt::Limits limits;
        limits.maxInputBytes = 2;
        QVERIFY(rejects(valid, limits));
    }

    void rejectsEndListAndTrailingBytes() {
        Bytes list;
        named(list, Type::List, "list");
        u8(list, static_cast<std::uint8_t>(Type::End));
        u32(list, 1);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::nbt::parse(list), cub3::engine::nbt::ParseError);
        Bytes trailing;
        named(trailing, Type::Int, "x");
        u32(trailing, 3);
        trailing.push_back(0);
        QVERIFY_EXCEPTION_THROWN(cub3::engine::nbt::parse(trailing), cub3::engine::nbt::ParseError);
    }
};

QTEST_GUILESS_MAIN(NativeNbtTests)
#include "NbtTests.moc"
