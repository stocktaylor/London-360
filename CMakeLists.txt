# london - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(london LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# The launch menu and in-game menu (../rexlauncher). OFF for builds where it
# can't run, such as WebAssembly (it restarts the process with execv).
option(LONDON_LAUNCHER "Build with the rexlauncher menus" ON)

# Sources
set(LONDON_SOURCES
    src/main.cpp
    src/london_hooks.cpp
)

if(WIN32)
    add_executable(london WIN32 ${LONDON_SOURCES})
else()
    add_executable(london ${LONDON_SOURCES})
endif()

target_compile_definitions(london PRIVATE LONDON_LAUNCHER=$<BOOL:${LONDON_LAUNCHER}>)

rexglue_setup_target(london GPU_PLUGINS xenos)

# In-window launch menu shared by the recomp projects (../rexlauncher).
if(LONDON_LAUNCHER)
    add_subdirectory(../rexlauncher rexlauncher)
    target_link_libraries(london PRIVATE rexlauncher)
endif()
