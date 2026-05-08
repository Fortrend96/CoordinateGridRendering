#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>

// Обёртка над OpenGL shader program.
//
// Класс отвечает за:
// - загрузку shader source из файлов;
// - компиляцию vertex/fragment shader;
// - линковку program;
// - установку uniform-параметров.
//
// Класс владеет OpenGL program id.
class CShaderProgram
{
public:
    // Создаёт пустой shader program.
    CShaderProgram();

    // Удаляет OpenGL program, если он был создан.
    ~CShaderProgram();

    // Копирование запрещено, потому что класс владеет OpenGL resource.
    CShaderProgram(const CShaderProgram&) = delete;

    // Копирующее присваивание запрещено, потому что класс владеет OpenGL resource.
    CShaderProgram& operator=(const CShaderProgram&) = delete;

    // Перемещающий конструктор.
    CShaderProgram(CShaderProgram&& other) noexcept;

    // Перемещающее присваивание.
    CShaderProgram& operator=(CShaderProgram&& other) noexcept;

    // Загружает, компилирует и линкует shader program из файлов.
    void LoadFromFiles(
        const std::string& strVertexShaderPath,
        const std::string& strFragmentShaderPath
    );

    // Делает shader program текущей.
    void Use() const;

    // Удаляет shader program.
    void Destroy();

    // Возвращает OpenGL id shader program.
    GLuint GetProgramId() const;

    // Устанавливает int uniform.
    void SetUniform1i(const std::string& strName, int nValue) const;

    // Устанавливает float uniform.
    void SetUniform1f(const std::string& strName, float fValue) const;

    // Устанавливает double uniform.
    void SetUniform1d(const std::string& strName, double dValue) const;

    // Устанавливает vec4 uniform.
    void SetUniformVec4f(const std::string& strName, const glm::vec4& vValue) const;

    // Устанавливает dvec2 uniform.
    void SetUniformVec2d(const std::string& strName, const glm::dvec2& vValue) const;

    // Устанавливает dvec3 uniform.
    void SetUniformVec3d(const std::string& strName, const glm::dvec3& vValue) const;

    // Устанавливает dvec4 uniform.
    void SetUniformVec4d(const std::string& strName, const glm::dvec4& vValue) const;

    // Устанавливает dmat4 uniform.
    void SetUniformMat4d(const std::string& strName, const glm::dmat4& mValue) const;

private:
    // Читает файл shader'а в строку.
    static std::string ReadTextFile(const std::string& strPath);

    // Компилирует shader указанного типа.
    static GLuint CompileShader(
        GLenum eShaderType,
        const std::string& strSource,
        const std::string& strDebugName
    );

    // Проверяет статус линковки program.
    static void CheckProgramLinkStatus(
        GLuint nProgramId,
        const std::string& strDebugName
    );

    // Возвращает location uniform'а.
    GLint GetUniformLocation(const std::string& strName) const;

private:
    // OpenGL id shader program.
    GLuint m_nProgramId;
};