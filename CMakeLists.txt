cmake_minimum_required(VERSION 3.5)
project(LD40)

add_subdirectory(ext/ginseng)
add_subdirectory(ext/lua)
add_subdirectory(ext/sol2)
add_subdirectory(ext/glm)
add_subdirectory(ext/lodepng)
add_subdirectory(ext/msdfgen)
add_subdirectory(ext/soloud)

if (EMSCRIPTEN)
    set(LD40_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Client Output Directory")

    add_subdirectory(ext/sushi)

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

    string(CONCAT LD40_EMSCRIPTEN_PORTS
        " -s USE_SDL=2"
        " -s USE_SDL_IMAGE=2"
        " -s USE_SDL_NET=2"
        " -s USE_FREETYPE=1")

    # Game Executable
    file(GLOB_RECURSE LD40_SRCS src/*.cpp src/*.hpp)
    add_executable(LD40 ${LD40_SRCS} ${LD40_JS} ${LD40_SHELL})
    target_include_directories(LD40 PRIVATE src/)
    string(CONCAT LD40_LINK_FLAGS
        "${LD40_EMSCRIPTEN_PORTS}"
        " -s DISABLE_EXCEPTION_CATCHING=0"
        " -lopenal"
        " --preload-file ${LD40_DATA_DIR}@data"
        " --shell-file ${LD40_SHELL}")
    string(CONCAT LD40_LINK_FLAGS_DEBUG
        " -g4"
        " --cpuprofiler"
        " --memoryprofiler")
    set_target_properties(LD40 PROPERTIES
        CXX_STANDARD 14
        SUFFIX .html
        COMPILE_FLAGS "${LD40_EMSCRIPTEN_PORTS}"
        LINK_FLAGS "${LD40_LINK_FLAGS}"
        LINK_FLAGS_DEBUG "${LD40_LINK_FLAGS_DEBUG}"
        LINK_DEPENDS "${LD40_SHELL};${LD40_DATA_FILES}"
        RUNTIME_OUTPUT_DIRECTORY "${LD40_WWW_DIR}")
    em_link_js_library(LD40 ${LD40_JS})
    target_link_libraries(LD40
        ginseng
        sol2
        sushi
        msdfgen
        soloud)
    add_dependencies(LD40 LD40_static)
else()
    find_package(sdl2 REQUIRED)
    find_package(OpenGL REQUIRED)

    add_subdirectory(ext/glad)
    add_subdirectory(ext/sushi)

    target_compile_definitions(sushi PUBLIC ANGLE_STATIC)

    set(LD40_DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Client Output Directory")

    # Emberjs Shim
    file(GLOB_RECURSE EMBERJS_SHIM_SRCS emberjs_shim_src/*.cpp emberjs_shim_src/*.hpp)
    add_library(emberjs_shim ${EMBERJS_SHIM_SRCS})
    set_target_properties(emberjs_shim PROPERTIES CXX_STANDARD 14)

    # Game Data
    set(LD40_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Directory")
    file(GLOB_RECURSE LD40_DATA_FILES "${LD40_DATA_DIR}/*")

    # Data Files
    add_custom_target(LD40_data
        COMMENT "Updating client data files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${LD40_DATA_DIR}" "${LD40_DIST_DIR}/data"
        SOURCES ${LD40_DATA_FILES})

    message(STATUS "OPENGL STUFF: ${OPENGL_LIBRARIES}")

    # Game Executable
    file(GLOB_RECURSE LD40_SRCS src/*.cpp src/*.hpp)
    add_executable(LD40 ${LD40_SRCS} ${LD40_JS} ${LD40_SHELL})
    target_include_directories(LD40 PRIVATE
        "src"
        ${SDL2_INCLUDE_DIRS})
    set_target_properties(LD40 PROPERTIES
        CXX_STANDARD 14
        RUNTIME_OUTPUT_DIRECTORY "${LD40_DIST_DIR}")
    target_link_libraries(LD40
        emberjs_shim
        ginseng
        sol2
        sushi
        msdfgen
        soloud
        ${SDL2_LIBRARIES}
        glad)
    add_dependencies(LD40 LD40_data)
endif()
