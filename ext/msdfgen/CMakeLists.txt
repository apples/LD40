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
target_compile_options(msdfgen PUBLIC "-s" "USE_FREETYPE=1")
set_target_properties(msdfgen PROPERTIES
    LINK_FLAGS "-s USE_FREETYPE=1")
