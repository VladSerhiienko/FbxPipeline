#include <fbxppch.h>
#include <fbxpstate.h>

void ExportScene( FbxScene* pScene );
void ConvertScene( FbxManager* lSdkManager, FbxScene* lScene, FbxString lFilePath );

int main( int argc, char** argv ) {

    auto& s = apemode::State::Main( argc, (const char**&) argv );
    bool convert = s.options[ "k" ].as< bool >( );

    if ( s.Initialize( ) ) {
        if ( s.Load( ) ) {
            if ( convert )
                ConvertScene( s.manager, s.scene, s.options[ "i" ].as< std::string >( ).c_str( ) );
            else {
                ExportScene( s.scene );
                s.Finalize( );
            }
        }
    }

    return 0;
}

/*
-i "/home/user/vserhiienko/models_v2/knight_artorias/source/Artorias.fbx.fbx"
-o ""/home/user/vserhiienko/models_v2/knight_artorias.fbxp"
-e "/home/user/vserhiienko/models_v2/knight_artorias/**"
--script-file "glTFMaterialExtension.py"
--script-input "/home/user/vserhiienko/models_v2/knight_artorias/scene.gltf"
*/

/*
-i "C:\Sources\Models\skull_salazar_downloadable\source\model\model.dae" -b -o "C:\Sources\Models\skull_salazar.fbxp" -l "C:\Sources\Models\skull_salazar.txt" -e "C:\Sources\Models\skull_salazar_downloadable\**" --script-file "glTFMaterialExtension.py" --script-input "C:\Sources\Models\skull_salazar_downloadable\scene.gltf"
-i "C:\Sources\Models\graograman__fur_study\source\LIONSKETCH.fbx" -b -o "C:\Sources\Models\graograman.fbxp" -l "C:\Sources\Models\graograman.txt" -e "C:\Sources\Models\graograman__fur_study\**" --script-file "glTFMaterialExtension.py" --script-input "C:\Sources\Models\graograman__fur_study\scene.gltf"
-i "C:/Sources/Models/warcraft-draenei-fanart/source/untitled1.fbx" -o "C:/Sources/Models/warcraft-draenei-fanart.fbxp" -l "C:/Sources/Models/warcraft-draenei-fanart.txt" -e "C:/Sources/Models/warcraft-draenei-fanart/**" -e "C:/Sources/Models/warcraft_draenei_fanart/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/warcraft_draenei_fanart/scene.gltf"
-i "C:/Sources/Models/1972-datsun-240k-gt/source/datsun240k.fbx" -b -o "C:/Sources/Models/1972-datsun-240k-gt.fbxp" -l "C:/Sources/Models/1972-datsun-240k-gt.txt" -e "C:/Sources/Models/1972_datsun_240k_gt/**" -e "C:/Sources/Models/1972-datsun-240k-gt/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/1972_datsun_240k_gt/scene.gltf"
-i "C:/Sources/Models/dreadroamer-free/source/Zbot_Animation.fbx" -b -o "C:/Sources/Models/dreadroamer-free.fbxp" -l "C:/Sources/Models/dreadroamer-free.txt" -e "C:/Sources/Models/dreadroamer-free/**" -e "C:/Sources/Models/dreadroamer_free/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/dreadroamer_free/scene.gltf"
-i "C:\Sources\Models\knight-artorias\source\Artorias.fbx.fbx" -o "C:/Sources/Models/knight-artorias.fbxp" -l "C:/Sources/Models/knight-artorias.txt" -e "C:/Sources/Models/knight-artorias/**" -e "C:/Sources/Models/knight_artorias/**" --script-file "glTFMaterialExtension.py" --script-input "C:\Sources\Models\knight_artorias\scene.gltf"
*/

// -i "C:/Sources/Models/1972-datsun-240k-gt/source/datsun240k.fbx" -o "C:/Sources/Models/1972-datsun-240k-gt.fbxp" -e "C:/Sources/Models/1972_datsun_240k_gt/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/1972_datsun_240k_gt/scene.gltf"
// -i "C:/Sources/Models/bristleback-dota-fan-art/source/POSE.fbx" -o "C:/Sources/Models/bristleback-dota-fan-art.fbxp" -e "C:/Sources/Models/bristleback_dota_fan-art/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/bristleback_dota_fan-art/scene.gltf"
// -i "C:/Sources/Models/warcraft-draenei-fanart/source/untitled1.fbx" -o "C:/Sources/Models/warcraft-draenei-fanart.fbxp" -e "C:/Sources/Models/warcraft_draenei_fanart/**" --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/warcraft_draenei_fanart/scene.gltf"

// -i "C:/Sources/Models/bristleback-dota-fan-art/source/POSE.fbx" -o "C:/Sources/Models/bristleback-dota-fan-art.fbxp"
// -i "C:/Sources/Models/warcraft-draenei-fanart/source/untitled1.fbx" -o "C:/Sources/Models/warcraft-draenei-fanart.fbxp"

// C:/Users/vladyslav.serhiienko/Downloads/apto logo/apto logo.FBX
// C:/Users/vladyslav.serhiienko/Downloads/girl-speedsculpt/source/Merged_PolySphere_4553.fbx
// C:/Users/vladyslav.serhiienko/Downloads/zophrac/source/Gunan_animated.fbx
// C:/Users/vladyslav.serhiienko/Downloads/bristleback-dota-fan-art/source/POSE.fbx

void ConvertScene( FbxManager* lSdkManager, FbxScene* lScene, FbxString lFilePath ) {
    auto& s = apemode::State::Get( );

    const char* lFileTypes[] = {
        "_fbx7ascii.fbx", "FBX ascii (*.fbx)",
        "_fbx6ascii.fbx", "FBX 6.0 ascii (*.fbx)",
        "_obj.obj", "Alias OBJ (*.obj)",
        //"_dae.dae",           "Collada DAE (*.dae)",
        //"_fbx7binary.fbx",     "FBX binary (*.fbx)",
        //"_fbx6binary.fbx", "FBX 6.0 binary (*.fbx)",
        //"_dxf.dxf",           "AutoCAD DXF (*.dxf)"
    };

    const size_t lFileTypeCount = sizeof( lFileTypes ) / sizeof( lFileTypes[ 0 ] ) / 2;

    for ( size_t i = 0; i < lFileTypeCount; ++i ) {
        // Retrieve the writer ID according to the description of file format.
        int lFormat = lSdkManager->GetIOPluginRegistry( )->FindWriterIDByDescription( lFileTypes[ i * 2 + 1 ] );

        // Construct the output file name.
        FbxString lNewFileName = lFilePath + lFileTypes[ i * 2 ];

        // Create an exporter.
        FbxExporter* lExporter = FbxExporter::Create( lSdkManager, "" );

        //
        lSdkManager->GetIOSettings( )->SetBoolProp( EXP_FBX_EMBEDDED, true );
        //

        // Initialize the exporter.
        int lResult = lExporter->Initialize( lNewFileName, lFormat, lSdkManager->GetIOSettings( ) );
        if ( lResult ) {
            // Export the scene.
            lResult = lExporter->Export( lScene );
            if ( lResult ) {
                s.console->info( "Exported {} {}.\n", lNewFileName.Buffer( ), lFileTypes[ i * 2 + 1 ] );
            }
        }

        // Destroy the exporter.
        lExporter->Destroy( );
    }
}