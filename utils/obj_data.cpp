#include "obj_data.hpp"

#include "third-party/tiny_obj_loader.h"
#include <stdexcept>
#include <iostream>
#include <format>

namespace KRV {

OBJData::OBJData() {}

OBJData::OBJData(std::string const &filename) {
    Init(filename);
}

void OBJData::Init(std::string const &filename) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;

    if (!reader.ParseFromFile(filename, reader_config)) {
        throw std::runtime_error("OBJData: Cannot parse OBJ from file");
    }

    if (!reader.Warning().empty()) {
        std::cout << "\033[33m" << "TinyObjReader: " << reader.Warning() << "\033[39m" << std::endl;
    }

    auto& shapes = reader.GetShapes();

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            if (fv != 3ULL) {
                throw std::runtime_error("OBJData: face does not contain 3 vertices");
            }

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to index
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                vertexIndices.push_back(static_cast<uint32_t>(idx.vertex_index));
                normalIndices.push_back(static_cast<uint32_t>(idx.normal_index));
                texCoordIndices.push_back(static_cast<uint32_t>(idx.texcoord_index));
            }

            numOfTriangles++;
            index_offset += fv;
        }
    }

    auto& attrib = reader.GetAttrib();
    vertices = attrib.vertices;
    normals = attrib.normals;
    texCoords = attrib.texcoords;


    // TODO: Fix this
    constexpr size_t VULKAN_UPDATE_MAX_SIZE = 65536ULL;
    if (vertices.size()*sizeof(float) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Vertex Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    } else if (vertexIndices.size()*sizeof(uint32_t) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Vertex Index Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    } else if (normals.size()*sizeof(float) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Normal Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    } else if (normalIndices.size()*sizeof(uint32_t) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Normal Index Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    } else if (texCoords.size()*sizeof(float) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Texture Coordinate Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    } else if (texCoordIndices.size()*sizeof(uint32_t) > VULKAN_UPDATE_MAX_SIZE) {
        throw std::runtime_error(std::format("Texture Coordinate Index Buffer Size must be less or equal than {} bytes", VULKAN_UPDATE_MAX_SIZE));
    }
}

void OBJData::Destroy() {
    numOfTriangles = 0U;

    vertexIndices.clear();
    vertexIndices.shrink_to_fit();
    normalIndices.clear();
    normalIndices.shrink_to_fit();
    texCoordIndices.clear();
    texCoordIndices.shrink_to_fit();

    vertices.clear();
    vertices.shrink_to_fit();
    normals.clear();
    normals.shrink_to_fit();
    texCoords.clear();
    texCoords.shrink_to_fit();
}

uint32_t OBJData::GetNumOfTriangles() const {
    return numOfTriangles;
}

std::vector<uint32_t> const &OBJData::GetVertexIndices() const {
    return vertexIndices;
}

std::vector<uint32_t> const &OBJData::GetNormalIndices() const {
    return normalIndices;
}

std::vector<uint32_t> const &OBJData::GetTexCoordIndices() const {
    return texCoordIndices;
}


std::vector<float> const &OBJData::GetVertices() const {
    return vertices;
}

std::vector<float> const &OBJData::GetNormals() const {
    return normals;
}

std::vector<float> const &OBJData::GetTexCoords() const {
    return texCoords;
}


}
