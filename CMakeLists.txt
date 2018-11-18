cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)
project(FbxPipeline)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")

set(BUILD_FOLDER_SUFFIX "build_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}_${CMAKE_CXX_COMPILER_ID}")
string(TOLOWER  ${BUILD_FOLDER_SUFFIX} BUILD_FOLDER_SUFFIX)
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

message(STATUS "FBX_SDK_INCLUDE_DIR = ${FBX_SDK_INCLUDE_DIR}")
message(STATUS "FBX_SDK_LIBRARY_DEBUG = ${FBX_SDK_LIBRARY_DEBUG}")
message(STATUS "FBX_SDK_LIBRARY_RELEASE = ${FBX_SDK_LIBRARY_RELEASE}")

if (FBX_SDK_INCLUDE_DIR STREQUAL "")
    message(FATAL_ERROR "FBXSDK not installed (FBX_SDK_INCLUDE_DIR not found).")
elseif(FBX_SDK_LIBRARY_DEBUG STREQUAL "")
    message(FATAL_ERROR "FBXSDK not installed (FBX_SDK_LIBRARY_DEBUG not found).")
elseif(FBX_SDK_LIBRARY_RELEASE STREQUAL "")
    message(FATAL_ERROR "FBXSDK not installed (FBX_SDK_LIBRARY_RELEASE not found).")
endif()

if (APPLE)
    find_package(Boost COMPONENTS system filesystem REQUIRED)
    message(STATUS "Boost_INCLUDE_DIRS = {Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_SYSTEM_LIBRARY = {Boost_SYSTEM_LIBRARY}")
    message(STATUS "Boost_FILESYSTEM_LIBRARY = {Boost_FILESYSTEM_LIBRARY}")
endif()

if (WIN32)
    set( default_cmake_args
         -G "Visual Studio 15 2017 Win64"
         -B${BUILD_FOLDER_SUFFIX} "-H." )
elseif(APPLE)
    set( default_cmake_args
         -B${BUILD_FOLDER_SUFFIX} -H. )
else()
    set( default_cmake_args
         -B${BUILD_FOLDER_SUFFIX} -H. )
endif()

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
    dlib
    GIT_REPOSITORY "git@github.com:VladSerhiienko/dlib.git"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/dlib"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(dlib SOURCE_DIR)
ExternalProject_Get_Property(dlib BINARY_DIR)
set(dlib_source_dir ${SOURCE_DIR})
set(dlib_binary_dir ${BINARY_DIR})
message(STATUS "dlib_source_dir = ${dlib_source_dir}")
message(STATUS "dlib_binary_dir = ${dlib_binary_dir}")

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
    GIT_TAG "v1.8.0"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/flatbuffers"
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
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
    CMAKE_ARGS ${default_cmake_args}
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
    pybind11
    GIT_REPOSITORY "git@github.com:pybind/pybind11.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/pybind11"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(pybind11 SOURCE_DIR)
ExternalProject_Get_Property(pybind11 BINARY_DIR)
set(pybind11_source_dir ${SOURCE_DIR})
set(pybind11_binary_dir ${BINARY_DIR})
message(STATUS "pybind11_source_dir = ${pybind11_source_dir}")
message(STATUS "pybind11_binary_dir = ${pybind11_binary_dir}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if (WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
endif()

find_package(PythonLibs REQUIRED)
if ( PYTHONLIBS_FOUND )
    get_filename_component(PYTHON_LIBRARY_DIR ${PYTHON_LIBRARY} DIRECTORY)
    get_filename_component(PYTHON_DIR ${PYTHON_LIBRARY_DIR} DIRECTORY)
    message(STATUS "PYTHONLIBS_VERSION_STRING = ${PYTHONLIBS_VERSION_STRING}")
    message(STATUS "PYTHON_LIBRARY = ${PYTHON_LIBRARY}")
    message(STATUS "PYTHON_LIBRARY_DIR = ${PYTHON_LIBRARY_DIR}")
    message(STATUS "PYTHON_DIR = ${PYTHON_DIR}")
endif ()

set (CMAKE_CXX_STANDARD 11)
if (WIN32)
    set(CONFIGURATION_SUFFIX "/$(Configuration)")
elseif (APPLE AND "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    set(CONFIGURATION_SUFFIX "/Debug")
    # TOFIX
    # set(CONFIGURATION_SUFFIX "/$CONFIGURATION")
else()
    set(CONFIGURATION_SUFFIX "")
    add_definitions("-g")
endif()

if (WIN32)
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -p ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
elseif (APPLE)
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -p ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${flatbuffers_binary_dir}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${flatbuffers_binary_dir}/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -p ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
endif()

add_library(
    FbxPipeline
    SHARED
    ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
    ${CMAKE_SOURCE_DIR}/FbxPipeline/scripts/glTFMaterialExtension.py
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpnorm.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpstate.h
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpbez.cpp
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
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpextensions.cpp
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/fbxpembeddedpy.cpp
)

if( MSVC )
    target_compile_options(
        FbxPipeline
        PRIVATE
        /bigobj
    )

    file(
        GLOB
        python_dlls_pdbs
        ${PYTHON_DIR}/*.dll
        ${PYTHON_DIR}/*.pdb
    )

    if( PYTHONLIBS_FOUND )
        message(STATUS "Adding python dynamic lib and debug files to output directory")
        foreach( python_dll_pdb ${python_dlls_pdbs} )
            message(STATUS "Adding ${python_dll_pdb} to output directory")
            add_custom_command(
                TARGET
                FbxPipeline
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${python_dll_pdb}
                ${CMAKE_BINARY_DIR}${CONFIGURATION_SUFFIX}
            )
        endforeach()
    endif()
endif()

file(
    GLOB
    fbxpipeline_scripts
    ${CMAKE_SOURCE_DIR}/FbxPipeline/scripts/*
)

foreach( fbxpipeline_script ${fbxpipeline_scripts} )
    message(STATUS "Adding ${fbxpipeline_script} to script directory")
    add_custom_command(
        TARGET
        FbxPipeline
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${fbxpipeline_script}
        ${CMAKE_BINARY_DIR}/Debug
    )
    add_custom_command(
        TARGET
        FbxPipeline
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${fbxpipeline_script}
        ${CMAKE_BINARY_DIR}/Release
    )
endforeach()

target_compile_definitions(
    FbxPipeline
    PRIVATE
    FBXPIPELINE_EXPORT=1
)

add_dependencies(
    FbxPipeline
    mathfu
    cxxopts
    spdlog
    zlib
    flatbuffers
    pybind11
    dlib
)

target_include_directories(
    FbxPipeline
    PUBLIC
    ${PYTHON_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline
    ${CMAKE_SOURCE_DIR}/dependencies/dlib
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/dependencies/vectorial/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${flatbuffers_source_dir}/include
    ${flatbuffers_source_dir}/grpc
    ${pybind11_source_dir}/include
    ${FBX_SDK_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

if (WIN32)
    set(
        third_party_libs
        debug ${dlib_binary_dir}/dlib${CONFIGURATION_SUFFIX}/dlib19.16.99_debug_64bit_msvc1912.lib
        debug ${zlib_binary_dir}${CONFIGURATION_SUFFIX}/zlibstaticd.lib
        debug ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        optimized ${zlib_binary_dir}${CONFIGURATION_SUFFIX}/zlibstatic.lib
        optimized ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        optimized ${dlib_binary_dir}/dlib${CONFIGURATION_SUFFIX}/dlib19.16.99_release_64bit_msvc1912.lib
        ${PYTHON_LIBRARY}
        winmm.lib
        imm32.lib
        version.lib
    )
elseif(APPLE)
    set(
        third_party_libs
        ${zlib_binary_dir}${CONFIGURATION_SUFFIX}/libz.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
        ${PYTHON_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        "-framework Cocoa"
        pthread
        m
        dl
    )
else()
    set(
        third_party_libs
        ${dlib_binary_dir}/dlib/libdlib.a
        ${zlib_binary_dir}/libz.a
        ${flatbuffers_binary_dir}/libflatbuffers.a
        ${PYTHON_LIBRARY}
        pthread
        stdc++fs
        m
        dl
    )
endif()

target_link_libraries(
    FbxPipeline
    debug ${FBX_SDK_LIBRARY_DEBUG}
    optimized ${FBX_SDK_LIBRARY_RELEASE}
    ${third_party_libs}
)

add_executable(
    FbxPipelineLauncher
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline/main.cpp
)

target_include_directories(
    FbxPipelineLauncher
    PUBLIC
    ${PYTHON_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/include
    ${CMAKE_SOURCE_DIR}/dependencies/mathfu/dependencies/vectorial/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${flatbuffers_source_dir}/include
    ${flatbuffers_source_dir}/grpc
    ${pybind11_source_dir}/include
    ${FBX_SDK_INCLUDE_DIR}
)

target_link_libraries(
    FbxPipelineLauncher
    FbxPipeline
)

set_target_properties(
    FbxPipelineLauncher
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")