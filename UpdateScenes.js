var fs = require('fs');
var path = require('path');


if (process.argv.length <= 2) {
    console.log("Usage: " + __filename + " <your/path/to/models>");
    process.exit(-1);
}

var splitter = '/';
if (process.platform == 'win32') {
    splitter = "\\";
}

var modelsPath = process.argv[2];
var childDirectories = [];
var sceneSrcFiles = [];

childDirectories.push(modelsPath);
while (childDirectories.length > 0) {
    var currentDirectory = childDirectories.shift();
    items = fs.readdirSync(currentDirectory);

    for (var i = 0; i < items.length; i++) {
        var currentItemAbsolutePath = currentDirectory + splitter + items[i];
        var currentItemStats = fs.statSync(currentItemAbsolutePath);

        if (currentItemStats.isDirectory()) {
            childDirectories.push(currentItemAbsolutePath);
        } else {
            var fileExtension = path.extname(currentItemAbsolutePath).toLowerCase();
            if (fileExtension === '.fbx' || fileExtension === '.dae' || fileExtension === '.obj') {
                sceneSrcFiles.push(currentItemAbsolutePath);
            }
        }
    }
}

var commands = [];
var commandsDrc = [];
var commandsRegDrc = [];
var commandArgExamples = [];

if (process.platform == 'win32') {
    commands.push('cd ./build_msvc_visual_studio_15_2017_win64/Release/');
    commandsDrc.push('cd ./build_msvc_visual_studio_15_2017_win64/Release/');
    commandsRegDrc.push('cd ./build_msvc_visual_studio_15_2017_win64/Release/');
} else if (process.platform == 'darwin') {
    commands.push('cd build_appleclang_xcode/Release');
    commandsDrc.push('cd build_appleclang_xcode/Release');
    commandsRegDrc.push('cd build_appleclang_xcode/Release');
} else {
    commands.push('cd build_linux_x86_64_gnu/Release');
}

commands.push('mkdir "' + modelsPath + splitter + 'FbxPipeline"');
commandsDrc.push('mkdir "' + modelsPath + splitter + 'FbxPipelineDrc"');
commandsRegDrc.push('mkdir "' + modelsPath + splitter + 'FbxPipelineDrc"');

commands.push('mkdir "' + modelsPath + splitter + 'Logs"');
commandsDrc.push('mkdir "' + modelsPath + splitter + 'LogsDrc"');
commandsRegDrc.push('mkdir "' + modelsPath + splitter + 'LogsDrc"');

fs.mkdirIfExistsSync = function (dir) {
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }
};

fs.mkdirIfExistsSync(modelsPath + splitter + 'FbxPipeline');
fs.mkdirIfExistsSync(modelsPath + splitter + 'FbxPipelineDrc');

fs.mkdirIfExistsSync(modelsPath + splitter + 'Logs');
fs.mkdirIfExistsSync(modelsPath + splitter + 'LogsDrc');


function getRegularCommand(sceneSrcFile, sceneName, modelsPath, sceneBaseDirectory, outDirSuffix) {
    var currentCommand = '';
    if (process.platform == 'win32') {
        currentCommand += 'FbxPipelineLauncher.exe';
        currentCommand += ' -i "' + sceneSrcFile + '"';
        currentCommand += ' -o "' + modelsPath + '\\FbxPipeline' + outDirSuffix + '\\' + sceneName + '.fbxp"';
        currentCommand += ' -l "' + modelsPath + '\\Logs' + outDirSuffix + '\\' + sceneName + '.txt"';

    } else {
        currentCommand += './FbxPipelineLauncher';
        currentCommand += ' -i "' + sceneSrcFile + '"';
        currentCommand += ' -o "' + modelsPath + '/FbxPipeline' + outDirSuffix + '/' + sceneName + '.fbxp"';
        currentCommand += ' -l "' + modelsPath + '/Logs' + outDirSuffix + '/' + sceneName + '.txt"';
    }

    if (sceneBaseDirectory !== "")
        currentCommand += ' -e "' + sceneBaseDirectory + '/**"';
    return currentCommand;
}

function getSketchfabCommand(sceneSrcFile, sceneName, modelsPath, sceneBaseDirectory, glTFSceneFile, outDirSuffix) {
    var currentCommand = '';
    if (process.platform == 'win32') {
        currentCommand += 'FbxPipelineLauncher.exe';
        currentCommand += ' -i "' + sceneSrcFile + '"';
        currentCommand += ' -o "' + modelsPath + '\\FbxPipeline' + outDirSuffix + '\\' + sceneName + '.fbxp"';
        currentCommand += ' -l "' + modelsPath + '\\Logs' + outDirSuffix + '\\' + sceneName + '.txt"';

    } else {
        currentCommand += './FbxPipelineLauncher';
        currentCommand += ' -i "' + sceneSrcFile + '"';
        currentCommand += ' -o "' + modelsPath + '/FbxPipeline' + outDirSuffix + '/' + sceneName + '.fbxp"';
        currentCommand += ' -l "' + modelsPath + '/Logs' + outDirSuffix + '/' + sceneName + '.txt"';
    }

    currentCommand += ' -e "' + sceneBaseDirectory + '/**"';
    currentCommand += ' --script-file glTFMaterialExtension.py';
    currentCommand += ' --script-input "' + glTFSceneFile + '"';
    return currentCommand;
}

console.log(sceneSrcFiles);
sceneSrcFiles.forEach(function (sceneSrcFile) {
    var sceneSrcDirectory = path.dirname(sceneSrcFile);
    var sceneSrcDirectoryName = path.basename(sceneSrcDirectory);

    if (sceneSrcDirectoryName === 'source') {
        var sceneBaseDirectory = path.dirname(sceneSrcDirectory);
        var glTFSceneFile = sceneBaseDirectory + splitter + 'scene.gltf';
        if (fs.existsSync(glTFSceneFile)) {
            // var sceneName = path.parse(sceneSrcFile).name;
            var sceneName = path.parse(sceneBaseDirectory).name;
            console.log('Adding project: ' + sceneName);

            // Example command (some parameters are skipped):
            // FbxPipelineLauncher.exe -i "C:/Sources/Models/bristleback-dota-fan-art/source/bristleback-dota-fan-art.fbx"
            // -o "C:/Sources/Models/FbxPipeline/bristleback-dota-fan-art-cubic.fbxp" --resample-framerate 0
            // -l "C:/Sources/Models/Logs/bristleback-dota-fan-art-cubic.txt" -e "C:/Sources/Models/bristleback-dota-fan-art/**"
            // --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/bristleback-dota-fan-art/scene.gltf"

            // --assets /Users/vlad.serhiienko/Projects/Home/Viewer/assets/** --scene
            // /Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/bristleback-dota-fan-art.fbxp

            if (process.platform == 'win32') {
                commandArgExamples.push('--assets "..\\..\\assets\\**" --scene "' + modelsPath + '\\FbxPipeline\\' + sceneName + '.fbxp"');
        
            } else {
                commandArgExamples.push('--assets /Users/vlad.serhiienko/Projects/Home/Viewer/assets/** --scene "' + modelsPath + '/FbxPipeline/' + sceneName + '.fbxp"');
            }
            
            var currentCommand = getSketchfabCommand(sceneSrcFile, sceneName, modelsPath, sceneBaseDirectory, glTFSceneFile, '');
            commands.push(currentCommand);

            var currentCommandDrc = getSketchfabCommand(sceneSrcFile, sceneName, modelsPath, sceneBaseDirectory, glTFSceneFile, 'Drc');
            currentCommandDrc += ' --mesh-compression draco-edgebreaker';
            currentCommandDrc += ' --anim-compression draco-keyframe-animation';
            commandsDrc.push(currentCommandDrc);
        }
    } else {
        var sceneName = path.parse(sceneSrcFile).name;
        console.log('Adding file: ' + sceneName);
        var currentCommandDrc = getRegularCommand(sceneSrcFile, sceneName, modelsPath, sceneSrcDirectory, 'Drc');
        currentCommandDrc += ' --mesh-compression draco-edgebreaker';
        currentCommandDrc += ' --anim-compression draco-keyframe-animation';
        commandsRegDrc.push(currentCommandDrc);
    }
});

var execExt = 'sh';

if (process.platform == 'win32') {
    execExt = 'bat';
}

fs.writeFileSync("ExportScenes." + execExt, commands.join('\n'));
fs.writeFileSync("ExportScenesDrc." + execExt, commandsDrc.join('\n'));
fs.writeFileSync("ExportScenesRegDrc." + execExt, commandsRegDrc.join('\n'));
fs.writeFileSync("CommandArgExamples", commandArgExamples.join('\n'));
