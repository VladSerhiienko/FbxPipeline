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
if (process.platform == 'win32') {
    commands.push('cd ./build_windows_amd64_msvc/Release/');
} else {
    commands.push('cd build_darwin_x86_64_appleclang/Release');
}

console.log(sceneSrcFiles);
sceneSrcFiles.forEach(function (sceneSrcFile) {
    var sceneSrcDirectory = path.dirname(sceneSrcFile);
    var sceneSrcDirectoryName = path.basename(sceneSrcDirectory);

    if (sceneSrcDirectoryName === 'source') {
        var sceneBaseDirectory = path.dirname(sceneSrcDirectory);
        var glTFSceneFile = sceneBaseDirectory + '\\scene.gltf';
        if (fs.existsSync(glTFSceneFile)) {
            var sceneName = path.parse(sceneSrcFile).name;

            // FbxPipelineLauncher.exe -i "C:/Sources/Models/bristleback-dota-fan-art/source/bristleback-dota-fan-art.fbx"
            // -o "C:/Sources/Models/FbxPipeline/bristleback-dota-fan-art-cubic.fbxp" --resample-framerate 0
            // -l "C:/Sources/Models/Logs/bristleback-dota-fan-art-cubic.txt" -e "C:/Sources/Models/bristleback-dota-fan-art/**"
            // --script-file "glTFMaterialExtension.py" --script-input "C:/Sources/Models/bristleback-dota-fan-art/scene.gltf"

            if (process.platform == 'win32') {
                var currentCommandPS = '';
                currentCommandPS += 'FbxPipelineLauncher.exe';
                currentCommandPS += ' -i "' + sceneSrcFile + '"';
                currentCommandPS += ' -o "' + modelsPath + '\\FbxPipeline\\' + sceneName + '.fbxp"';
                currentCommandPS += ' -l "' + modelsPath + '\\Logs\\' + sceneName + '.txt"';
                currentCommandPS += ' -e "' + sceneBaseDirectory + '/**"';
                currentCommandPS += ' --script-file glTFMaterialExtension.py';
                currentCommandPS += ' --script-input "' + glTFSceneFile + '"';
                commands.push(currentCommandPS);
            } else {
                var currentCommandSH = '';
                currentCommandSH += './FbxPipelineLauncher';
                currentCommandSH += ' -i "' + sceneSrcFile + '"';
                currentCommandSH += ' -o "' + modelsPath + '/FbxPipeline/' + sceneName + '.fbxp"';
                currentCommandSH += ' -l "' + modelsPath + '/Logs/' + sceneName + '.txt"';
                currentCommandSH += ' -e "' + sceneBaseDirectory + '/**"';
                currentCommandSH += ' --script-file glTFMaterialExtension.py';
                currentCommandSH += ' --script-input "' + glTFSceneFile + '"';
                commands.push(currentCommandSH);
            }
        }

        // console.log(sceneBaseDirectory);
    }
});

console.log(commands);
if (process.platform == 'win32') {
    fs.writeFileSync("UpdateScenes.gen.bat", commands.join('\n'));
} else {
    fs.writeFileSync("UpdateScenes.gen.sh", commands.join('\n'));
}
