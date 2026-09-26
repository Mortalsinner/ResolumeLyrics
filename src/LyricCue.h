#pragma once

#include <FFGLSDK.h>
#include <string>
#include <vector>
#include <windows.h>
#include <gdiplus.h>

class FFGLGradients : public CFFGLPlugin
{
public:
    FFGLGradients();

    FFResult InitGL(const FFGLViewportStruct* vp) override;
    FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
    FFResult DeInitGL() override;

    FFResult SetFloatParameter(unsigned int index, float value) override;
    float GetFloatParameter(unsigned int index) override;

    FFResult SetTextParameter(unsigned int index, const char* value) override;
    char* GetTextParameter(unsigned int index) override;

private:
    void RebuildLines();
    void RenderTextToTexture();
    void Advance(int delta);
    std::wstring Utf8ToWide(const std::string& text);
    std::wstring CurrentLineWide() const;

    ffglex::FFGLShader shader;
    ffglex::FFGLScreenQuad quad;

    GLuint texture = 0;
    GLint textureLocation = -1;

    unsigned int viewportWidth = 1920;
    unsigned int viewportHeight = 1080;

    std::string lyricsText =
        "I see the lights\n"
        "They're shining bright\n"
        "We're running through the night";

    std::vector<std::string> lines;
    int currentLine = 0;

    float fontSize = 64.0f;
    float x = 0.5f;
    float y = 0.5f;
    float opacity = 1.0f;
    float align = 0.5f; // 0 left, .5 center, 1 right
    bool dirty = true;

    ULONG_PTR gdiplusToken = 0;
};
