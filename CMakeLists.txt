cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(szProjectName CoordinateGridRendering)
project(${szProjectName})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/glad)
add_subdirectory(external/glm)

source_group("Source Files" FILES
    src/main.cpp
    src/OrbitCamera.h
    src/OrbitCamera.cpp
)

source_group("Shaders" FILES
    shaders/fullscreen_triangle.vert
    shaders/grid.frag
)

set_source_files_properties(
    shaders/fullscreen_triangle.vert
    shaders/grid.frag
    PROPERTIES
        HEADER_FILE_ONLY TRUE
)

add_executable(${szProjectName}
    src/main.cpp
    src/OrbitCamera.h
    src/OrbitCamera.cpp

    shaders/fullscreen_triangle.vert
    shaders/grid.frag
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${szProjectName})

target_link_libraries(${szProjectName}
    PRIVATE
    glfw
    glad
    glm
)

target_include_directories(${szProjectName}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Копируем папку shaders рядом с exe.
# build/Debug/shaders
# build/Release/shaders
add_custom_command(TARGET ${szProjectName} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${szProjectName}>/shaders
)