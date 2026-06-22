cmake_minimum_required(VERSION 3.16)
project(praktika CXX)

# ---- C++ standard ----
# C++20 нужен для корутин Drogon (co_await execSqlCoro / sendRequestCoro).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE / clangd tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Main target ----
add_executable(${PROJECT_NAME} main.cc)

# ---- Drogon framework ----
# Requires Drogon to be installed (see README.md).
# vcpkg:  cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake ..
# MSYS2:  pacman -S mingw-w64-x86_64-drogon
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

# ---- Auto-collect sources from Drogon's conventional folders ----
aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters     FILTER_SRC)
aux_source_directory(plugins     PLUGIN_SRC)
aux_source_directory(models      MODEL_SRC)

target_sources(${PROJECT_NAME} PRIVATE
    ${CTL_SRC}
    ${FILTER_SRC}
    ${PLUGIN_SRC}
    ${MODEL_SRC}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/filters
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# ---- Copy runtime config next to the binary ----
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json
               ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)
