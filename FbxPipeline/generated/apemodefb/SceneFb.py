# automatically generated by the FlatBuffers compiler, do not modify

# namespace: apemodefb

import flatbuffers

class SceneFb(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAsSceneFb(cls, buf, offset):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = SceneFb()
        x.Init(buf, n + offset)
        return x

    # SceneFb
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # SceneFb
    def Version(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

    # SceneFb
    def Transforms(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 144
            from .TransformFb import TransformFb
            obj = TransformFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def TransformsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def TransformsLimits(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 92
            from .TransformLimitsFb import TransformLimitsFb
            obj = TransformLimitsFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def TransformsLimitsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Nodes(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .NodeFb import NodeFb
            obj = NodeFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def NodesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Meshes(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .MeshFb import MeshFb
            obj = MeshFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def MeshesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def AnimStacks(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 8
            from .AnimStackFb import AnimStackFb
            obj = AnimStackFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def AnimStacksLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def AnimLayers(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(16))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 16
            from .AnimLayerFb import AnimLayerFb
            obj = AnimLayerFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def AnimLayersLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(16))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def AnimCurves(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(18))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .AnimCurveFb import AnimCurveFb
            obj = AnimCurveFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def AnimCurvesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(18))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Materials(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(20))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .MaterialFb import MaterialFb
            obj = MaterialFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def MaterialsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(20))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Textures(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(22))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 72
            from .TextureFb import TextureFb
            obj = TextureFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def TexturesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(22))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Cameras(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(24))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 28
            from .CameraFb import CameraFb
            obj = CameraFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def CamerasLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(24))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Lights(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(26))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 64
            from .LightFb import LightFb
            obj = LightFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def LightsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(26))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Skins(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(28))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .SkinFb import SkinFb
            obj = SkinFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def SkinsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(28))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def Files(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from .FileFb import FileFb
            obj = FileFb()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # SceneFb
    def FilesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def BoolValues(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(32))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.BoolFlags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 1))
        return 0

    # SceneFb
    def BoolValuesAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(32))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.BoolFlags, o)
        return 0

    # SceneFb
    def BoolValuesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(32))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def IntValues(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Int32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # SceneFb
    def IntValuesAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Int32Flags, o)
        return 0

    # SceneFb
    def IntValuesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def FloatValues(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(36))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Float32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # SceneFb
    def FloatValuesAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(36))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Float32Flags, o)
        return 0

    # SceneFb
    def FloatValuesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(36))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # SceneFb
    def StringValues(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(38))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.String(a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return ""

    # SceneFb
    def StringValuesLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(38))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

def SceneFbStart(builder): builder.StartObject(18)
def SceneFbAddVersion(builder, version): builder.PrependUint8Slot(0, version, 0)
def SceneFbAddTransforms(builder, transforms): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(transforms), 0)
def SceneFbStartTransformsVector(builder, numElems): return builder.StartVector(144, numElems, 4)
def SceneFbAddTransformsLimits(builder, transformsLimits): builder.PrependUOffsetTRelativeSlot(2, flatbuffers.number_types.UOffsetTFlags.py_type(transformsLimits), 0)
def SceneFbStartTransformsLimitsVector(builder, numElems): return builder.StartVector(92, numElems, 4)
def SceneFbAddNodes(builder, nodes): builder.PrependUOffsetTRelativeSlot(3, flatbuffers.number_types.UOffsetTFlags.py_type(nodes), 0)
def SceneFbStartNodesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddMeshes(builder, meshes): builder.PrependUOffsetTRelativeSlot(4, flatbuffers.number_types.UOffsetTFlags.py_type(meshes), 0)
def SceneFbStartMeshesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddAnimStacks(builder, animStacks): builder.PrependUOffsetTRelativeSlot(5, flatbuffers.number_types.UOffsetTFlags.py_type(animStacks), 0)
def SceneFbStartAnimStacksVector(builder, numElems): return builder.StartVector(8, numElems, 4)
def SceneFbAddAnimLayers(builder, animLayers): builder.PrependUOffsetTRelativeSlot(6, flatbuffers.number_types.UOffsetTFlags.py_type(animLayers), 0)
def SceneFbStartAnimLayersVector(builder, numElems): return builder.StartVector(16, numElems, 4)
def SceneFbAddAnimCurves(builder, animCurves): builder.PrependUOffsetTRelativeSlot(7, flatbuffers.number_types.UOffsetTFlags.py_type(animCurves), 0)
def SceneFbStartAnimCurvesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddMaterials(builder, materials): builder.PrependUOffsetTRelativeSlot(8, flatbuffers.number_types.UOffsetTFlags.py_type(materials), 0)
def SceneFbStartMaterialsVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddTextures(builder, textures): builder.PrependUOffsetTRelativeSlot(9, flatbuffers.number_types.UOffsetTFlags.py_type(textures), 0)
def SceneFbStartTexturesVector(builder, numElems): return builder.StartVector(72, numElems, 4)
def SceneFbAddCameras(builder, cameras): builder.PrependUOffsetTRelativeSlot(10, flatbuffers.number_types.UOffsetTFlags.py_type(cameras), 0)
def SceneFbStartCamerasVector(builder, numElems): return builder.StartVector(28, numElems, 4)
def SceneFbAddLights(builder, lights): builder.PrependUOffsetTRelativeSlot(11, flatbuffers.number_types.UOffsetTFlags.py_type(lights), 0)
def SceneFbStartLightsVector(builder, numElems): return builder.StartVector(64, numElems, 4)
def SceneFbAddSkins(builder, skins): builder.PrependUOffsetTRelativeSlot(12, flatbuffers.number_types.UOffsetTFlags.py_type(skins), 0)
def SceneFbStartSkinsVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddFiles(builder, files): builder.PrependUOffsetTRelativeSlot(13, flatbuffers.number_types.UOffsetTFlags.py_type(files), 0)
def SceneFbStartFilesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddBoolValues(builder, boolValues): builder.PrependUOffsetTRelativeSlot(14, flatbuffers.number_types.UOffsetTFlags.py_type(boolValues), 0)
def SceneFbStartBoolValuesVector(builder, numElems): return builder.StartVector(1, numElems, 1)
def SceneFbAddIntValues(builder, intValues): builder.PrependUOffsetTRelativeSlot(15, flatbuffers.number_types.UOffsetTFlags.py_type(intValues), 0)
def SceneFbStartIntValuesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddFloatValues(builder, floatValues): builder.PrependUOffsetTRelativeSlot(16, flatbuffers.number_types.UOffsetTFlags.py_type(floatValues), 0)
def SceneFbStartFloatValuesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbAddStringValues(builder, stringValues): builder.PrependUOffsetTRelativeSlot(17, flatbuffers.number_types.UOffsetTFlags.py_type(stringValues), 0)
def SceneFbStartStringValuesVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def SceneFbEnd(builder): return builder.EndObject()
