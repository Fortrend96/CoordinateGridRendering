cmake_minimum_required(VERSION 3.20)

project(CoordinateGridRendering LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Чтобы exe и папка shaders лежали рядом в Debug/Release.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")

# ------------------------------------------------------------------------------
# External libraries
# ------------------------------------------------------------------------------

# GLFW.
add_subdirectory(external/glfw)

# GLAD.
add_subdirectory(external/glad)

# GLM — header-only.
set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(PROJECT_SOURCES
    src/main.cpp

    src/Application.h
    src/Application.cpp

    src/ShaderProgram.h
    src/ShaderProgram.cpp

    src/GridRenderer.h
    src/GridRenderer.cpp

    src/OrbitCamera.h
    src/OrbitCamera.cpp

    src/AxisMarkerRenderer.h
    src/AxisMarkerRenderer.cpp

    src/DemoSceneRenderer.h
    src/DemoSceneRenderer.cpp

    src/ViewUniformBuffer.h
    src/ViewUniformBuffer.cpp
)

set(PROJECT_SHADERS
    shaders/fullscreen_quad.vert
    shaders/grid.frag

    shaders/axis_marker.vert
    shaders/axis_marker.frag

    shaders/object.vert
    shaders/object.frag

    shaders/common/ViewUniformBlock.glsl
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_SHADERS}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${GLM_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        glad
)

# ------------------------------------------------------------------------------
# Visual Studio filters
# ------------------------------------------------------------------------------

source_group("Source Files" FILES
    src/main.cpp
    src/Application.cpp
    src/ShaderProgram.cpp
    src/GridRenderer.cpp
    src/OrbitCamera.cpp
    src/AxisMarkerRenderer.cpp
    src/DemoSceneRenderer.cpp
    src/ViewUniformBuffer.cpp
)

source_group("Header Files" FILES
    src/Application.h
    src/ShaderProgram.h
    src/GridRenderer.h
    src/OrbitCamera.h
    src/AxisMarkerRenderer.h
    src/DemoSceneRenderer.h
    src/ViewUniformBuffer.h
)

source_group("Shaders" FILES
    ${PROJECT_SHADERS}
)

# ------------------------------------------------------------------------------
# Shader copy
# ------------------------------------------------------------------------------

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying shaders to output directory"
)

set_property(
	DIRECTORY
	PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)