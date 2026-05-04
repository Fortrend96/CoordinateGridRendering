#version 430 core

layout(location = 0) out vec4 outColor;

void main()
{
    // Пока просто красим весь экран.
    // Это проверка, что:
    // 1. OpenGL-контекст создан;
    // 2. GLAD работает;
    // 3. шейдеры компилируются;
    // 4. draw call реально выполняется.
    outColor = vec4(0.15, 0.20, 0.35, 1.0);
}
