#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "stb_image.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Shader.hpp"
#include "Light.hpp"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Texture
{
    GLuint id;
    std::string type;
    std::string path;
};

static std::vector<Texture> g_loadedTextures;

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    GLuint vao, vbo, ebo;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures) :
        vertices(vertices), indices(indices), textures(textures)
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, uv)));

        glBindVertexArray(0);
    }

    void draw(const Shader& shader)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            if (textures[i].type == "texture_diffuse")
                shader.SetInt(shader.GetUniformLocation(("material.diffuse" + std::to_string(diffuseNr++)).c_str()), i);
            if (textures[i].type == "texture_specular")
                shader.SetInt(shader.GetUniformLocation(("material.specular" + std::to_string(specularNr++)).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
};

struct Model
{
    std::string path;
    std::vector<Mesh> meshes;

    Model(const std::string& path) : path(path)
    {
        isValid = false;
        std::string directory = path.substr(0, path.find_last_of('/'));
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "failed to load model at \"" << path << "\": " << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene, directory);
        isValid = true;
    }

    void draw(const Shader& shader)
    {
        for (size_t i = 0; i < meshes.size(); i++)
            meshes[i].draw(shader);
    }

    explicit operator bool() const
    {
        return isValid;
    }

private:
    bool isValid;

    void processNode(const aiNode* node, const aiScene* scene, const std::string& directory)
    {
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory));
            std::cout << "loaded mesh \"" << mesh->mName.C_Str() << "\"\n";
        }

        for (size_t i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene, directory);
    }

    Mesh processMesh(const aiMesh* mesh, const aiScene* scene, const std::string& directory) const
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // vertices
        for (size_t i = 0; i < mesh->mNumVertices; i++)
        {
            glm::vec2 uv = mesh->mTextureCoords[0]
                ? glm::vec2(uv.x = mesh->mTextureCoords[0][i].x, uv.y = mesh->mTextureCoords[0][i].y)
                : glm::vec2();
            vertices.push_back(Vertex(
                glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
                glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
                glm::vec2(uv)
            ));
        }

        // indices
        for (size_t i = 0; i < mesh->mNumFaces; i++)
            for (size_t j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);

        // textures
        if (mesh->mMaterialIndex >= 0)
        {
            auto* material = scene->mMaterials[mesh->mMaterialIndex];
            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", directory);
            textures = { diffuseMaps };
            auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", directory);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return { vertices, indices, textures };
    }

    std::vector<Texture> loadMaterialTextures(const aiMaterial* material, aiTextureType type, const std::string& name, const std::string& directory) const
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
        {
            aiString textureLocalPath;
            material->GetTexture(type, i, &textureLocalPath);
            textures.push_back(processTexture(textureLocalPath, name, directory));
        }
        return textures;
    }

    Texture processTexture(const aiString& textureLocalPath, const std::string& name, const std::string& directory) const
    {
        std::string texturePath = directory + "/" + textureLocalPath.C_Str();

        // check if already loaded
        for (size_t i = 0; i < g_loadedTextures.size(); i++)
            if (g_loadedTextures[i].path == texturePath)
                return g_loadedTextures[i];
        // create new
        Texture texture
        {
            .id = loadTextureFromFile(texturePath.c_str()),
            .type = name,
            .path = texturePath
        };
        g_loadedTextures.push_back(texture);
        std::cout << "loaded texture \"" << texturePath << "\"\n";
        return texture;
    }

    static GLuint loadTextureFromFile(const char* path)
    {
        int width, height, nrChannels;
        stbi_uc* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (!data)
        {
            std::cerr << "failed to load image \"" << path << "\"" << std::endl;
            return -1;
        }

        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else
        {
            std::cerr << "image \"" << path << "\" has unsupported number of channels: " << nrChannels << std::endl;
            return -1;
        }

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return textureId;
    }
};