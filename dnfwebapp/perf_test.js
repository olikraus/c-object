const fs = require('fs');
const addonPath = process.env.DWA_NODE_PATH || './build/Release/dwa_node.node';
const dwaNode = require(addonPath);

try {
    const inputPath = 'perf_input.json';
    if (!fs.existsSync(inputPath)) {
        console.error("perf_input.json not found. Run 'make perf_input.json' first.");
        process.exit(1);
    }

    console.log("Reading perf_input.json...");
    const inputData = fs.readFileSync(inputPath, 'utf8');

    console.log("Executing DWA via Node.js native addon...");
    const start = Date.now();
    const resStr = dwaNode.execute(inputData);
    const end = Date.now();

    console.log(`Total Time: ${(end - start) / 1000.0}s`);
    const resObj = JSON.parse(resStr);
    if (resObj.log) {
        console.log("Internal Log:\n" + resObj.log);
    }
} catch (err) {
    console.error("Performance test error:", err);
    process.exit(1);
}
