#!/bin/sh

mkdir draco_build_v140x86
cd draco_build_v140x86
cmake -G "Visual Studio 14 2015" ../draco/
cd ..

mkdir draco_build_v140x64
cd draco_build_v140x64
cmake -G "Visual Studio 14 2015 Win64" ../draco/
cd ..

cd draco_build_v140x86
/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe draco.sln /property:Configuration=Debug
/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe draco.sln /property:Configuration=Release
cd ..

cd draco_build_v140x64
/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe draco.sln /property:Configuration=Debug
/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe draco.sln /property:Configuration=Release
cd ..