# Web-Based System Software Toolkit

This project wraps the original 8-module C System Software Toolkit in a browser-based interface.

## Requirements (Windows)
- GCC / MinGW-w64 available as `gcc` in PATH
- Node.js LTS available as `node` in PATH

No npm packages are required.

## Run
Double-click `run.bat`.

It compiles the C engine and starts the web server at:
http://localhost:3000

## Architecture
Browser (HTML/CSS/JS) -> Node.js API -> C engine -> module output -> browser

## Modules
1. Lexical Analyzer
2. Symbol Table
3. Two-Pass Assembler
4. Macro Processor
5. Linker & Loader
6. Recursive Descent Parser
7. Quadruple Generator
8. Code Optimizer

The original C source files are retained and are used by the web backend.
