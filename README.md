```
    ________         ____  _            ___          
   / ____/ /_  _  __/ __ \(_)___  ___  / (_)___  ___ 
  / /_  / __ \| |/_/ /_/ / / __ \/ _ \/ / / __ \/ _ \
 / __/ / /_/ />  </ ____/ / /_/ /  __/ / / / / /  __/
/_/   /_.___/_/|_/_/   /_/ .___/\___/_/_/_/ /_/\___/ 
                        /_/                          
```


**FbxPipeline** [![Build status](https://ci.appveyor.com/api/projects/status/lob4wuwoji3awkeq?svg=true)](https://ci.appveyor.com/project/VladSerhiienko/fbxpipeline) [![Build Status](https://travis-ci.org/VladSerhiienko/FbxPipeline.svg?branch=master)](https://travis-ci.org/VladSerhiienko/FbxPipeline) is the command line exporter for the *.FBX* files. It suits for the projects that already use or plan to use **flatbuffers** (Google's library for serialisation, https://google.github.io/flatbuffers/).


## The main advantages are:
 - No libraries needed except *flatbuffers*
 - Python Embedding (glTF plugin example)
 - Single generated header file from the scheme file (the pre-generated file in the repository can be used)
 - Packing for meshes (reduces memory bandwidth)
 - No processing on loading (simply *memcpy* the data and set appropriate *image/buffers formats/attributes*)
 - Binary format (the loading speed is an essential factor; however, the way the file will be serialised depends on flatbuffers, that is very flexible)
 - Animation
 - Skinning
 - Free

![alt text](https://github.com/VladSerhiienko/FbxPipeline/blob/master/docs/AK.PNG "AK")
![alt text](https://github.com/VladSerhiienko/FbxPipeline/blob/master/docs/9mm.PNG "9mm")

## Features, that will be available soon:
 - Animation compression
 - Mesh optimisation (reduces GPU vertex caching and memory bandwidth)
 - Parallelize mesh processing
 - Integration of *zlib/lzma* for compression
 - Image compression (*ETC, PVR*, PVR SDK)

## Command line example
### Fbx + textures 
```sh
-i "$(ModelsDir)knight-artorias\source\Artorias.fbx"
-o "$(SolutionDir)assets\Artoriasv2.fbxp"
-e "$(ModelsDir)knight-artorias\**" -m ".*\.png"
-p
```
### Fbx + materials with textures in glTF
```sh
-i "C:\Sources\Models\knight-artorias\source\Artorias.fbx.fbx"
-o "C:/Sources/Models/knight-artorias.fbxp"
-l "C:/Sources/Models/knight-artorias.txt"
-e "C:/Sources/Models/knight-artorias/**"
-e "C:/Sources/Models/knight_artorias/**"
--script-file "glTFMaterialExtension.py"
--script-input "C:\Sources\Models\knight_artorias\scene.gltf"
```
|Argument|Comment|
|--------|-------|
|-i|Input .FBX file|
|-o|Output .FBX file|
|-p|Enable mesh packing|
|-b|Remove bad polies|
|-e|Sets search location(s) for the files specified for embedding (*two stars* at the end mean recursive look-ups), the option can be used multiple times, for example: **-e** *../path/one/* **-e** *../path/two/\*\** (*all the child folders in ../path/two/ folder will be added recursively*)|
|-m|Embed file, regex (**.\*\\.png** means all the *.png* files), the option can be used multiple times|

## How to build (Linux, bash + cmake + make):

### Bash
```
cd <dev directory>
git clone git@github.com:VladSerhiienko/FbxPipeline.git
cd FbxPipeline
cmake -Bbuild_linux_x86_64_gnu -H.
make -C build_linux_x86_64
```

## How to build (Windows, PS + cmake + MSBuild): 

### PowerShell:
```
cd <dev directory>
git clone git@github.com:VladSerhiienko/FbxPipeline.git
cd FbxPipeline
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_amd64_msvc "-H."
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' build_windows_amd64\FbxPipeline.sln /target:ALL_BUILD /p:Configuration=Debug
```

# License
Licensed under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License. You may obtain a copy of
the License at

<http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.
