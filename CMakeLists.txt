cmake_minimum_required(VERSION 3.5)
project(LD40)

add_subdirectory(ext/ginseng)
add_subdirectory(ext/lua)
add_subdirectory(ext/sol2)
add_subdirectory(ext/glm)
add_subdirectory(ext/lodepng)
add_subdirectory(ext/sushi)
add_subdirectory(ext/msdfgen)

set(LD40_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Client Output Directory")

# Static Files
file(GLOB_RECURSE LD40_STATIC static/*)
add_custom_target(LD40_static
    COMMENT "Updating client static files"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static "${LD40_WWW_DIR}"
    SOURCES ${LD40_STATIC})

# HTML Shell
set(LD40_SHELL "${CMAKE_SOURCE_DIR}/src/shell.html" CACHE PATH "Emscripten Shell")

# JS Library
file(GLOB_RECURSE LD40_JS "src/*.js")

# Game Data
set(LD40_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Directory")
file(GLOB_RECURSE LD40_DATA_FILES "${LD40_DATA_DIR}/*")

# Game Executable
file(GLOB_RECURSE LD40_SRCS src/*.cpp src/*.hpp)
add_executable(LD40 ${LD40_SRCS} ${LD40_JS} ${LD40_SHELL})
target_include_directories(LD40 PRIVATE src/)
string(CONCAT LD40_LINK_FLAGS
    " -s DISABLE_EXCEPTION_CATCHING=0"
    " -s USE_SDL=2"
    " -s USE_SDL_IMAGE=2"
    " -s USE_SDL_NET=2"
    " -s USE_FREETYPE=1"
    " --preload-file ${LD40_DATA_DIR}@data"
    " --shell-file ${LD40_SHELL}")
string(CONCAT LD40_LINK_FLAGS_DEBUG
    " -g4"
    " --cpuprofiler"
    " --memoryprofiler")
set_target_properties(LD40 PROPERTIES
    CXX_STANDARD 14
    SUFFIX .html
    LINK_FLAGS "${LD40_LINK_FLAGS}"
    LINK_FLAGS_DEBUG "${LD40_LINK_FLAGS_DEBUG}"
    LINK_DEPENDS "${LD40_SHELL};${LD40_DATA_FILES}"
    RUNTIME_OUTPUT_DIRECTORY "${LD40_WWW_DIR}")
em_link_js_library(LD40 ${LD40_JS})
target_link_libraries(LD40
    ginseng
    sol2
    sushi
    msdfgen)
add_dependencies(LD40 LD40_static)

