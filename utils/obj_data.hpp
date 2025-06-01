#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace KRV {

// Only triangulated meshes are supported.
// Multiple shapes are supported.
class OBJData {
public:
    OBJData();
    explicit OBJData(std::string const &filename);

    void Init(std::string const &filename);
    void Destroy();

    uint32_t GetNumOfTriangles() const;

    std::vector<uint32_t> const &GetVertexIndices() const;
    std::vector<uint32_t> const &GetNormalIndices() const;
    std::vector<uint32_t> const &GetTexCoordIndices() const;

    std::vector<float> const &GetVertices() const;
    std::vector<float> const &GetNormals() const;
    std::vector<float> const &GetTexCoords() const;

private:
    uint32_t numOfTriangles = 0U;

    std::vector<uint32_t> vertexIndices = {};
    std::vector<uint32_t> normalIndices = {};
    std::vector<uint32_t> texCoordIndices = {};

    std::vector<float> vertices = {};
    std::vector<float> normals = {};
    std::vector<float> texCoords = {};
};

}
