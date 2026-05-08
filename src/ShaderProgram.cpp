#include "ShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

CShaderProgram::CShaderProgram()
    : m_nProgramId(0)
{
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

void CShaderProgram::LoadFromFiles(
    const std::string& strVertexShaderPath,
    const std::string& strFragmentShaderPath
)
{
    Destroy();

    const std::string strVertexSource = ReadTextFile(strVertexShaderPath);
    const std::string strFragmentSource = ReadTextFile(strFragmentShaderPath);

    const GLuint nVertexShaderId = CompileShader(
        GL_VERTEX_SHADER,
        strVertexSource,
        strVertexShaderPath
    );

    const GLuint nFragmentShaderId = CompileShader(
        GL_FRAGMENT_SHADER,
        strFragmentSource,
        strFragmentShaderPath
    );

    m_nProgramId = glCreateProgram();

    glAttachShader(m_nProgramId, nVertexShaderId);
    glAttachShader(m_nProgramId, nFragmentShaderId);

    glLinkProgram(m_nProgramId);

    glDetachShader(m_nProgramId, nVertexShaderId);
    glDetachShader(m_nProgramId, nFragmentShaderId);

    glDeleteShader(nVertexShaderId);
    glDeleteShader(nFragmentShaderId);

    CheckProgramLinkStatus(
        m_nProgramId,
        strVertexShaderPath + " + " + strFragmentShaderPath
    );
}

void CShaderProgram::Use() const
{
    glUseProgram(m_nProgramId);
}

void CShaderProgram::Destroy()
{
    if (m_nProgramId != 0)
    {
        glDeleteProgram(m_nProgramId);
        m_nProgramId = 0;
    }
}

GLuint CShaderProgram::GetProgramId() const
{
    return m_nProgramId;
}

void CShaderProgram::SetUniform1i(const std::string& strName, int nValue) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform1i(nLocation, nValue);
}

void CShaderProgram::SetUniform1f(const std::string& strName, float fValue) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform1f(nLocation, fValue);
}

void CShaderProgram::SetUniform1d(const std::string& strName, double dValue) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform1d(nLocation, dValue);
}

void CShaderProgram::SetUniformVec4f(
    const std::string& strName,
    const glm::vec4& vValue
) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform4fv(
        nLocation,
        1,
        glm::value_ptr(vValue)
    );
}

void CShaderProgram::SetUniformVec2d(
    const std::string& strName,
    const glm::dvec2& vValue
) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform2d(
        nLocation,
        vValue.x,
        vValue.y
    );
}

void CShaderProgram::SetUniformVec3d(
    const std::string& strName,
    const glm::dvec3& vValue
) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform3d(
        nLocation,
        vValue.x,
        vValue.y,
        vValue.z
    );
}

void CShaderProgram::SetUniformVec4d(
    const std::string& strName,
    const glm::dvec4& vValue
) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniform4d(
        nLocation,
        vValue.x,
        vValue.y,
        vValue.z,
        vValue.w
    );
}

void CShaderProgram::SetUniformMat4d(
    const std::string& strName,
    const glm::dmat4& mValue
) const
{
    const GLint nLocation = GetUniformLocation(strName);

    if (nLocation < 0)
    {
        return;
    }

    glUniformMatrix4dv(
        nLocation,
        1,
        GL_FALSE,
        glm::value_ptr(mValue)
    );
}

std::string CShaderProgram::ReadTextFile(const std::string& strPath)
{
    std::ifstream fileStream(strPath);

    if (!fileStream.is_open())
    {
        throw std::runtime_error("Failed to open file: " + strPath);
    }

    std::stringstream stringStream;
    stringStream << fileStream.rdbuf();

    return stringStream.str();
}

GLuint CShaderProgram::CompileShader(
    GLenum eShaderType,
    const std::string& strSource,
    const std::string& strDebugName
)
{
    const GLuint nShaderId = glCreateShader(eShaderType);

    const char* pszSource = strSource.c_str();

    glShaderSource(
        nShaderId,
        1,
        &pszSource,
        nullptr
    );

    glCompileShader(nShaderId);

    GLint nCompileStatus = GL_FALSE;

    glGetShaderiv(
        nShaderId,
        GL_COMPILE_STATUS,
        &nCompileStatus
    );

    if (nCompileStatus != GL_TRUE)
    {
        GLint nInfoLogLength = 0;

        glGetShaderiv(
            nShaderId,
            GL_INFO_LOG_LENGTH,
            &nInfoLogLength
        );

        std::vector<char> arrInfoLog(
            static_cast<std::size_t>(std::max(nInfoLogLength, 1))
        );

        glGetShaderInfoLog(
            nShaderId,
            nInfoLogLength,
            nullptr,
            arrInfoLog.data()
        );

        std::string strInfoLog(arrInfoLog.data());

        glDeleteShader(nShaderId);

        throw std::runtime_error(
            "Shader compilation failed: " + strDebugName + "\n" + strInfoLog
        );
    }

    return nShaderId;
}

void CShaderProgram::CheckProgramLinkStatus(
    GLuint nProgramId,
    const std::string& strDebugName
)
{
    GLint nLinkStatus = GL_FALSE;

    glGetProgramiv(
        nProgramId,
        GL_LINK_STATUS,
        &nLinkStatus
    );

    if (nLinkStatus != GL_TRUE)
    {
        GLint nInfoLogLength = 0;

        glGetProgramiv(
            nProgramId,
            GL_INFO_LOG_LENGTH,
            &nInfoLogLength
        );

        std::vector<char> arrInfoLog(
            static_cast<std::size_t>(std::max(nInfoLogLength, 1))
        );

        glGetProgramInfoLog(
            nProgramId,
            nInfoLogLength,
            nullptr,
            arrInfoLog.data()
        );

        std::string strInfoLog(arrInfoLog.data());

        throw std::runtime_error(
            "Shader program link failed: " + strDebugName + "\n" + strInfoLog
        );
    }
}

GLint CShaderProgram::GetUniformLocation(const std::string& strName) const
{
    const GLint nLocation = glGetUniformLocation(
        m_nProgramId,
        strName.c_str()
    );

    if (nLocation < 0)
    {
        std::cerr << "Warning: uniform not found: " << strName << '\n';
    }

    return nLocation;
}