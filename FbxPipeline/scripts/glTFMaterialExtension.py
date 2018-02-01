
import sys
import json
import os.path
from pprint import pprint

import FbxPipeline

def glTFExportMaterials(state, gltf_path):
    print("glTFExportMaterials: " + gltf_path)
    if (os.path.isfile(gltf_path)):
        data = json.load(open(gltf_path))
        pprint(data)

FbxPipeline.RegisterExtension(glTFExportMaterials)