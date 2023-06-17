#pragma once

#include <Project.Library/Application.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <string_view>
#include <vector>
#include <memory>
#include <array>
#include <optional>

//Fwog Stuff

#include <Fwog/BasicTypes.h>
#include <Fwog/Buffer.h>
#include <Fwog/Pipeline.h>
#include <Fwog/Rendering.h>
#include <Fwog/Shader.h>
#include <Fwog/Texture.h>


namespace Primitives
{
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    using indexType =  uint32_t;

    // Took it from fwog's examples 02_deferred.cpp
    static constexpr std::array<Vertex, 24> cubeVertices{
        // front (+z)
        Vertex{{-0.5, -0.5, 0.5}, {0, 0, 1}, {0, 0}},
        {{0.5, -0.5, 0.5}, {0, 0, 1}, {1, 0}},
        {{0.5, 0.5, 0.5}, {0, 0, 1}, {1, 1}},
        {{-0.5, 0.5, 0.5}, {0, 0, 1}, {0, 1}},

        // back (-z)
        {{-0.5, 0.5, -0.5}, {0, 0, -1}, {1, 1}},
        {{0.5, 0.5, -0.5}, {0, 0, -1}, {0, 1}},
        {{0.5, -0.5, -0.5}, {0, 0, -1}, {0, 0}},
        {{-0.5, -0.5, -0.5}, {0, 0, -1}, {1, 0}},

        // left (-x)
        {{-0.5, -0.5, -0.5}, {-1, 0, 0}, {0, 0}},
        {{-0.5, -0.5, 0.5}, {-1, 0, 0}, {1, 0}},
        {{-0.5, 0.5, 0.5}, {-1, 0, 0}, {1, 1}},
        {{-0.5, 0.5, -0.5}, {-1, 0, 0}, {0, 1}},

        // right (+x)
        {{0.5, 0.5, -0.5}, {1, 0, 0}, {1, 1}},
        {{0.5, 0.5, 0.5}, {1, 0, 0}, {0, 1}},
        {{0.5, -0.5, 0.5}, {1, 0, 0}, {0, 0}},
        {{0.5, -0.5, -0.5}, {1, 0, 0}, {1, 0}},

        // top (+y)
        {{-0.5, 0.5, 0.5}, {0, 1, 0}, {0, 0}},
        {{0.5, 0.5, 0.5}, {0, 1, 0}, {1, 0}},
        {{0.5, 0.5, -0.5}, {0, 1, 0}, {1, 1}},
        {{-0.5, 0.5, -0.5}, {0, 1, 0}, {0, 1}},

        // bottom (-y)
        {{-0.5, -0.5, -0.5}, {0, -1, 0}, {0, 0}},
        {{0.5, -0.5, -0.5}, {0, -1, 0}, {1, 0}},
        {{0.5, -0.5, 0.5}, {0, -1, 0}, {1, 1}},
        {{-0.5, -0.5, 0.5}, {0, -1, 0}, {0, 1}},
    };

    static constexpr std::array<indexType, 36> cubeIndices{
        0,  1,  2,  2,  3,  0,

        4,  5,  6,  6,  7,  4,

        8,  9,  10, 10, 11, 8,

        12, 13, 14, 14, 15, 12,

        16, 17, 18, 18, 19, 16,

        20, 21, 22, 22, 23, 20,
    };
}

struct DrawObject
{
    //T1 and T2 can be different container types. std::array or std::vector.
    //Didn't want this to be a constructor because the actual DrawObject struct does not need to be templated.
    template <typename T1, typename T2>
    static DrawObject Init(T1 const& vertexList, T2 const& indexList, size_t indexCount);

    std::optional<Fwog::Buffer> vertexBuffer;
    std::optional<Fwog::Buffer> indexBuffer;

    uint32_t indexCount;

    struct ObjectUniform
    {
        glm::mat4 modelTransform = glm::mat4(1.0f);
    };

    std::optional<Fwog::TypedBuffer<ObjectUniform>> modelUniformBuffer;
};

struct Camera
{
    //To Do: Rename this from 'global' to camera stuff. Its just called that because the Fwog Examples did.
    struct GlobalUniforms {
        glm::mat4 viewProj;
        glm::vec3 eyePos;
    };

    GlobalUniforms globalStruct;
    std::optional<Fwog::TypedBuffer<GlobalUniforms>> globalUniformsBuffer;
};

class ProjectApplication final : public Application
{
protected:
    void AfterCreatedUiContext() override;
    void BeforeDestroyUiContext() override;
    bool Load() override;
    void RenderScene() override;
    void RenderUI() override;
    void Update() override;

private:
    //uint32_t shaderProgram;
    //bool MakeShader(std::string_view vertexShaderFilePath, std::string_view fragmentShaderFilePath);

    Fwog::GraphicsPipeline MakePipeline(std::string_view vertexShaderPath, std::string_view fragmentShaderPath);

    //Using stbi_load
    Fwog::Texture MakeTexture(std::string_view texturePath, int32_t expectedChannels = 4);

    Camera MakeCamera();

    std::optional<Fwog::GraphicsPipeline> pipelineTextured;
    std::optional<Fwog::Texture> cubeTexture;
    std::optional<Camera> camera;

    static constexpr size_t numCubes = 2;
    DrawObject exampleCubes[numCubes];
};