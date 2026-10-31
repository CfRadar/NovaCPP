#!/usr/bin/env node
/**
 * NovaCPP npm Dependency Pre-Bundler
 * Scans package.json dependencies, pre-bundles them via esbuild into ESM browser modules,
 * and generates render/dist/importmap.json.
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..');
const DIST_DIR = path.join(ROOT_DIR, 'render', 'dist');
const VENDOR_DIR = path.join(DIST_DIR, 'vendor');
const IMPORTMAP_FILE = path.join(DIST_DIR, 'importmap.json');

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

export async function bundleVendorDependencies() {
    const pkgPath = path.join(ROOT_DIR, 'package.json');
    if (!fs.existsSync(pkgPath)) {
        return;
    }

    const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf-8'));
    const deps = Object.keys(pkg.dependencies || {});

    // Ensure output directories exist
    if (!fs.existsSync(DIST_DIR)) {
        fs.mkdirSync(DIST_DIR, { recursive: true });
    }
    if (!fs.existsSync(VENDOR_DIR)) {
        fs.mkdirSync(VENDOR_DIR, { recursive: true });
    }

    if (deps.length === 0) {
        fs.writeFileSync(IMPORTMAP_FILE, JSON.stringify({ imports: {} }, null, 2));
        return;
    }

    let esbuild;
    try {
        esbuild = await import('esbuild');
    } catch (e) {
        log('ERROR', 'esbuild is not installed. Run `npm install esbuild -D` to enable npm package bundling.', RED);
        return;
    }

    log('VENDOR', `Pre-bundling ${deps.length} npm dependenc${deps.length === 1 ? 'y' : 'ies'}...`, CYAN);
    const start = Date.now();
    const importMap = { imports: {} };

    for (const dep of deps) {
        const safeName = dep.replace(/\//g, '_') + '.js';
        const outFile = path.join(VENDOR_DIR, safeName);
        const webPath = `/dist/vendor/${safeName}`;

        try {
            await esbuild.build({
                entryPoints: [dep],
                bundle: true,
                format: 'esm',
                platform: 'browser',
                target: ['es2020'],
                minify: process.env.NODE_ENV === 'production',
                sourcemap: true,
                outfile: outFile,
                absWorkingDir: ROOT_DIR
            });

            importMap.imports[dep] = webPath;
            log('VENDOR', `  + ${dep} -> ${webPath}`, GREEN);
        } catch (err) {
            log('VENDOR', `  x Failed to bundle ${dep}: ${err.message}`, RED);
        }
    }

    fs.writeFileSync(IMPORTMAP_FILE, JSON.stringify(importMap, null, 2));
    const elapsed = Date.now() - start;
    log('VENDOR', `Pre-bundling completed in ${elapsed}ms`, GREEN);
}

// Run directly if invoked via CLI
if (process.argv[1] === __filename) {
    bundleVendorDependencies().catch((err) => {
        console.error(err);
        process.exit(1);
    });
}

