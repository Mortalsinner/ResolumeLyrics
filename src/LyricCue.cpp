#include "FFGLGradients.h"

#include <algorithm>
#include <sstream>
#include <vector>
#include <cstring>
#pragma comment(lib, "gdiplus.lib")

using namespace ffglex;
using namespace Gdiplus;

enum ParamType : FFUInt32
{
    PT_LYRICS = 0,
    PT_NEXT,
    PT_PREVIOUS,
    PT_RESET,
    PT_FONT_SIZE,
    PT_X,
    PT_Y,
    PT_OPACITY,
    PT_ALIGNMENT,
};

static CFFGLPluginInfo PluginInfo(
    PluginFactory<FFGLGradients>,
    "LYC1",
    "Lyric Cue",
    2, 1,
    1, 0,
    FF_SOURCE,
    "Live lyric cue source for Resolume",
    "Lyric Cue FFGL"
);

static const char vertexShaderCode[] = R"(#version 410 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

out vec2 uv;

void main()
{
    gl_Position = vPosition;
    uv = vUV;
}
)";

static const char fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D lyricTexture;

in vec2 uv;
out vec4 fragColor;

void main()
{
    fragColor = texture(lyricTexture, uv);
}
)";

FFGLGradients::FFGLGradients()
{
    SetMinInputs(0);
    SetMaxInputs(0);

    SetParamInfo(PT_LYRICS, "Lyrics", FF_TYPE_TEXT, 0.0f);
    SetParamInfo(PT_NEXT, "Next", FF_TYPE_EVENT, 0.0f);
    SetParamInfo(PT_PREVIOUS, "Previous", FF_TYPE_EVENT, 0.0f);
    SetParamInfo(PT_RESET, "Reset", FF_TYPE_EVENT, 0.0f);

    SetParamInfof(PT_FONT_SIZE, "Font Size", FF_TYPE_STANDARD);
    SetParamInfof(PT_X, "X Position", FF_TYPE_STANDARD);
    SetParamInfof(PT_Y, "Y Position", FF_TYPE_STANDARD);
    SetParamInfof(PT_OPACITY, "Opacity", FF_TYPE_ALPHA);
    SetParamInfof(PT_ALIGNMENT, "Alignment", FF_TYPE_STANDARD);

    SetParamRange(PT_FONT_SIZE, 12.0f, 240.0f);

    FFGLLog::LogToHost("Lyric Cue created");
    RebuildLines();
}

FFResult FFGLGradients::InitGL(const FFGLViewportStruct* vp)
{
    viewportWidth = vp->width;
    viewportHeight = vp->height;

    if (!shader.Compile(vertexShaderCode, fragmentShaderCode))
    {
        DeInitGL();
        return FF_FAIL;
    }

    if (!quad.Initialise())
    {
        DeInitGL();
        return FF_FAIL;
    }

    ScopedShaderBinding shaderBinding(shader.GetGLID());
    textureLocation = shader.FindUniform("lyricTexture");

    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok)
    {
        DeInitGL();
        return FF_FAIL;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    dirty = true;

    return CFFGLPlugin::InitGL(vp);
}

FFResult FFGLGradients::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{
    if (dirty)
        RenderTextToTexture();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ScopedShaderBinding shaderBinding(shader.GetGLID());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(textureLocation, 0);

    quad.Draw();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

    return FF_SUCCESS;
}

FFResult FFGLGradients::DeInitGL()
{
    if (texture != 0)
    {
        glDeleteTextures(1, &texture);
        texture = 0;
    }

    if (gdiplusToken != 0)
    {
        GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }

    shader.FreeGLResources();
    quad.Release();
    textureLocation = -1;

    return FF_SUCCESS;
}

void FFGLGradients::Advance(int delta)
{
    if (lines.empty())
    {
        currentLine = 0;
        dirty = true;
        return;
    }

    currentLine = std::clamp(currentLine + delta, 0, (int)lines.size() - 1);
    dirty = true;
}

FFResult FFGLGradients::SetFloatParameter(unsigned int index, float value)
{
    switch (index)
    {
    case PT_NEXT:
        if (value > 0.5f) Advance(1);
        return FF_SUCCESS;

    case PT_PREVIOUS:
        if (value > 0.5f) Advance(-1);
        return FF_SUCCESS;

    case PT_RESET:
        if (value > 0.5f)
        {
            currentLine = 0;
            dirty = true;
        }
        return FF_SUCCESS;

    case PT_FONT_SIZE:
        fontSize = std::clamp(value, 12.0f, 240.0f);
        dirty = true;
        return FF_SUCCESS;

    case PT_X:
        x = std::clamp(value, 0.0f, 1.0f);
        dirty = true;
        return FF_SUCCESS;

    case PT_Y:
        y = std::clamp(value, 0.0f, 1.0f);
        dirty = true;
        return FF_SUCCESS;

    case PT_OPACITY:
        opacity = std::clamp(value, 0.0f, 1.0f);
        dirty = true;
        return FF_SUCCESS;

    case PT_ALIGNMENT:
        align = std::clamp(value, 0.0f, 1.0f);
        dirty = true;
        return FF_SUCCESS;

    default:
        return FF_FAIL;
    }
}

float FFGLGradients::GetFloatParameter(unsigned int index)
{
    switch (index)
    {
    case PT_FONT_SIZE: return fontSize;
    case PT_X: return x;
    case PT_Y: return y;
    case PT_OPACITY: return opacity;
    case PT_ALIGNMENT: return align;
    default: return 0.0f;
    }
}

FFResult FFGLGradients::SetTextParameter(unsigned int index, const char* value)
{
    if (index != PT_LYRICS)
        return FF_FAIL;

    lyricsText = value ? value : "";
    RebuildLines();
    currentLine = 0;
    dirty = true;

    return FF_SUCCESS;
}

char* FFGLGradients::GetTextParameter(unsigned int index)
{
    if (index != PT_LYRICS)
        return (char*)FF_FAIL;

    return const_cast<char*>(lyricsText.c_str());
}

void FFGLGradients::RebuildLines()
{
    lines.clear();

    std::string normalized = lyricsText;
    normalized.erase(
        std::remove(normalized.begin(), normalized.end(), '\r'),
        normalized.end()
    );

    std::stringstream stream(normalized);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty())
            lines.push_back(line);
    }

    if (lines.empty())
        currentLine = 0;
    else
        currentLine = std::clamp(currentLine, 0, (int)lines.size() - 1);
}

std::wstring FFGLGradients::Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return L"";

    int count = MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(),
        (int)text.size(),
        nullptr, 0
    );

    if (count <= 0)
        return L"";

    std::wstring result(count, L'\0');

    MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(),
        (int)text.size(),
        result.data(),
        count
    );

    return result;
}

std::wstring FFGLGradients::CurrentLineWide() const
{
    if (lines.empty())
        return L"";

    int index = std::clamp(currentLine, 0, (int)lines.size() - 1);
    return Utf8ToWide(lines[index]);
}

void FFGLGradients::RenderTextToTexture()
{
    const UINT width = std::max(512u, viewportWidth);
    const UINT height = std::max(256u, viewportHeight);

    Bitmap bitmap(width, height, PixelFormat32bppPARGB);

    Graphics graphics(&bitmap);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.Clear(Color(0, 0, 0, 0));

    FontFamily family(L"Arial");
    Font font(&family, fontSize, FontStyleBold, UnitPixel);

    BYTE alpha = (BYTE)(std::clamp(opacity, 0.0f, 1.0f) * 255.0f);
    SolidBrush brush(Color(alpha, 255, 255, 255));

    StringFormat format;
    format.SetAlignment(
        align < 0.333f ? StringAlignmentNear :
        align > 0.666f ? StringAlignmentFar :
                         StringAlignmentCenter
    );
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoClip);

    RectF rect(
        0.0f,
        0.0f,
        (REAL)width,
        (REAL)height
    );

    std::wstring line = CurrentLineWide();

    graphics.DrawString(
        line.c_str(),
        -1,
        &font,
        rect,
        &format,
        &brush
    );

    BitmapData data;
    Rect lockRect(0, 0, width, height);

    if (bitmap.LockBits(
        &lockRect,
        ImageLockModeRead,
        PixelFormat32bppPARGB,
        &data
    ) == Ok)
    {
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            data.Scan0
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        bitmap.UnlockBits(&data);
    }

    dirty = false;
}
