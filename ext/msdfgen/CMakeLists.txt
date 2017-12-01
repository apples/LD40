cmake_minimum_required(VERSION 3.5)
project(msdfgen)

file(GLOB_RECURSE msdfgen_HEADERS
	"core/*.h"
	"lib/*.h"
        "ext/*.h"
	"include/*.h"
)

file(GLOB_RECURSE msdfgen_SOURCES
	"core/*.cpp"
	"lib/*.cpp"
        "ext/*.cpp"
)

add_library(msdfgen ${msdfgen_SOURCES} ${msdfgen_HEADERS})
target_include_directories(msdfgen PUBLIC "include")
target_include_directories(msdfgen INTERFACE ".")

# Freetype
if (EMSCRIPTEN)
    set_target_properties(msdfgen PROPERTIES
        COMPILE_FLAGS "-s USE_FREETYPE=1"
        LINK_FLAGS "-s USE_FREETYPE=1")
else()
    find_package(PkgConfig)
    pkg_search_module(FREETYPE REQUIRED freetype2)
    target_link_libraries(msdfgen ${FREETYPE_LIBRARIES})
    target_include_directories(msdfgen PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_compile_options(msdfgen PUBLIC ${FREETYPE_CFLAGS_OTHER})
endif()
