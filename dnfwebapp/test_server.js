const http = require('http');
const fs = require('fs');
const path = require('path');
const server = require('./server');

const TEST_PORT = 8081;

server.listen(TEST_PORT, async () => {
    console.log(`Test server started on port ${TEST_PORT}`);
    try {
        const inputPath = path.join(__dirname, '../test/dwa_test_input.json');
        const expectedPath = path.join(__dirname, '../test/dwa_expected_output.json');

        const inputData = fs.readFileSync(inputPath, 'utf8');
        const expectedData = JSON.parse(fs.readFileSync(expectedPath, 'utf8'));

        console.log("Sending test request to server...");
        const response = await fetch(`http://localhost:${TEST_PORT}/execute`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: inputData
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const resultJson = await response.json();

        const filteredResult = resultJson.result;
        const filteredExpected = expectedData.result;

        const resStr = JSON.stringify(filteredResult);
        const expStr = JSON.stringify(filteredExpected);

        if (resStr === expStr) {
            console.log("TEST PASSED: Node.js addon and server execution matches expected output!");
            process.exit(0);
        } else {
            console.error("TEST FAILED: Result does not match expected output.");
            console.error("Got:", resStr.substring(0, 200) + "...");
            console.error("Expected:", expStr.substring(0, 200) + "...");
            process.exit(1);
        }
    } catch (err) {
        console.error("TEST ERROR:", err);
        process.exit(1);
    } finally {
        server.close();
    }
});
