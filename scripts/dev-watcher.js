#!/usr/bin/env node
/**
 * NovaCPP Hot Reloading & Fast Refresh Watch Runner
 * Watches source files, triggers incremental C++ builds or instant CSS injection,
 * and maintains continuous SSE communication with the browser.
 */

import fs from 'fs';
import path from 'path';
import http from 'http';
import { spawn, execSync } from 'child_process';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..');
const PORT = process.env.PORT || 8080;

let serverProcess = null;
let isRebuilding = false;
let buildPending = false;
let debounceTimer = null;

// ANSI Colors
const CYAN = '\x1b[36m';
const GREEN = '\x1b[32m';
const YELLOW = '\x1b[33m';
const RED = '\x1b[31m';
const RESET = '\x1b[0m';
const BOLD = '\x1b[1m';

function log(prefix, msg, color = CYAN) {
    console.log(`${color}${BOLD}[${prefix}]${RESET} ${msg}`);
}

// Kill any orphaned NovaCPP processes
function killOrphanedServers() {
    try {
        if (process.platform === 'win32') {
            execSync('taskkill /F /IM NovaCPP.exe /T 2>nul', { stdio: 'ignore' });
        } else {
            execSync('pkill -9 -f NovaCPP 2>/dev/null', { stdio: 'ignore' });
        }
    } catch (e) {}
}

// Send HMR event to the running NovaCPP server
function broadcastHMREvent(event, data = {}) {
    return new Promise((resolve) => {
        const payload = JSON.stringify(data);
        const req = http.request({
            hostname: '127.0.0.1',
            port: PORT,
            path: '/nova/hmr/broadcast',
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-Nova-HMR-Event': event,
                'Content-Length': Buffer.byteLength(payload)
            },
            timeout: 1000
        }, (res) => {
            resolve(res.statusCode === 200);
        });

        req.on('error', () => resolve(false));
        req.on('timeout', () => { req.destroy(); resolve(false); });
        req.write(payload);
        req.end();
    });
}

// Gracefully stop server and ensure port/file locks are released
function stopServer() {
    return new Promise((resolve) => {
        if (!serverProcess) {
            killOrphanedServers();
            return setTimeout(resolve, 150);
        }

        let resolved = false;
        const finish = () => {
            if (resolved) return;
            resolved = true;
            killOrphanedServers();
            serverProcess = null;
            setTimeout(resolve, 200);
        };

        const req = http.request({
            hostname: '127.0.0.1',
            port: PORT,
            path: '/nova/stop',
            method: 'POST',
            timeout: 500
        }, () => {
            setTimeout(finish, 100);
        });

        req.on('error', finish);
        req.on('timeout', () => { req.destroy(); finish(); });
        req.end();
    });
}

// Poll server until it is fully listening and responding
function waitForServerReady(port = PORT, maxWaitMs = 6000) {
    const start = Date.now();
    return new Promise((resolve) => {
        const check = () => {
            const req = http.request({
                hostname: '127.0.0.1',
                port: port,
                path: '/',
                method: 'GET',
                timeout: 300
            }, (res) => {
                if (res.statusCode >= 200 && res.statusCode < 500) {
                    resolve(true);
                } else {
                    retry();
                }
            });
            req.on('error', retry);
            req.on('timeout', () => { req.destroy(); retry(); });
            req.end();
        };

        const retry = () => {
            if (Date.now() - start > maxWaitMs) {
                resolve(false);
            } else {
                setTimeout(check, 100);
            }
        };

        check();
    });
}

// Spawn NovaCPP server binary
function startServer() {
    const isWindows = process.platform === 'win32';
    const exeName = isWindows ? 'NovaCPP.exe' : 'NovaCPP';
    
    // Check possible binary locations (VS Debug folder or root build folder)
    let binaryPath = path.join(ROOT_DIR, 'build', 'Debug', exeName);
    if (!fs.existsSync(binaryPath)) {
        binaryPath = path.join(ROOT_DIR, 'build', exeName);
    }

    if (!fs.existsSync(binaryPath)) {
        log('ERROR', `Binary not found at ${binaryPath}. Running initial build...`, RED);
        if (!runBuild()) return;
    }

    log('SERVER', `Launching ${path.basename(binaryPath)} on http://localhost:${PORT}`, GREEN);
    
    serverProcess = spawn(binaryPath, [], {
        cwd: ROOT_DIR,
        stdio: ['ignore', 'inherit', 'inherit']
    });

    serverProcess.on('exit', (code) => {
        if (!isRebuilding && code !== null && code !== 0) {
            log('SERVER', `Process exited with code ${code}`, YELLOW);
        }
    });
}

// Execute incremental C++ build
function runBuild() {
    log('BUILD', 'Compiling changes...', CYAN);
    const start = Date.now();
    try {
        const isWindows = process.platform === 'win32';
        const buildCmd = isWindows 
            ? 'cmake --build build --target NovaCPP -- /m:4 /verbosity:quiet'
            : 'cmake --build build --target NovaCPP -j4';

        execSync(buildCmd, { cwd: ROOT_DIR, stdio: 'inherit' });
        const elapsed = ((Date.now() - start) / 1000).toFixed(2);
        log('BUILD', `Compilation succeeded in ${elapsed}s`, GREEN);
        return true;
    } catch (err) {
        log('BUILD', 'Compilation failed. See compiler output above.', RED);
        return false;
    }
}

// Handle file change events
async function handleFileChange(filepath) {
    const ext = path.extname(filepath).toLowerCase();
    const basename = path.basename(filepath);

    // Ignore build output and temp files
    if (filepath.includes('build') || filepath.includes('.git') || filepath.includes('.vscode') || filepath.includes('node_modules')) {
        return;
    }

    // 0. npm package dependencies changed
    if (basename === 'package.json') {
        log('VENDOR', 'package.json changed -> Re-bundling vendor modules...', CYAN);
        try {
            execSync('node scripts/bundle-vendor.js', { cwd: ROOT_DIR, stdio: 'inherit' });
            await broadcastHMREvent('reload', { file: basename, timestamp: Date.now() });
        } catch (e) {
            log('VENDOR', 'Vendor bundle error: ' + e.message, RED);
        }
        return;
    }

    // 1. Instant CSS Hot Swap (Zero compilation, <15ms)
    if (ext === '.css') {
        log('HMR', `CSS changed (${basename}) -> Pushing hot stylesheet swap...`, GREEN);
        await broadcastHMREvent('style-update', { file: basename, timestamp: Date.now() });
        return;
    }

    // 2. Client JS changed
    if (ext === '.js' && filepath.includes('render')) {
        log('HMR', `Client script changed (${basename}) -> Pushing live reload...`, YELLOW);
        await broadcastHMREvent('reload', { file: basename });
        return;
    }

    // 3. C++ Source changed (Incremental compile + Component Fast Refresh)
    if (ext === '.cpp' || ext === '.hpp' || ext === '.h') {
        if (isRebuilding) {
            buildPending = true;
            return;
        }

        isRebuilding = true;
        log('FAST REFRESH', `Source changed (${basename}) -> Hot recompiling...`, CYAN);

        await stopServer();
        const success = runBuild();
        
        isRebuilding = false;

        if (success) {
            startServer();
            const ready = await waitForServerReady(PORT, 8000);
            if (ready) {
                log('HMR', `Fast Refresh ready. Broadcasting reload event...`, GREEN);
                await broadcastHMREvent('reload', { file: basename, timestamp: Date.now() });
            } else {
                log('SERVER', 'Server started but did not respond to ping within timeout.', YELLOW);
            }
        } else {
            log('HMR', 'Waiting for fixes before restarting...', YELLOW);
        }

        if (buildPending) {
            buildPending = false;
            handleFileChange(filepath);
        }
    }
}

// Watch directory recursively
function watchDirectory(dir) {
    if (!fs.existsSync(dir)) return;

    fs.watch(dir, { recursive: true }, (eventType, filename) => {
        if (!filename) return;
        const fullPath = path.join(dir, filename);

        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            handleFileChange(fullPath);
        }, 100);
    });
}

// Main execution
console.log(`\n${CYAN}${BOLD}========================================${RESET}`);
console.log(`${CYAN}${BOLD}   NovaCPP Hot Reloading Dev Runner   ${RESET}`);
console.log(`${CYAN}${BOLD}========================================${RESET}\n`);

// Clean up any stale orphaned NovaCPP processes before starting
killOrphanedServers();

// Ensure build directory exists
if (!fs.existsSync(path.join(ROOT_DIR, 'build', 'CMakeCache.txt'))) {
    log('INIT', 'Running initial CMake configuration...', CYAN);
    execSync('cmake -B build', { cwd: ROOT_DIR, stdio: 'inherit' });
}

// Pre-bundle any declared vendor packages before starting server
try {
    execSync('node scripts/bundle-vendor.js', { cwd: ROOT_DIR, stdio: 'inherit' });
} catch (e) {}

// Start initial build & server
if (runBuild()) {
    startServer();
}

// Watch src, render, novacpp folders and package.json
log('WATCH', 'Watching for changes in src/, render/, novacpp/, and package.json...', CYAN);
watchDirectory(path.join(ROOT_DIR, 'src'));
watchDirectory(path.join(ROOT_DIR, 'render'));
watchDirectory(path.join(ROOT_DIR, 'novacpp'));

// Watch root package.json
const rootPkg = path.join(ROOT_DIR, 'package.json');
if (fs.existsSync(rootPkg)) {
    fs.watch(rootPkg, () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            handleFileChange(rootPkg);
        }, 100);
    });
}

// Clean shutdown on Ctrl+C
process.on('SIGINT', async () => {
    log('WATCH', 'Shutting down dev server...', YELLOW);
    await stopServer();
    process.exit(0);
});

