const http = require('http');

let dwaNode;
try {
    const addonPath = process.env.DWA_NODE_PATH || './build/Release/dwa_node.node';
    dwaNode = require(addonPath);
} catch (e) {
    console.error("Failed to load native addon from " + (process.env.DWA_NODE_PATH || './build/Release/dwa_node.node') + ". Please build it first using 'make node'.");
    process.exit(1);
}

const PORT = process.env.PORT || 8080;

const server = http.createServer((req, res) => {
    if (req.method === 'POST' && (req.url === '/' || req.url === '/execute')) {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        req.on('end', () => {
            try {
                const resultJson = dwaNode.execute(body);
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(resultJson);
            } catch (err) {
                res.writeHead(500, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: err.message }));
            }
        });
    } else {
        res.writeHead(404, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Not Found. Use POST /execute with DNF/JSON input.' }));
    }
});

if (require.main === module) {
    server.listen(PORT, () => {
        console.log(`DNF WebApp Node.js server running on port ${PORT}`);
    });
}

module.exports = server;
