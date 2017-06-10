```
                                 ___                       ___                                          
                                /  /\       _____         /__/|                                         
                               /  /:/_     /  /::\       |  |:|                                         
                              /  /:/ /\   /  /:/\:\      |  |:|                                         
                             /  /:/ /:/  /  /:/~/::\   __|__|:|                                         
                            /__/:/ /:/  /__/:/ /:/\:| /__/::::\____                                     
                            \  \:\/:/   \  \:\/:/~/:/    ~\~~\::::/                                     
                             \  \::/     \  \::/ /:/      |~~|:|~~                                      
                              \  \:\      \  \:\/:/       |  |:|                                        
                               \  \:\      \  \::/        |  |:|                                        
                                \__\/       \__\/         |__|/                                         
      ___                     ___         ___                                     ___           ___     
     /  /\      ___          /  /\       /  /\                      ___          /__/\         /  /\    
    /  /::\    /  /\        /  /::\     /  /:/_                    /  /\         \  \:\       /  /:/_   
   /  /:/\:\  /  /:/       /  /:/\:\   /  /:/ /\    ___     ___   /  /:/          \  \:\     /  /:/ /\  
  /  /:/~/:/ /__/::\      /  /:/~/:/  /  /:/ /:/_  /__/\   /  /\ /__/::\      _____\__\:\   /  /:/ /:/_ 
 /__/:/ /:/  \__\/\:\__  /__/:/ /:/  /__/:/ /:/ /\ \  \:\ /  /:/ \__\/\:\__  /__/::::::::\ /__/:/ /:/ /\
 \  \:\/:/      \  \:\/\ \  \:\/:/   \  \:\/:/ /:/  \  \:\  /:/     \  \:\/\ \  \:\~~\~~\/ \  \:\/:/ /:/
  \  \::/        \__\::/  \  \::/     \  \::/ /:/    \  \:\/:/       \__\::/  \  \:\  ~~~   \  \::/ /:/ 
   \  \:\        /__/:/    \  \:\      \  \:\/:/      \  \::/        /__/:/    \  \:\        \  \:\/:/  
    \  \:\       \__\/      \  \:\      \  \::/        \__\/         \__\/      \  \:\        \  \::/   
     \__\/                   \__\/       \__\/                                   \__\/         \__\/    

```
# FbxPipeline
**FbxPipeline** is the command line exporter for the *.FBX* files. It suits for the projects that already use or plan to use **flatbuffers** (Google's library for serialisation, https://google.github.io/flatbuffers/).

## The main advantages are:
 - No libraries needed except *flatbuffers*
 - Single generated header file from the scheme file (the pre-generated file in the repository can be used)
 - Packing for meshes (reduces memory bandwidth)
 - Mesh optimisation (reduces GPU vertex caching and memory bandwidth)
 - No processing on loading (simply *memcpy* the data and set appropriate *image/buffers formats/attributes*)
 - Binary format (the loading speed is an essential factor; however, the way the file will be serialised depends on flatbuffers, that is very flexible)
 - Free

## Features, that will be available soon:
 - Animation
 - Skinning
 - Parallelize mesh processing
 - Integration of *zlib/lzma* for compression
 - Image compression (*ETC, PVR*, PVR SDK)
 - Animation compression

## Command line example
```sh
-i "$(ModelsDir)knight-artorias\source\Artorias.fbx" -o "$(SolutionDir)assets\Artoriasv2.fbxp" -p -e "$(ModelsDir)knight-artorias\**" -m ".*\.png"
```
|Argument|Comment|
|--------|-------|
|-i, --input-file|Input .FBX file|
|-o, --output-file|Output .FBX file|
|-p,--pack-meshes|Enable mesh packing|
|-e,--search-location|Sets search location(s) for the files specified for embedding (*two stars* at the end mean recursive look-ups), the option can be used multiple times, for example: **-e** *../path/one/* **-e** *../path/two/\*\** (*all the child folders in ../path/two/ folder will be added recursively*)|
|-m,--embed-file|Embed file, regex (**.\*\\.png** means all the *.png* files), the option can be used multiple times|

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
