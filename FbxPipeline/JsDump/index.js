var fs = require('fs');
var flatbuffers = require('../../ThirdParty/flatbuffers/js/flatbuffers').flatbuffers;
var fbxp = require('../generated/v140x64Release/scene_generated.js').fbxp;

console.log(flatbuffers);
console.log(fbxp);

var serializedFileName = "";
if (process.argv.length >= 3) {
    console.log('Dumping ', process.argv[2]);
    var data = new Uint8Array(fs.readFileSync(process.argv[2]));
    if (data) {
        var buf = new flatbuffers.ByteBuffer(data);
        var scene = fbxp.fb.SceneFb.getRootAsSceneFb(buf);
        for (var i = 0; i < scene.namesLength(); ++i) {
            var name = scene.names(i);
            console.log(name.h(), ' -> ', name.v());
        }
    } else {
        console.log('File is inaccessible.');
    }
} else {
    console.log('Usage:   node index.js [FBXP file path]');
    console.log('Example: node index.js "../serializedFbxFile.fbxp"');
}