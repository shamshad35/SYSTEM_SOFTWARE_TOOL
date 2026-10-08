const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const PORT = Number(process.env.PORT || 3000);
const ROOT = __dirname;
const PUBLIC = path.join(ROOT, 'public');
const exe = process.platform === 'win32' ? path.join(ROOT, 'toolkit_web.exe') : path.join(ROOT, 'toolkit_web');

const modules = {
  lexical: { title: 'Lexical Analyzer', description: 'Convert source code into tokens and identify keywords, identifiers, operators and separators.', example: 'int total = price + 10;' },
  symbol: { title: 'Symbol Table', description: 'Build a symbol table with datatype, size and memory address.', example: 'int x y float total char grade' },
  assembler: { title: 'Two-Pass Assembler', description: 'Generate symbol table, intermediate code and object code from assembly source.', example: 'START 100\nMOVER AREG, X\nADD BREG, Y\nX DC 5\nY DS 1\nEND' },
  macro: { title: 'Macro Processor', description: 'Define macros and expand macro calls into their resulting program.', example: 'MACRO\nINCR &X\nADD AREG, &X\nMEND\nSTART\nINCR NUM\nEND' },
  linker: { title: 'Linker & Loader', description: 'Relocate module symbols and generate an absolute symbol table.', example: 'MAIN X=0 Y=20\nUTIL Z=5\nEND' },
  parser: { title: 'Recursive Descent Parser', description: 'Validate arithmetic expressions using recursive-descent grammar.', example: 'a + b * (c - 2)' },
  quadruple: { title: 'Quadruple Generator', description: 'Generate intermediate quadruple representation from an arithmetic expression.', example: 'a + b * c - d' },
  optimizer: { title: 'Code Optimizer', description: 'Apply simple constant folding and algebraic optimizations to three-address code.', example: 'a = 10 + 20\nb = a + 0\nc = b * 1\nd = c * 0' }
};

function send(res, code, body, type='application/json') {
  res.writeHead(code, {'Content-Type': type, 'Cache-Control':'no-store'}); res.end(body);
}
function json(res, code, obj) { send(res, code, JSON.stringify(obj), 'application/json'); }
function safeFile(p) { return path.normalize(p).startsWith(path.normalize(PUBLIC + path.sep)); }
function runC(mode, input) {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(exe)) return reject(new Error('C engine not built. Run run.bat first.'));
    const child = spawn(exe, [mode], { cwd: ROOT, windowsHide: true });
    let out='', err='';
    child.stdout.on('data', d => out += d); child.stderr.on('data', d => err += d);
    child.on('error', reject);
    child.on('close', code => code === 0 ? resolve(out) : reject(new Error(err || `C engine exited with code ${code}`)));
    child.stdin.end(String(input || ''));
  });
}

const server = http.createServer(async (req, res) => {
  try {
    if (req.method === 'GET' && req.url === '/api/modules') return json(res,200,modules);
    if (req.method === 'POST' && req.url.startsWith('/api/run/')) {
      const mode = req.url.split('/').pop();
      if (!modules[mode]) return json(res,404,{error:'Unknown module'});
      let body=''; req.on('data', c => { body += c; if (body.length > 200000) req.destroy(); });
      req.on('end', async () => {
        try {
          const data = JSON.parse(body || '{}');
          const output = await runC(mode, data.input || '');
          json(res,200,{mode, output});
        } catch(e) { json(res,500,{error:e.message}); }
      });
      return;
    }
    if (req.method === 'GET') {
      let pathname = decodeURIComponent(req.url.split('?')[0]);
      if (pathname === '/') pathname = '/index.html';
      const file = path.join(PUBLIC, pathname);
      if (!safeFile(file) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) return send(res,404,'Not found','text/plain');
      const ext = path.extname(file); const types={'.html':'text/html','.css':'text/css','.js':'application/javascript','.json':'application/json','.svg':'image/svg+xml'};
      return send(res,200,fs.readFileSync(file),types[ext]||'application/octet-stream');
    }
    send(res,405,'Method not allowed','text/plain');
  } catch(e) { send(res,500,e.message,'text/plain'); }
});
server.listen(PORT, () => console.log(`System Software Toolkit running at http://localhost:${PORT}`));
