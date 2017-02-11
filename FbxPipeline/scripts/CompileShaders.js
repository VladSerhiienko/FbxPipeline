const fs = require('fs');
const path = require('path');
const exec = require('child_process').exec;
const spawn = require('child_process').spawnSync;

function removeExtension(filename) {
    var lastDotPosition = filename.lastIndexOf(".");
    if (lastDotPosition === -1) return filename;
    else return filename.substr(0, lastDotPosition);
}

var shaderc = process.argv[2];
var shaderDir = process.argv[3];
var outputDir = process.argv[4];
var includeDir = process.argv[5];
var varyingDef = process.argv[6];

//console.log('Current folder:', currentDir);
fs.readdir(shaderDir, (err, files) => {
    files.forEach(fileName => {
        if (path.extname(fileName) == ".sc") {

            const fileBaseName = removeExtension(fileName);
            const filePath = shaderDir + path.basename(fileName);
            const filePathBin = outputDir + fileBaseName + '.bin';
            const filePathBin2c = outputDir + fileBaseName + '.bin.h';

            console.log(' <- ', fileName);
            console.log(' -> ', filePath);
            console.log(' -> ', fileBaseName);
            console.log(' -> ', filePathBin2c);

            var shaderType = null;

            if (fileName.substring(0, 3) == "fs_") {
                shaderType = "fragment";

            } else if (fileName.substring(0, 3) == "vs_") {
                shaderType = "vertex";
            }

            if (shaderType != null) {
                var shadercCmdBin
                    = shaderc + 
                    //'/../../ThirdParty/bgfx/.build/win64_vs2015/bin/shadercRelease' +
                    ' -f ' +
                    filePath +
                    ' -o ' +
                    filePathBin +
                    ' -i ' +
                    includeDir +
                    //process.cwd() +
                    //'/../../ThirdParty/bgfx/src' +
                    ' --type ' +
                    shaderType +
                    ' --platform windows' +
                    ' --verbose ' +
                    ' --varyingdef ' + varyingDef;

                var shadercCmdBin2c
                    = shaderc + 
                    //'/../../ThirdParty/bgfx/.build/win64_vs2015/bin/shadercRelease' +
                    ' -f ' +
                    filePath +
                    ' -o ' +
                    filePathBin2c +
                    ' --bin2c ' +
                    fileBaseName +
                    ' -i ' +
                    includeDir +
                    //process.cwd() +
                    //'/../../ThirdParty/bgfx/src' +
                    ' --type ' +
                    shaderType + 
                    ' --platform windows' +
                    ' --verbose' +
                    ' --varyingdef ' + varyingDef;

                console.log(shadercCmdBin);
                console.log(shadercCmdBin2c);

                var scBin = exec(shadercCmdBin, function(err, stdout, stderr) {
                    //console.log(' bin (err) -> ', err);
                    //console.log(' bin (stderr) -> ', stderr);
                    //console.log(' bin (stdout) -> ', stdout);
                });

                scBin.stdout.pipe(process.stdout);
                scBin.stderr.pipe(process.stderr);

                var scBin2c = exec(shadercCmdBin2c, function(err, stdout, stderr) {
                    //console.log(' bin2c (err) -> ', err);
                    //console.log(' bin2c (stderr) -> ', stderr);
                    //console.log(' bin2c (stdout) -> ', stdout);
                });

                scBin2c.stdout.pipe(process.stdout);
                scBin2c.stderr.pipe(process.stderr);
            }
        }
    });
})