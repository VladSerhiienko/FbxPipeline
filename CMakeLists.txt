cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)
project(FbxPipeline)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "VK_SDK_PATH = $ENV{VK_SDK_PATH}")

set(BUILD_FOLDER_SUFFIX "build_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER  ${BUILD_FOLDER_SUFFIX} BUILD_FOLDER_SUFFIX)
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

ExternalProject_Add(
    mathfu
    GIT_REPOSITORY "git@github.com:google/mathfu.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/mathfu"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(mathfu SOURCE_DIR)
set(mathfu_source_dir ${SOURCE_DIR})
message(STATUS "mathfu_source_dir = ${mathfu_source_dir}")

ExternalProject_Add(
    cxxopts
    GIT_REPOSITORY "git@github.com:jarro2783/cxxopts.git"
    GIT_TAG "1_0"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/cxxopts"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(cxxopts SOURCE_DIR)
set(cxxopts_source_dir ${SOURCE_DIR})
message(STATUS "cxxopts_source_dir = ${cxxopts_source_dir}")

ExternalProject_Add(
    spdlog
    GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/spdlog"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(spdlog SOURCE_DIR)
set(spdlog_source_dir ${SOURCE_DIR})
message(STATUS "spdlog_source_dir = ${spdlog_source_dir}")

ExternalProject_Add(
    flatbuffers
    GIT_REPOSITORY "git@github.com:google/flatbuffers.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/flatbuffers"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H.
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(flatbuffers SOURCE_DIR)
ExternalProject_Get_Property(flatbuffers BINARY_DIR)
set(flatbuffers_source_dir ${SOURCE_DIR})
set(flatbuffers_binary_dir ${BINARY_DIR})
message(STATUS "flatbuffers_source_dir = ${flatbuffers_source_dir}")
message(STATUS "flatbuffers_binary_dir = ${flatbuffers_binary_dir}")

ExternalProject_Add(
    zlib
    GIT_REPOSITORY "git@github.com:madler/zlib.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/zlib"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H.
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(zlib SOURCE_DIR)
ExternalProject_Get_Property(zlib BINARY_DIR)
set(zlib_source_dir ${SOURCE_DIR})
set(zlib_binary_dir ${BINARY_DIR})
message(STATUS "zlib_source_dir = ${zlib_source_dir}")
message(STATUS "zlib_binary_dir = ${zlib_binary_dir}")

ExternalProject_Add(
    rapidjson
    GIT_REPOSITORY "git@github.com:Tencent/rapidjson.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/rapidjson"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H. -DRAPIDJSON_BUILD_DOC=OFF
                                            -DRAPIDJSON_BUILD_TESTS=OFF
                                            -DRAPIDJSON_BUILD_EXAMPLES=OFF
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(rapidjson SOURCE_DIR)
ExternalProject_Get_Property(rapidjson BINARY_DIR)
set(rapidjson_source_dir ${SOURCE_DIR})
set(rapidjson_binary_dir ${BINARY_DIR})
message(STATUS "rapidjson_source_dir = ${rapidjson_source_dir}")
message(STATUS "rapidjson_binary_dir = ${rapidjson_binary_dir}")

ExternalProject_Add(
    pybind11
    GIT_REPOSITORY "git@github.com:pybind/pybind11.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/pybind11"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H. -DPYBIND11_TEST=OFF
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(pybind11 SOURCE_DIR)
ExternalProject_Get_Property(pybind11 BINARY_DIR)
set(pybind11_source_dir ${SOURCE_DIR})
set(pybind11_binary_dir ${BINARY_DIR})
message(STATUS "pybind11_source_dir = ${pybind11}")
message(STATUS "pybind11_binary_dir = ${pybind11_binary_dir}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if (WIN32)
    set(FBX_SDK_INCLUDE_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2018.1.1/include" CACHE PATH "FBX SDK Include Directory")
    set(FBX_SDK_LIBRARY_DEBUG "C:/Program Files/Autodesk/FBX/FBX SDK/2018.1.1/lib/vs2015/x64/debug/libfbxsdk-md.lib" CACHE PATH "FBX SDK Library")
    set(FBX_SDK_LIBRARY_RELEASE "C:/Program Files/Autodesk/FBX/FBX SDK/2018.1.1/lib/vs2015/x64/release/libfbxsdk-md.lib" CACHE PATH "FBX SDK Library")
else()
    add_definitions("-g")
    set(FBX_SDK_INCLUDE_DIR "~/FbxSdk/include" CACHE PATH "FBX SDK Include Directory")
    set(FBX_SDK_LIBRARY_DIR "~/FbxSdk/lib" CACHE PATH "FBX SDK Library Directory")
    find_library( FBX_SDK_LIBRARY_RELEASE fbxsdk HINTS ${FBX_SDK_LIBRARY_DIR}/gcc4/x64/release ${FBX_SDK_LIBRARY_DIR}/clang/release )
    set(FBX_SDK_LIBRARY_DEBUG ${FBX_SDK_LIBRARY_RELEASE} CACHE PATH "FBX SDK Library")
endif()

message(STATUS "FBX_SDK_INCLUDE_DIR = ${FBX_SDK_INCLUDE_DIR}")
message(STATUS "FBX_SDK_LIBRARY_DEBUG = ${FBX_SDK_LIBRARY_DEBUG}")
message(STATUS "FBX_SDK_LIBRARY_RELEASE = ${FBX_SDK_LIBRARY_RELEASE}")

if (WIN32)
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${flatbuffers_binary_dir}/Debug/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}/Debug/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${flatbuffers_binary_dir}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
endif()

add_executable(
    FbxPipeline
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpnorm.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpstate.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpanimation.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpfileutils.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmaterial.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmem.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpcam.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxplight.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmesh.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpmeshopt.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpnode.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxppacking.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxppch.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpstate.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxptransform.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/main.cpp
)

add_dependencies(
    FbxPipeline
    mathfu
    cxxopts
    spdlog
    zlib
    flatbuffers
    rapidjson
    pybind11
)

target_include_directories(
    FbxPipeline
    PUBLIC
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/dependencies/vectorial/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${flatbuffers_source_dir}/include
    ${flatbuffers_source_dir}/grpc
    ${rapidjson_source_dir}/include
    ${FBX_SDK_INCLUDE_DIR}
)

if (WIN32)
    set(
        third_party_libs
        debug ${zlib_binary_dir}/Debug/zlibstaticd.lib
        debug ${flatbuffers_binary_dir}/Debug/flatbuffers.lib
        optimized ${zlib_binary_dir}/Release/zlibstatic.lib
        optimized ${flatbuffers_binary_dir}/Release/flatbuffers.lib
        winmm.lib
        imm32.lib
        version.lib
    )
else()
    set(
        third_party_libs
        ${zlib_binary_dir}/libz.a
        ${flatbuffers_binary_dir}/libflatbuffers.a
        pthread
        stdc++fs
        m
        dl
    )
endif()

target_link_libraries( # Specifies the target library.
    FbxPipeline
    debug ${FBX_SDK_LIBRARY_DEBUG}
    optimized ${FBX_SDK_LIBRARY_RELEASE}
    ${third_party_libs}
)