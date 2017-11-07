
cmake_minimum_required(VERSION 3.4.1)
project (FbxPipeline)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
add_definitions("-DKFBX_NODLL")

if (WIN32)
    set(FBX_SDK_INCLUDE_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2017.0.1/include" CACHE PATH "FBX SDK Include Directory")
    set(FBX_SDK_LIBRARY_DEBUG "C:/Program Files/Autodesk/FBX/FBX SDK/2017.0.1/lib/vs2015/x64/debug/libfbxsdk-md.lib" CACHE PATH "FBX SDK Library")
    set(FBX_SDK_LIBRARY_RELEASE "C:/Program Files/Autodesk/FBX/FBX SDK/2017.0.1/lib/vs2015/x64/release/libfbxsdk-md.lib" CACHE PATH "FBX SDK Library")
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
        COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_windows_x86-64/Release/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_windows_x86-64/Release/flatc.exe -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/FbxPipeline/generated/scene_generated.h
        DEPENDS ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs
        COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_linux_x86-64/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -c ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_linux_x86-64/flatc -o ${CMAKE_SOURCE_DIR}/FbxPipeline/generated -s ${CMAKE_SOURCE_DIR}/FbxPipeline/schemes/scene.fbs --gen-mutable
        COMMENT "Generating Headers From Schemes"
        VERBATIM
    )
endif()

add_executable(
    FbxPipeline
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

target_include_directories(
    FbxPipeline
    PUBLIC
    ${CMAKE_SOURCE_DIR}/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/FbxPipeline/FbxPipeline
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/grpc
    ${FBX_SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/ThirdParty/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/cxxopts/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/mathfu/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/mathfu/dependencies/vectorial/include
    ${CMAKE_SOURCE_DIR}/ThirdParty/spdlog/include
)

if (WIN32)
    set(
        third_party_libs
        debug ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_windows_x86-64/Debug/flatbuffers.lib
        optimized ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_windows_x86-64/Release/flatbuffers.lib
        winmm.lib
        imm32.lib
        version.lib
    )
else()
    set(
        third_party_libs
        ${CMAKE_SOURCE_DIR}/ThirdParty/flatbuffers/build_linux_x86-64/libflatbuffers.a
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