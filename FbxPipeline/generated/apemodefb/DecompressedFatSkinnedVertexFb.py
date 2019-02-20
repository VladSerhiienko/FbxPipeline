# automatically generated by the FlatBuffers compiler, do not modify

# namespace: apemodefb

import flatbuffers

class DecompressedFatSkinnedVertexFb(object):
    __slots__ = ['_tab']

    # DecompressedFatSkinnedVertexFb
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # DecompressedFatSkinnedVertexFb
    def DecompressedSkinned(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 0)
        return obj

    # DecompressedFatSkinnedVertexFb
    def ExtraJointIndices(self): return self._tab.Get(flatbuffers.number_types.Uint32Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(84))
    # DecompressedFatSkinnedVertexFb
    def ExtraJointWeights(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 88)
        return obj


def CreateDecompressedFatSkinnedVertexFb(builder, decompressed_skinned_decompressed_position_x, decompressed_skinned_decompressed_position_y, decompressed_skinned_decompressed_position_z, decompressed_skinned_decompressed_uv_x, decompressed_skinned_decompressed_uv_y, decompressed_skinned_decompressed_normal_x, decompressed_skinned_decompressed_normal_y, decompressed_skinned_decompressed_normal_z, decompressed_skinned_decompressed_tangent_x, decompressed_skinned_decompressed_tangent_y, decompressed_skinned_decompressed_tangent_z, decompressed_skinned_decompressed_reflectionIndexPacked, decompressed_skinned_decompressed_color_x, decompressed_skinned_decompressed_color_y, decompressed_skinned_decompressed_color_z, decompressed_skinned_decompressed_color_w, decompressed_skinned_jointIndices, decompressed_skinned_joint_weights_x, decompressed_skinned_joint_weights_y, decompressed_skinned_joint_weights_z, decompressed_skinned_joint_weights_w, extraJointIndices, extra_joint_weights_x, extra_joint_weights_y, extra_joint_weights_z, extra_joint_weights_w):
    builder.Prep(4, 104)
    builder.Prep(4, 16)
    builder.PrependFloat32(extra_joint_weights_w)
    builder.PrependFloat32(extra_joint_weights_z)
    builder.PrependFloat32(extra_joint_weights_y)
    builder.PrependFloat32(extra_joint_weights_x)
    builder.PrependUint32(extraJointIndices)
    builder.Prep(4, 84)
    builder.Prep(4, 16)
    builder.PrependFloat32(decompressed_skinned_joint_weights_w)
    builder.PrependFloat32(decompressed_skinned_joint_weights_z)
    builder.PrependFloat32(decompressed_skinned_joint_weights_y)
    builder.PrependFloat32(decompressed_skinned_joint_weights_x)
    builder.PrependUint32(decompressed_skinned_jointIndices)
    builder.Prep(4, 64)
    builder.Prep(4, 16)
    builder.PrependFloat32(decompressed_skinned_decompressed_color_w)
    builder.PrependFloat32(decompressed_skinned_decompressed_color_z)
    builder.PrependFloat32(decompressed_skinned_decompressed_color_y)
    builder.PrependFloat32(decompressed_skinned_decompressed_color_x)
    builder.PrependUint32(decompressed_skinned_decompressed_reflectionIndexPacked)
    builder.Prep(4, 12)
    builder.PrependFloat32(decompressed_skinned_decompressed_tangent_z)
    builder.PrependFloat32(decompressed_skinned_decompressed_tangent_y)
    builder.PrependFloat32(decompressed_skinned_decompressed_tangent_x)
    builder.Prep(4, 12)
    builder.PrependFloat32(decompressed_skinned_decompressed_normal_z)
    builder.PrependFloat32(decompressed_skinned_decompressed_normal_y)
    builder.PrependFloat32(decompressed_skinned_decompressed_normal_x)
    builder.Prep(4, 8)
    builder.PrependFloat32(decompressed_skinned_decompressed_uv_y)
    builder.PrependFloat32(decompressed_skinned_decompressed_uv_x)
    builder.Prep(4, 12)
    builder.PrependFloat32(decompressed_skinned_decompressed_position_z)
    builder.PrependFloat32(decompressed_skinned_decompressed_position_y)
    builder.PrependFloat32(decompressed_skinned_decompressed_position_x)
    return builder.Offset()
