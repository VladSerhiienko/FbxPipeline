# automatically generated by the FlatBuffers compiler, do not modify

# namespace: apemodefb

import flatbuffers

class SubmeshFb(object):
    __slots__ = ['_tab']

    # SubmeshFb
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # SubmeshFb
    def BboxMin(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 0)
        return obj

    # SubmeshFb
    def BboxMax(self, obj):
        obj.Init(self._tab.Bytes, self._tab.Pos + 12)
        return obj

    # SubmeshFb
    def BaseVertex(self): return self._tab.Get(flatbuffers.number_types.Uint32Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(24))
    # SubmeshFb
    def VertexCount(self): return self._tab.Get(flatbuffers.number_types.Uint32Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(28))
    # SubmeshFb
    def BaseIndex(self): return self._tab.Get(flatbuffers.number_types.Uint32Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(32))
    # SubmeshFb
    def IndexCount(self): return self._tab.Get(flatbuffers.number_types.Uint32Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(36))
    # SubmeshFb
    def BaseSubset(self): return self._tab.Get(flatbuffers.number_types.Uint16Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(40))
    # SubmeshFb
    def SubsetCount(self): return self._tab.Get(flatbuffers.number_types.Uint16Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(42))
    # SubmeshFb
    def VertexFormat(self): return self._tab.Get(flatbuffers.number_types.Uint8Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(44))
    # SubmeshFb
    def CompressionType(self): return self._tab.Get(flatbuffers.number_types.Uint8Flags, self._tab.Pos + flatbuffers.number_types.UOffsetTFlags.py_type(45))

def CreateSubmeshFb(builder, bbox_min_x, bbox_min_y, bbox_min_z, bbox_max_x, bbox_max_y, bbox_max_z, baseVertex, vertexCount, baseIndex, indexCount, baseSubset, subsetCount, vertexFormat, compressionType):
    builder.Prep(4, 48)
    builder.Pad(2)
    builder.PrependUint8(compressionType)
    builder.PrependUint8(vertexFormat)
    builder.PrependUint16(subsetCount)
    builder.PrependUint16(baseSubset)
    builder.PrependUint32(indexCount)
    builder.PrependUint32(baseIndex)
    builder.PrependUint32(vertexCount)
    builder.PrependUint32(baseVertex)
    builder.Prep(4, 12)
    builder.PrependFloat32(bbox_max_z)
    builder.PrependFloat32(bbox_max_y)
    builder.PrependFloat32(bbox_max_x)
    builder.Prep(4, 12)
    builder.PrependFloat32(bbox_min_z)
    builder.PrependFloat32(bbox_min_y)
    builder.PrependFloat32(bbox_min_x)
    return builder.Offset()
