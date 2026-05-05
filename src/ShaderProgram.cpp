#include "ShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

CShaderProgram::CShaderProgram()
    : m_nProgramId(0)
{
}

CShaderProgram::CShaderProgram(const std::string& szVertexShaderPath, const std::string& szFragmentShaderPath)
    : m_nProgramId(0)
{
    LoadFromFiles(szVertexShaderPath, szFragmentShaderPath);
}

CShaderProgram::~CShaderProgram()
{
    Destroy();
}

CShaderProgram::CShaderProgram(CShaderProgram&& other) noexcept
    : m_nProgramId(other.m_nProgramId)
{
    other.m_nProgramId = 0;
}

CShaderProgram& CShaderProgram::operator=(CShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        m_nProgramId = other.m_nProgramId;
        other.m_nProgramId = 0;
    }

    return *this;
}

void CShaderProgram::LoadFromFiles(const std::string& szVertexShaderPath, const std::string& szFragmentShaderPath)
{
    Destroy();

    m_nProgramId = CreateProgram(szVertexShaderPath, szFragmentShaderPath);
}

void CShaderProgram::Use() const
{
    glUseProgram(m_nProgramId);
}

GLuint CShaderProgram::GetProgramId() const
{
    return m_nProgramId;
}

void CShaderProgram::SetUniformMat4d(const char* pszName, const glm::dmat4& mValue) const
{
    glUniformMatrix4dv(
        GetUniformLocation(pszName),
        1,
        GL_FALSE,
        glm::value_ptr(mValue)
    );
}

void CShaderProgram::SetUniformVec2d(const char* pszName, const glm::dvec2& vValue) const
{
    glUniform2d(
        GetUniformLocation(pszName),
        vValue.x,
        vValue.y
    );
}

void CShaderProgram::SetUniformVec3d(const char* pszName, const glm::dvec3& vValue) const
{
    glUniform3d(
        GetUniformLocation(pszName),
        vValue.x,
        vValue.y,
        vValue.z
    );
}

void CShaderProgram::SetUniformVec4f(const char* pszName, const glm::vec4& vValue) const
{
    glUniform4f(
        GetUniformLocation(pszName),
        vValue.x,
        vValue.y,
        vValue.z,
        vValue.w
    );
}

void CShaderProgram::SetUniform1d(const char* pszName, double dValue) const
{
    glUniform1d(GetUniformLocation(pszName), dValue);
}

void CShaderProgram::SetUniform1f(const char* pszName, float fValue) const
{
    glUniform1f(GetUniformLocation(pszName), fValue);
}

void CShaderProgram::SetUniformVec4d(const char* pszName, const glm::dvec4& vValue) const
{
    glUniform4d(
        GetUniformLocation(pszName),
        vValue.x,
        vValue.y,
        vValue.z,
        vValue.w
    );
}

void CShaderProgram::SetUniform1i(const char* pszName, int nValue) const
{
    glUniform1i(GetUniformLocation(pszName), nValue);
}

std::string CShaderProgram::ReadTextFile(const std::string& szFilePath)
{
    std::ifstream file(szFilePath);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + szFilePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

GLuint CShaderProgram::CompileShader(
    GLenum eShaderType,
    const std::string& szSource,
    const std::string& szDebugName
)
{
    GLuint nShader = glCreateShader(eShaderType);

    const char* pszSource = szSource.c_str();
    glShaderSource(nShader, 1, &pszSource, nullptr);
    glCompileShader(nShader);

    GLint nSuccess = GL_FALSE;
    glGetShaderiv(nShader, GL_COMPILE_STATUS, &nSuccess);

    if (nSuccess != GL_TRUE)
    {
        GLint nLogLength = 0;
        glGetShaderiv(nShader, GL_INFO_LOG_LENGTH, &nLogLength);

        std::string szLog;
        szLog.resize(static_cast<size_t>(nLogLength));

        glGetShaderInfoLog(nShader, nLogLength, nullptr, szLog.data());

        glDeleteShader(nShader);

        throw std::runtime_error("Failed to compile shader: " + szDebugName + "\n" + szLog);
    }

    return nShader;
}

GLuint CShaderProgram::CreateProgram(
    const std::string& szVertexShaderPath,
    const std::string& szFragmentShaderPath
)
{
    const std::string szVertexSource = ReadTextFile(szVertexShaderPath);
    const std::string szFragmentSource = ReadTextFile(szFragmentShaderPath);

    GLuint nVertexShader = CompileShader(GL_VERTEX_SHADER, szVertexSource, szVertexShaderPath);
    GLuint nFragmentShader = CompileShader(GL_FRAGMENT_SHADER, szFragmentSource, szFragmentShaderPath);

    GLuint nProgram = glCreateProgram();

    glAttachShader(nProgram, nVertexShader);
    glAttachShader(nProgram, nFragmentShader);
    glLinkProgram(nProgram);

    glDetachShader(nProgram, nVertexShader);
    glDetachShader(nProgram, nFragmentShader);

    glDeleteShader(nVertexShader);
    glDeleteShader(nFragmentShader);

    GLint nSuccess = GL_FALSE;
    glGetProgramiv(nProgram, GL_LINK_STATUS, &nSuccess);

    if (nSuccess != GL_TRUE)
    {
        GLint nLogLength = 0;
        glGetProgramiv(nProgram, GL_INFO_LOG_LENGTH, &nLogLength);

        std::string szLog;
        szLog.resize(static_cast<size_t>(nLogLength));

        glGetProgramInfoLog(nProgram, nLogLength, nullptr, szLog.data());

        glDeleteProgram(nProgram);

        throw std::runtime_error("Failed to link shader program\n" + szLog);
    }

    return nProgram;
}

GLint CShaderProgram::GetUniformLocation(const char* pszName) const
{
    GLint nLocation = glGetUniformLocation(m_nProgramId, pszName);

    if (nLocation == -1)
    {
        std::cerr << "Warning: uniform not found: " << pszName << '\n';
    }

    return nLocation;
}

void CShaderProgram::Destroy()
{
    if (m_nProgramId != 0)
    {
        glDeleteProgram(m_nProgramId);
        m_nProgramId = 0;
    }
}
