
cmake_minimum_required(VERSION 3.4.1)
project (FbxPipeline)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if (WIN32)
    message(ERROR "Configure FBX SDK For Windows Builds")
else()
    set(FBX_SDK_INCLUDE_DIR "~/FbxSdk/include" CACHE PATH "FBX SDK Include Directory (Required by FbxPipeline)")
    set(FBX_SDK_LIBRARY_DIR "~/FbxSdk/lib" CACHE PATH "FBX SDK Library Directory (Required by FbxPipeline)")
    find_library( FBX_SDK_LIBRARY fbxsdk HINTS ${FBX_SDK_LIBRARY_DIR}/gcc4/x64/release ${FBX_SDK_LIBRARY_DIR}/clang/release )
endif()

message(STATUS "FBX_SDK_INCLUDE_DIR = ${FBX_SDK_INCLUDE_DIR}")
message(STATUS "FBX_SDK_LIBRARY_DIR = ${FBX_SDK_LIBRARY_DIR}")
message(STATUS "FBX_SDK_LIBRARY = ${FBX_SDK_LIBRARY}")

add_executable(
    flatc
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/grpc/src/compiler/cpp_generator.cc
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/grpc/src/compiler/go_generator.cc
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/code_generators.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/flatc.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_cpp.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_fbs.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_general.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_go.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_grpc.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_js.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_php.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_python.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_text.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_parser.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/reflection.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/util.cpp
)

add_custom_command(
    TARGET flatc
    POST_BUILD
    DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
    COMMAND echo Generating Headers From Schemes
    COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    COMMAND ./flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
    COMMAND ./flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dbuildubuntu
    COMMENT "Generating Headers From Schemes"
    VERBATIM
)

target_include_directories(
    flatc
    PUBLIC
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/grpc
)

add_library(
    flatbuffers
    STATIC
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/code_generators.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_gen_text.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/idl_parser.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/reflection.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/src/util.cpp
)

target_include_directories(
    flatbuffers
    PUBLIC
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/grpc
)

add_executable(
    fbxp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpanimation.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpfileutils.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmaterial.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmem.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmesh.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmeshopt.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpnode.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxppacking.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxppch.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpstate.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxptransform.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/main.cpp
)

add_dependencies (
    fbxp
    flatc
)

target_include_directories(
    fbxp
    PUBLIC
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/include
    PRIVATE
    ${FBX_SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/ThirdParty/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/cxxopts/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/gli
    ${CMAKE_SOURCE_DIR}/ThirdParty/gli/gli
    ${CMAKE_SOURCE_DIR}/ThirdParty/glm
    ${CMAKE_SOURCE_DIR}/ThirdParty/glm/glm
    ${CMAKE_SOURCE_DIR}/ThirdParty/lodepng
    ${CMAKE_SOURCE_DIR}/ThirdParty/lua
    ${CMAKE_SOURCE_DIR}/ThirdParty/mathfu/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/mathfu/dependencies/vectorial/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/nuklear
    ${CMAKE_SOURCE_DIR}/ThirdParty/sdl2/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/spdlog/include
)

target_link_libraries( # Specifies the target library.
    fbxp
    flatbuffers
    ${FBX_SDK_LIBRARY}
    pthread
    m
    dl
    stdc++fs
)