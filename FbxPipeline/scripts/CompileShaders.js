const fs = require('fs');
const path = require('path');
const exec = require('child_process').exec;
const spawn = require('child_process').spawnSync;

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

var shaderc = removeFile(process.argv[1]);
var shaderDir = shaderc + '/shaders';
if (process.argv.length >= 3)
    shaderDir = process.argv[2];

var rutimeDir = shaderDir + '/runtime/';
var rutimeIncludeDir = shaderDir + '/include/';

if (!fs.existsSync(rutimeDir)){
    fs.mkdirSync(rutimeDir);
}

if (!fs.existsSync(rutimeIncludeDir)){
    fs.mkdirSync(rutimeIncludeDir);
}

var platformProfiles = ['windows', 'android', 'osx', 'ios'];
var commandTemplate = 'shaderc -f {{shaderFilePath}} -o {{binFilePath}} {{bin2c}} ' +
                      '-i ../shaders/shared -i ../../ThirdParty/bgfx/src --verbose ' +
                      '--type {{shaderType}} --platform {{platformProfile}}';

fs.readdir(shaderDir, (err, files) => {
    files.forEach(fileName => {
         if (path.extname(fileName) == ".sc") {
            var shaderType = null;
            var shaderFilePref = fileName.substring(0, 3);
            if (shaderFilePref == "fs_") {
                shaderType = "fragment";
            } else if (shaderFilePref == "vs_") {
                shaderType = "vertex";
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
                    var binFilePath = rutimeDir + removeExtension(fileName) + '_' + value + '.bin';
                    var incFilePath = rutimeIncludeDir + removeExtension(fileName) + '_' + value + ".bin.h";

                    var binCmd = binCmdTemplate.replace('{{binFilePath}}', binFilePath).replace('{{platformProfile}}', value);
                    var incCmd = incCmdTemplate.replace('{{binFilePath}}', incFilePath).replace('{{platformProfile}}', value);

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