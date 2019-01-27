# automatically generated by the FlatBuffers compiler, do not modify

# namespace: apemodefb

import flatbuffers

class StaticSkinnedVertexQTangentFb(object):
    __slots__ = ['_tab']

    # StaticSkinnedVertexQTangentFb
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # StaticSkinnedVertexQTangentFb
    def Position(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 0)
        return obj

    # StaticSkinnedVertexQTangentFb
    def Qtangent(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 12)
        return obj

    # StaticSkinnedVertexQTangentFb
    def Uv(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 28)
        return obj

    # StaticSkinnedVertexQTangentFb
    def Weights(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 36)
        return obj

    # StaticSkinnedVertexQTangentFb
    def Indices(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 52)
        return obj


def CreateStaticSkinnedVertexQTangentFb(builder, position_x, position_y, position_z, qtangent_s, qtangent_nx, qtangent_ny, qtangent_nz, uv_x, uv_y, weights_x, weights_y, weights_z, weights_w, indices_x, indices_y, indices_z, indices_w):
    builder.Prep(4, 68)
    builder.Prep(4, 16)
    builder.PrependFloat32(indices_w)
    builder.PrependFloat32(indices_z)
    builder.PrependFloat32(indices_y)
    builder.PrependFloat32(indices_x)
    builder.Prep(4, 16)
    builder.PrependFloat32(weights_w)
    builder.PrependFloat32(weights_z)
    builder.PrependFloat32(weights_y)
    builder.PrependFloat32(weights_x)
    builder.Prep(4, 8)
    builder.PrependFloat32(uv_y)
    builder.PrependFloat32(uv_x)
    builder.Prep(4, 16)
    builder.PrependFloat32(qtangent_nz)
    builder.PrependFloat32(qtangent_ny)
    builder.PrependFloat32(qtangent_nx)
    builder.PrependFloat32(qtangent_s)
    builder.Prep(4, 12)
    builder.PrependFloat32(position_z)
    builder.PrependFloat32(position_y)
    builder.PrependFloat32(position_x)
    return builder.Offset()
