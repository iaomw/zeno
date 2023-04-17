#include "zeno/unreal/ZenoUnrealTypes.h"
#include <rapidjson/stringbuffer.h>
#include <variant>

zeno::SimpleCharBuffer::SimpleCharBuffer(const char *InChar, size_t Size) {
    length = Size + 1;
    data = new char[length];
    memcpy(data, InChar, Size * sizeof(char));
    data[Size] = '\0';
}

zeno::SimpleCharBuffer::SimpleCharBuffer(zeno::SimpleCharBuffer &&InBuffer) noexcept {
    length = InBuffer.length;
    data = InBuffer.data;
    InBuffer.data = nullptr;
    InBuffer.length = 0;
}

zeno::SimpleCharBuffer &zeno::SimpleCharBuffer::operator=(zeno::SimpleCharBuffer &&InBuffer) noexcept {
    length = InBuffer.length;
    data = InBuffer.data;
    InBuffer.data = nullptr;
    InBuffer.length = 0;
    return *this;
}

zeno::SimpleCharBuffer::~SimpleCharBuffer() {
    delete []data;
}

zeno::SimpleCharBuffer::SimpleCharBuffer(const char *InChar)
{
    if (nullptr == InChar) {
        length = 0;
        data = nullptr;
        return;
    }
    length = std::strlen(InChar);
    data = new char[length+1];
    data[length++] = '\0';
    memcpy(data, InChar, length * sizeof(char));
}

bool IsNearlyZero(float value, float tolerance = 0.0001) {
    return std::abs(value) < tolerance;
}

zeno::unreal::Mesh::Mesh(const std::vector<zeno::vec3f> &verts, const std::vector<zeno::vec3i> &trigs) {
    vertices.reserve(verts.size());
    for (const auto &item : verts) {
        auto [x, y, z] = item;
        vertices.push_back( { x, y, z } );
    }
    triangles.reserve(trigs.size());
    for (const auto &item : trigs) {
        triangles.push_back(item);
    }
}
