#pragma once
#include <cstdint>

struct GLFWwindow;

class Application
{
public:
    void Run();

protected:
    void Close();
    bool IsKeyPressed(int32_t key);
    virtual void AfterCreatedUiContext();
    virtual void BeforeDestroyUiContext();
    virtual bool Initialize();
    virtual bool Load();
    virtual void Unload();
    virtual void RenderScene();
    virtual void RenderUI(double dt);
    virtual void Update(double dt);

    static constexpr unsigned int windowWidth = 1920;
    static constexpr unsigned int windowHeight = 1080;

private:
    GLFWwindow* _windowHandle = nullptr;
    void Render(double dt);
};