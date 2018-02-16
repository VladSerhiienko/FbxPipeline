
import sys
import json
import os.path
import FbxPipeline

def value_id_get_type_and_index(value_id):
    return (value_id & 0x000f), ((value_id >> 8) & 0x0fff)

def gltf_sampler_get_wrap_mode_uv(gltf_sampler_json):
    wrap_s = gltf_sampler_json["wrapS"]
    wrap_t = gltf_sampler_json["wrapT"]
    wrap_mode_u = FbxPipeline.EWrapModeFb.Repeat
    wrap_mode_v = FbxPipeline.EWrapModeFb.Repeat
    if wrap_s == 33071:
        wrap_mode_u = FbxPipeline.EWrapModeFb.Clamp
    if wrap_t == 33071:
        wrap_mode_v = FbxPipeline.EWrapModeFb.Clamp
    return wrap_mode_u, wrap_mode_v

def sync_textures(state, gltf_json):
    FbxPipeline.log_info("sync_textures ...")
    gltf_texture_index_to_texture_index = dict()

    gltf_textures_json = gltf_json["textures"]
    for gltf_texture_index, gltf_texture_json in enumerate(gltf_textures_json):
        gltf_image_index = gltf_texture_json["source"]
        gltf_image_json = gltf_json["images"][gltf_image_index]
        gltf_image_uri = gltf_image_json["uri"]

        FbxPipeline.log_info("gltf texture: source: {}".format(gltf_image_index))
        FbxPipeline.log_info("            : uri: {}".format(gltf_image_uri))

        texture_file_name = os.path.basename(gltf_image_uri)
        texture_file_path = FbxPipeline.find_file(texture_file_name)
        texture_file_id = state.embed_file(texture_file_path)

        FbxPipeline.log_info("            : file_name: {}".format(texture_file_name))
        FbxPipeline.log_info("            : file_path: {}".format(texture_file_path))
        FbxPipeline.log_info("            : file_id: {}".format(texture_file_id))

        if texture_file_id != -1:
            gltf_sampler_index = gltf_texture_json["sampler"]
            gltf_sampler_json = gltf_json["samplers"][gltf_sampler_index]
            wrap_mode_u, wrap_mode_v = gltf_sampler_get_wrap_mode_uv(gltf_sampler_json)

            FbxPipeline.log_info("            : sampler_index: {}".format(gltf_sampler_index))
            FbxPipeline.log_info("            : wrap_mode_u: {} wrap_mode_v: {}".format(wrap_mode_u, wrap_mode_v))

            texture_index = state.textures.size()

            texture = FbxPipeline.TextureFb()
            texture.id = texture_index
            texture.name_id = state.push_string(texture_file_name)
            texture.file_id = texture_file_id
            texture.texture_type_id = state.push_string("TextureVideoClip")
            texture.wrap_mode_u = wrap_mode_u
            texture.wrap_mode_v = wrap_mode_v
            texture.blend_mode = FbxPipeline.EBlendModeFb.Additive
            texture.alpha_source = FbxPipeline.EAlphaSourceFb.Unknown
            texture.mapping_type = FbxPipeline.EMappingTypeFb.UV
            texture.texture_use = FbxPipeline.ETextureUseFb.Standard
            texture.planar_mapping_normal = FbxPipeline.EPlanarMappingNormalFb.PlanarNormalX
            texture.cropping_bottom = 0
            texture.cropping_left = 0
            texture.cropping_right = 0
            texture.cropping_top = 0
            texture.rotation_u = 0
            texture.rotation_v = 0
            texture.rotation_w = 0
            texture.offset_u = 0
            texture.offset_v = 0
            texture.scale_u = 1
            texture.scale_v = 1
            texture.swap_uv = False
            texture.wipe_mode = False
            texture.premultiplied_alpha = True
            state.textures.push_back(texture)
            gltf_texture_index_to_texture_index[gltf_texture_index] = texture_index

    FbxPipeline.log_info("TextureMap: {}".format(gltf_texture_index_to_texture_index))
    return gltf_texture_index_to_texture_index

def find_name_by_name_id(state, name_id):
    name_type, name_index = value_id_get_type_and_index(name_id)
    if name_type == FbxPipeline.EValueTypeFb.String:
        return state.string_values[name_index]
    else:
        return ""

def find_material_index_by_name(state, gltf_material_name):
    for material_index, material in enumerate(state.materials):
        material_name = find_name_by_name_id(state, material.name_id)
        if (material_name == gltf_material_name):
            return material_index
    return -1

def gltf_material_json_to_material(state, gltf_material_json, gltf_texture_index_to_texture_index):
    material = FbxPipeline.Material()
    material.name_id = state.push_string(gltf_material_json["name"])

    FbxPipeline.log_info("gltf material: {}".format(gltf_material_json["name"]))

    if "doubleSided" in gltf_material_json:
        FbxPipeline.log_info("             : doubleSided")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("doubleSided")
        materialProperty.value_id = state.push_bool(gltf_material_json["doubleSided"])
        material.properties.push_back(materialProperty)

    if "alphaCutoff" in gltf_material_json:
        FbxPipeline.log_info("             : alphaCutoff")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("alphaCutoff")
        materialProperty.value_id = state.push_float(gltf_material_json["alphaCutoff"])
        material.properties.push_back(materialProperty)

    if "alphaMode" in gltf_material_json:
        FbxPipeline.log_info("             : alphaMode")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("alphaMode")
        materialProperty.value_id = state.push_string(gltf_material_json["alphaMode"])
        material.properties.push_back(materialProperty)

    if "emissiveFactor" in gltf_material_json:
        FbxPipeline.log_info("             : emissiveFactor")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("emissiveFactor")
        materialProperty.value_id = state.push_float3(gltf_material_json["emissiveFactor"][0],
                                                      gltf_material_json["emissiveFactor"][1],
                                                      gltf_material_json["emissiveFactor"][2])
        material.properties.push_back(materialProperty)

    if "normalTexture" in gltf_material_json:
        FbxPipeline.log_info("             : normalTexture")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("normalTexture")
        materialProperty.value_id = gltf_texture_index_to_texture_index[gltf_material_json["normalTexture"]["index"]]
        material.texture_properties.push_back(materialProperty)

    if "occlusionTexture" in gltf_material_json:
        FbxPipeline.log_info("             : occlusionTexture")
        materialProperty = FbxPipeline.MaterialPropFb()
        materialProperty.name_id = state.push_string("occlusionTexture")
        materialProperty.value_id = gltf_texture_index_to_texture_index[gltf_material_json["occlusionTexture"]["index"]]
        material.texture_properties.push_back(materialProperty)

    if "pbrMetallicRoughness" in gltf_material_json:
        pbr_metallic_roughness_json = gltf_material_json["pbrMetallicRoughness"]

        if "baseColorFactor" in pbr_metallic_roughness_json:
            FbxPipeline.log_info("             : baseColorFactor")
            materialProperty = FbxPipeline.MaterialPropFb()
            materialProperty.name_id = state.push_string("baseColorFactor")
            materialProperty.value_id = state.push_float4(pbr_metallic_roughness_json["baseColorFactor"][0],
                                                          pbr_metallic_roughness_json["baseColorFactor"][1],
                                                          pbr_metallic_roughness_json["baseColorFactor"][2],
                                                          pbr_metallic_roughness_json["baseColorFactor"][3])
            material.properties.push_back(materialProperty)

        if "metallicFactor" in pbr_metallic_roughness_json:
            FbxPipeline.log_info("             : metallicFactor")
            materialProperty = FbxPipeline.MaterialPropFb()
            materialProperty.name_id = state.push_string("metallicFactor")
            materialProperty.value_id = state.push_float(pbr_metallic_roughness_json["metallicFactor"])
            material.properties.push_back(materialProperty)

        if "baseColorTexture" in pbr_metallic_roughness_json:
            FbxPipeline.log_info("             : baseColorTexture")
            materialProperty = FbxPipeline.MaterialPropFb()
            materialProperty.name_id = state.push_string("baseColorTexture")
            materialProperty.value_id = gltf_texture_index_to_texture_index[pbr_metallic_roughness_json["baseColorTexture"]["index"]]
            material.texture_properties.push_back(materialProperty)

        if "metallicRoughnessTexture" in pbr_metallic_roughness_json:
            FbxPipeline.log_info("             : metallicRoughnessTexture")
            materialProperty = FbxPipeline.MaterialPropFb()
            materialProperty.name_id = state.push_string("metallicRoughnessTexture")
            materialProperty.value_id = gltf_texture_index_to_texture_index[pbr_metallic_roughness_json["metallicRoughnessTexture"]["index"]]
            material.texture_properties.push_back(materialProperty)

        if "roughnessFactor" in pbr_metallic_roughness_json:
            FbxPipeline.log_info("             : roughnessFactor")
            materialProperty = FbxPipeline.MaterialPropFb()
            materialProperty.name_id = state.push_string("roughnessFactor")
            materialProperty.value_id = state.push_float(pbr_metallic_roughness_json["roughnessFactor"])
            material.properties.push_back(materialProperty)

    if "extensions" in gltf_material_json:
        gltf_material_extensions_json = gltf_material_json["extensions"]
        if "KHR_materials_pbrSpecularGlossiness" in  gltf_material_extensions_json:
            pbr_metallic_roughness_json = gltf_material_extensions_json["KHR_materials_pbrSpecularGlossiness"]

            if "diffuseFactor" in pbr_metallic_roughness_json:
                FbxPipeline.log_info("             : diffuseFactor")
                materialProperty = FbxPipeline.MaterialPropFb()
                materialProperty.name_id = state.push_string("diffuseFactor")
                materialProperty.value_id = state.push_float4(pbr_metallic_roughness_json["diffuseFactor"][0],
                                                              pbr_metallic_roughness_json["diffuseFactor"][1],
                                                              pbr_metallic_roughness_json["diffuseFactor"][2],
                                                              pbr_metallic_roughness_json["diffuseFactor"][3])
                material.properties.push_back(materialProperty)

            if "diffuseTexture" in pbr_metallic_roughness_json:
                FbxPipeline.log_info("             : diffuseTexture")
                materialProperty = FbxPipeline.MaterialPropFb()
                materialProperty.name_id = state.push_string("diffuseTexture")
                materialProperty.value_id = gltf_texture_index_to_texture_index[pbr_metallic_roughness_json["diffuseTexture"]["index"]]
                material.texture_properties.push_back(materialProperty)

            if "specularFactor" in pbr_metallic_roughness_json:
                FbxPipeline.log_info("             : specularFactor")
                materialProperty = FbxPipeline.MaterialPropFb()
                materialProperty.name_id = state.push_string("specularFactor")
                materialProperty.value_id = state.push_float3(pbr_metallic_roughness_json["specularFactor"][0],
                                                              pbr_metallic_roughness_json["specularFactor"][1],
                                                              pbr_metallic_roughness_json["specularFactor"][2])
                material.properties.push_back(materialProperty)

            if "glossinessFactor" in pbr_metallic_roughness_json:
                FbxPipeline.log_info("             : glossinessFactor")
                materialProperty = FbxPipeline.MaterialPropFb()
                materialProperty.name_id = state.push_string("glossinessFactor")
                materialProperty.value_id = state.push_float(pbr_metallic_roughness_json["glossinessFactor"])
                material.properties.push_back(materialProperty)

            if "specularGlossinessTexture" in pbr_metallic_roughness_json:
                FbxPipeline.log_info("             : specularGlossinessTexture")
                materialProperty = FbxPipeline.MaterialPropFb()
                materialProperty.name_id = state.push_string("specularGlossinessTexture")
                materialProperty.value_id = gltf_texture_index_to_texture_index[pbr_metallic_roughness_json["specularGlossinessTexture"]["index"]]
                material.texture_properties.push_back(materialProperty)

    return material

def gltf_export_materials(state, gltf_path):
    FbxPipeline.log_info("gltf_export_materials: {}".format(gltf_path))

    if (os.path.splitext(gltf_path)[1].lower() == ".gltf" and os.path.isfile(gltf_path)):
        gltf_json = json.load(open(gltf_path))

        if (gltf_json == None):
            FbxPipeline.log_error("Failed to path gltf as json.")
            return

        gltf_materials_json = gltf_json["materials"]
        if (gltf_materials_json == None):
            FbxPipeline.log_error("Failed to get gltf materials from json.")
            return

        gltf_texture_index_to_texture_index = sync_textures(state, gltf_json)
        overrided_materials = list()

        for gltf_material_json in gltf_materials_json:
            gltf_material_name = gltf_material_json["name"]
            material_index = find_material_index_by_name(state, gltf_material_name)

            if material_index != -1:
                overrided_materials.append(state.materials[material_index])
                material = gltf_material_json_to_material(state, gltf_material_json, gltf_texture_index_to_texture_index)
                # material.id = material_index
                state.materials[material_index] = material

        for overrided_material in overrided_materials:
            overrided_material.id = state.materials.size()
            state.materials.push_back(overrided_material)

    else:
        FbxPipeline.log_error("Path {} does not point to file or not a .gltf file.".format(gltf_path))
        return

FbxPipeline.register_extension(gltf_export_materials)