#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>

// Обёртка над OpenGL shader program.
//
// Класс владеет OpenGL program object:
// - создаёт его;
// - удаляет в деструкторе;
// - запрещает копирование;
// - разрешает перемещение.
//
// Это RAII-подход: ресурс автоматически освобождается,
// когда объект CShaderProgram уничтожается.
class CShaderProgram
{
public:
    CShaderProgram();
    CShaderProgram(const std::string& szVertexShaderPath, const std::string& szFragmentShaderPath);
    ~CShaderProgram();

    CShaderProgram(const CShaderProgram&) = delete;
    CShaderProgram& operator=(const CShaderProgram&) = delete;

    CShaderProgram(CShaderProgram&& other) noexcept;
    CShaderProgram& operator=(CShaderProgram&& other) noexcept;

    void LoadFromFiles(const std::string& szVertexShaderPath, const std::string& szFragmentShaderPath);
    void Use() const;

    GLuint GetProgramId() const;

    void SetUniformMat4d(const char* pszName, const glm::dmat4& mValue) const;
    void SetUniformVec2d(const char* pszName, const glm::dvec2& vValue) const;
    void SetUniformVec3d(const char* pszName, const glm::dvec3& vValue) const;
    void SetUniformVec4f(const char* pszName, const glm::vec4& vValue) const;
    void SetUniform1d(const char* pszName, double dValue) const;
    void SetUniform1f(const char* pszName, float fValue) const;

private:
    static std::string ReadTextFile(const std::string& szFilePath);
    static GLuint CompileShader(GLenum eShaderType, const std::string& szSource, const std::string& szDebugName);
    static GLuint CreateProgram(const std::string& szVertexShaderPath, const std::string& szFragmentShaderPath);

    GLint GetUniformLocation(const char* pszName) const;
    void Destroy();

private:
    GLuint m_nProgramId;
};