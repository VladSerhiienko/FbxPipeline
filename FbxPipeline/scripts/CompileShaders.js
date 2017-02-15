
//
// A script for shader compilation using shaderc tool by bgfx.
// Generates all possible shaders
// For include it uses ../shaders/shared and bgfx/src folders.
// All the output goes to assets/shaders/include and runtime folders.
// Output folders will be created automatically if needed.
//

const fs = require('fs');
const path = require('path');
const exec = require('child_process').exec;

function removeExtension(filename) {
    var lastDotPosition = filename.lastIndexOf(".");
    if (lastDotPosition === -1) return filename;
    else return filename.substr(0, lastDotPosition);
}

function removeFile(filename) {
    var lastDotPosition = filename.lastIndexOf("/");
    if (lastDotPosition === -1) lastDotPosition = filename.lastIndexOf("\\");
    if (lastDotPosition === -1) return filename;
    else return filename.substr(0, lastDotPosition);
}

function createFolderIfNeeded(foldername) {
    if (false === fs.existsSync(foldername)){
        fs.mkdirSync(foldername);
    }
}

var shaderc = removeFile(process.argv[1]);
var shaderDir = shaderc + '/shaders';
if (process.argv.length >= 3)
    shaderDir = process.argv[2];

var rutimeDir = shaderc + '/../assets/shaders/runtime/';
var rutimeIncludeDir = shaderc + '/../assets/shaders/include/';

createFolderIfNeeded(shaderc + '/../assets/shaders/');
createFolderIfNeeded(shaderc + '/../assets/shaders/runtime/');
createFolderIfNeeded(shaderc + '/../assets/shaders/include/');

var platformProfiles = ['windows -p {{hlsl}}s_3_0 -O 3',
                        'windows -p {{hlsl}}s_5_0 -O 3',
                        'nacl',
                        'android',
                        'linux -p 120',
                        'linux -p spirv',
                        'osx -p metal',
                        'asm.js'];

var commandTemplate = 'shaderc -f {{shaderFilePath}} -o {{binFilePath}} {{bin2c}} ' +
                      '-i ../shaders/shared -i ../../ThirdParty/bgfx/src --verbose ' +
                      '--type {{shaderType}} --platform {{platformProfile}}';

fs.readdir(shaderDir, (err, files) => {
    files.forEach(fileName => {
         if (path.extname(fileName) == ".sc") {
            var shaderType = null;
            var hlslShaderType = null;
            var shaderFilePref = fileName.substring(0, 3);

            if (shaderFilePref == "fs_") {
                shaderType = "fragment";
                hlslShaderType = "p";
            } else if (shaderFilePref == "vs_") {
                shaderType = "vertex";
                hlslShaderType = "v";
            } else if (shaderFilePref == "cs_") {
                shaderType = "compute";
                hlslShaderType = "c";
            }

            if (shaderType != null) {
                var shaderFilePath = shaderDir + '/' + fileName;

                var binCmdTemplate = commandTemplate.replace('{{shaderFilePath}}', shaderFilePath);
                binCmdTemplate = binCmdTemplate.replace('{{bin2c}}', '');
                binCmdTemplate = binCmdTemplate.replace('{{shaderType}}', shaderType);

                var incCmdTemplate = commandTemplate.replace('{{shaderFilePath}}', shaderFilePath);
                incCmdTemplate = incCmdTemplate.replace('{{bin2c}}', '--bin2c');
                incCmdTemplate = incCmdTemplate.replace('{{shaderType}}', shaderType);

                console.log('Bin (T): ', binCmdTemplate);
                console.log('Inc (T): ', incCmdTemplate);

                platformProfiles.forEach((value, index, array) => {
                    var extensionValue = value.replace('.', '_');
                        extensionValue = extensionValue.replace(/ /g, '_');
                        extensionValue = extensionValue.replace(/-/g, '_');
                        extensionValue = extensionValue.replace('{{hlsl}}', hlslShaderType);
                    var platformProfile = value.replace('{{hlsl}}', hlslShaderType);

                    var binFilePath = rutimeDir + removeExtension(fileName) + '_' + extensionValue + '.bin';
                    var incFilePath = rutimeIncludeDir + removeExtension(fileName) + '_' + extensionValue + ".bin.h";

                    var binCmd = binCmdTemplate.replace('{{binFilePath}}', binFilePath).replace('{{platformProfile}}', platformProfile);
                    var incCmd = incCmdTemplate.replace('{{binFilePath}}', incFilePath).replace('{{platformProfile}}', platformProfile);

                    console.log('Bin: ', binCmd);
                    console.log('Inc: ', incCmd);

                    var binOutput = exec(binCmd);
                    binOutput.stdout.pipe(process.stdout);
                    binOutput.stderr.pipe(process.stderr);

                    var incOutput = exec(incCmd);
                    incOutput.stdout.pipe(process.stdout);
                    incOutput.stderr.pipe(process.stderr);
                });
            }
        }
    });
});