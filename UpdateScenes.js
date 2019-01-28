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
var commandsQt = [];
var commandsDrc = [];
var commandsDrcQt = [];
var commandArgExamples = [];

if (process.platform == 'win32') {
    commands.push('cd ./build_windows_amd64_msvc/Release/');
} else if (process.platform == 'darwin') {
    commands.push('cd build_darwin_x86_64_appleclang_xcode/Release');
} else {
    commands.push('cd build_linux_x86_64_gnu/Release');
}

commands.push('mkdir "' + modelsPath + splitter + 'FbxPipeline"');
commands.push('mkdir "' + modelsPath + splitter + 'Logs"');

fs.mkdirIfExistsSync = function (dir) {
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }``
};

if (process.platform == 'win32') {
    fs.mkdirIfExistsSync(modelsPath + '\\FbxPipeline');
    fs.mkdirIfExistsSync(modelsPath + '\\Logs');
} else {
    fs.mkdirIfExistsSync(modelsPath + '/FbxPipeline');
    fs.mkdirIfExistsSync(modelsPath + '/Logs');
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
            
            var currentCommand = '';
            if (process.platform == 'win32') {
                currentCommand += 'FbxPipelineLauncher.exe';
                currentCommand += ' -i "' + sceneSrcFile + '"';
                currentCommand += ' -o "' + modelsPath + '\\FbxPipeline\\' + sceneName + '.fbxp"';
                currentCommand += ' -l "' + modelsPath + '\\Logs\\' + sceneName + '.txt"';
                currentCommand += ' -e "' + sceneBaseDirectory + '/**"';
                currentCommand += ' --script-file glTFMaterialExtension.py';
                currentCommand += ' --script-input "' + glTFSceneFile + '"';
                commands.push(currentCommand);
                commandArgExamples.push('--assets "..\\..\\assets\\**" --scene "' + modelsPath + '\\FbxPipeline\\' + sceneName + '.fbxp"');

            } else {
                currentCommand += './FbxPipelineLauncher';
                currentCommand += ' -i "' + sceneSrcFile + '"';
                currentCommand += ' -o "' + modelsPath + '/FbxPipeline/' + sceneName + '.fbxp"';
                currentCommand += ' -l "' + modelsPath + '/Logs/' + sceneName + '.txt"';
                currentCommand += ' -e "' + sceneBaseDirectory + '/**"';
                currentCommand += ' --script-file glTFMaterialExtension.py';
                currentCommand += ' --script-input "' + glTFSceneFile + '"';
                commands.push(currentCommand);
                commandArgExamples.push('--assets /Users/vlad.serhiienko/Projects/Home/Viewer/assets/** --scene "' + modelsPath + '/FbxPipeline/' + sceneName + '.fbxp"');
            }

            var currentCommandQt = currentCommand;
            currentCommandQt += ' --tangent-frame-format quat-float';
            commandsQt.push(currentCommandQt);

            var currentCommandDrc = currentCommand;
            currentCommandDrc += ' --mesh-compression draco-edgebreaker';
            commandsDrc.push(currentCommandQt);

            var currentCommandDrcQt = currentCommandDrc;
            currentCommandDrcQt += ' --tangent-frame-format quat-float';
            commandsDrcQt.push(currentCommandQt);
        }
    }
});

var execExt = 'sh';

if (process.platform == 'win32') {
    execExt = 'bat';
}

fs.writeFileSync("UpdateScenes.gen." + execExt, commands.join('\n'));
fs.writeFileSync("UpdateScenesQt.gen." + execExt, commandsQt.join('\n'));
fs.writeFileSync("UpdateScenesDrc.gen." + execExt, commandsDrc.join('\n'));
fs.writeFileSync("UpdateScenesDrcQt.gen." + execExt, commandsDrcQt.join('\n'));
fs.writeFileSync("CommandArgExamples", commandArgExamples.join('\n'));
