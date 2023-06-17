#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Project/ProjectApplication.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <vector>
#include <queue>
#include <set>

static std::string Slurp(std::string_view path)
{
    std::ifstream file(path.data(), std::ios::ate);
    std::string result(file.tellg(), '\0');
    file.seekg(0);
    file.read((char*)result.data(), result.size());
    return result;
}

namespace fs = std::filesystem;

static std::string FindTexturePath(const fs::path& basePath, const cgltf_image* image)
{
    std::string texturePath;
    if (!image->uri)
    {
        auto newPath = basePath / image->name;
        if (!newPath.has_extension())
        {
            if (std::strcmp(image->mime_type, "image/png") == 0)
            {
                newPath.replace_extension("png");
            }
            else if (std::strcmp(image->mime_type, "image/jpg") == 0)
            {
                newPath.replace_extension("jpg");
            }
        }
        texturePath = newPath.generic_string();
    }
    else
    {
        texturePath = (basePath / image->uri).generic_string();
    }
    return texturePath;
}


template <typename T1, typename T2>
DrawObject DrawObject::Init(T1 const& vertexList, T2 const& indexList, size_t indexCount)
{
    DrawObject object;
    object.vertexBuffer.emplace(vertexList);
    object.indexBuffer.emplace(indexList);
    object.modelUniformBuffer =  Fwog::TypedBuffer<DrawObject::ObjectUniform>(Fwog::BufferStorageFlag::DYNAMIC_STORAGE);

    //Fwog takes in uint32_t for the indexCount but .size() on a container returns size_t. I'll just cast it here and hope its fine.
    object.indexCount = static_cast<uint32_t>(indexCount);
    
    return object;
}


Fwog::Texture ProjectApplication::MakeTexture(std::string_view texturePath, int32_t expectedChannels)
{
    int32_t textureWidth, textureHeight, textureChannels;
    unsigned char* textureData =
        stbi_load(texturePath.data(), &textureWidth, &textureHeight, &textureChannels, expectedChannels);
    assert(textureData);

    //How many times can this texture be divided evenly by half?
    uint32_t divideByHalfAmounts =  uint32_t(1 + floor(log2(glm::max(textureWidth, textureHeight))));

    Fwog::Texture createdTexture = Fwog::CreateTexture2DMip(
        {static_cast<uint32_t>(textureWidth),
        static_cast<uint32_t>(textureHeight)},
        Fwog::Format::R8G8B8A8_SRGB,
        divideByHalfAmounts);

    Fwog::TextureUpdateInfo updateInfo{
        .dimension = Fwog::UploadDimension::TWO,
        .level = 0,
        .offset = {},
        .size = {static_cast<uint32_t>(textureWidth),
        static_cast<uint32_t>(textureHeight), 1},
        .format = Fwog::UploadFormat::RGBA,
        .type = Fwog::UploadType::UBYTE,
        .pixels = textureData};

    createdTexture.SubImage(updateInfo);
    createdTexture.GenMipmaps();
    stbi_image_free(textureData);

    return createdTexture;
}


void ProjectApplication::AfterCreatedUiContext()
{
}

void ProjectApplication::BeforeDestroyUiContext()
{

}


bool ProjectApplication::Load()
{
    if (!Application::Load())
    {
        spdlog::error("App: Unable to load");
        return false;
    }

    pipelineTextured = MakePipeline("./data/shaders/main.vs.glsl", "./data/shaders/main.fs.glsl");
    for (size_t i = 0; i < numCubes; ++i)
    {
        //https://en.cppreference.com/w/cpp/language/class_template_argument_deduction 
        //because the containers which are the parameters are constexpr
        exampleCubes[i] = DrawObject::Init(Primitives::cubeVertices, Primitives::cubeIndices, Primitives::cubeIndices.size());
    }

    cubeTexture = MakeTexture("./data/textures/fwog_logo.png");

    return true;
}

void ProjectApplication::Update()
{
    if (IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        Close();
    }



}

void ProjectApplication::RenderScene()
{
    Fwog::BeginSwapchainRendering(Fwog::SwapchainRenderInfo{
        .viewport =
        Fwog::Viewport{.drawRect{.offset = {0, 0},
        .extent = {windowWidth, windowHeight}},
        .minDepth = 0.0f,
        .maxDepth = 1.0f},
        .colorLoadOp = Fwog::AttachmentLoadOp::CLEAR,
        .clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthLoadOp = Fwog::AttachmentLoadOp::CLEAR,
        .clearDepthValue = 1.0f});


    Fwog::SamplerState ss;
    ss.minFilter = Fwog::Filter::LINEAR;
    ss.magFilter = Fwog::Filter::LINEAR;
    ss.mipmapFilter = Fwog::Filter::LINEAR;
    ss.addressModeU = Fwog::AddressMode::REPEAT;
    ss.addressModeV = Fwog::AddressMode::REPEAT;
    ss.anisotropy = Fwog::SampleCount::SAMPLES_16;
    auto nearestSampler = Fwog::Sampler(ss);

    //Could refactor this to be a function of a class
    auto drawObject = [&](DrawObject const& object, Fwog::Texture const& textureAlbedo, Fwog::Sampler const& sampler)
    {
        Fwog::Cmd::BindGraphicsPipeline(pipelineTextured.value());
        Fwog::Cmd::BindUniformBuffer(0, globalUniformsBuffer.value());
        Fwog::Cmd::BindUniformBuffer(1, object.modelUniformBuffer.value());

        Fwog::Cmd::BindSampledImage(0, textureAlbedo, sampler);
        Fwog::Cmd::BindVertexBuffer(0, object.vertexBuffer.value(), 0, sizeof(Primitives::Vertex));
        Fwog::Cmd::BindIndexBuffer(object.indexBuffer.value(), Fwog::IndexType::UNSIGNED_INT);
        Fwog::Cmd::DrawIndexed(object.indexCount, 1, 0, 0, 0);
    };

    drawObject(exampleCubes[0], cubeTexture.value(), nearestSampler);
    Fwog::EndRendering();

}

void ProjectApplication::RenderUI()
{
    ImGui::Begin("Window");
    {
        ImGui::TextUnformatted("Hello Fwog!");
        ImGui::End();
    }

    ImGui::ShowDemoWindow();
}


Fwog::GraphicsPipeline ProjectApplication::MakePipeline(std::string_view vertexShaderPath, std::string_view fragmentShaderPath)
{
    auto LoadFile = [](std::string_view path)
    {
        std::ifstream file{ path.data() };
        std::string returnString { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        return returnString;
    };

    auto vertexShader = Fwog::Shader(Fwog::PipelineStage::VERTEX_SHADER, LoadFile(vertexShaderPath));
    auto fragmentShader = Fwog::Shader(Fwog::PipelineStage::FRAGMENT_SHADER, LoadFile(fragmentShaderPath));

    //Ensures this matches the shader and your vertex buffer data type

    static constexpr auto sceneInputBindingDescs = std::array{
        Fwog::VertexInputBindingDescription{
            // position
            .location = 0,
            .binding = 0,
            .format = Fwog::Format::R32G32B32_FLOAT,
            .offset = offsetof(Primitives::Vertex, position),
    },
    Fwog::VertexInputBindingDescription{
            // normal
            .location = 1,
            .binding = 0,
            .format = Fwog::Format::R32G32B32_FLOAT,
            .offset = offsetof(Primitives::Vertex, normal),
    },
    Fwog::VertexInputBindingDescription{
            // texcoord
            .location = 2,
            .binding = 0,
            .format = Fwog::Format::R32G32_FLOAT,
            .offset = offsetof(Primitives::Vertex, uv),
    },
    };

    auto inputDescs = sceneInputBindingDescs;
    auto primDescs =
        Fwog::InputAssemblyState{Fwog::PrimitiveTopology::TRIANGLE_LIST};

    return Fwog::GraphicsPipeline{{
            .vertexShader = &vertexShader,
            .fragmentShader = &fragmentShader,
            .inputAssemblyState = primDescs,
            .vertexInputState = {inputDescs},
            .depthState = {.depthTestEnable = true,
            .depthWriteEnable = true,
            .depthCompareOp = Fwog::CompareOp::LESS},
        }};
}

//bool ProjectApplication::MakeShader(std::string_view vertexShaderFilePath, std::string_view fragmentShaderFilePath)
//{
//    int success = false;
//    char log[1024] = {};
//    const auto vertexShaderSource = Slurp(vertexShaderFilePath);
//    const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
//    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
//    glCompileShader(vertexShader);
//    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//    if (!success)
//    {
//        glGetShaderInfoLog(vertexShader, 1024, nullptr, log);
//        spdlog::error(log);
//        return false;
//    }
//
//    const auto fragmentShaderSource = Slurp(fragmentShaderFilePath);
//    const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();
//    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
//    glCompileShader(fragmentShader);
//    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
//    if (!success)
//    {
//        glGetShaderInfoLog(fragmentShader, 1024, nullptr, log);
//        spdlog::error(log);
//        return false;
//    }
//
//    shaderProgram = glCreateProgram();
//    glAttachShader(shaderProgram, vertexShader);
//    glAttachShader(shaderProgram, fragmentShader);
//    glLinkProgram(shaderProgram);
//    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
//    if (!success)
//    {
//        glGetProgramInfoLog(shaderProgram, 1024, nullptr, log);
//        spdlog::error(log);
//
//        return false;
//    }
//
//    glDeleteShader(vertexShader);
//    glDeleteShader(fragmentShader);
//
//    return true;
//}
//
