@echo off
setlocal
cd /d "%~dp0"
echo ================================================
echo   SYSTEM SOFTWARE TOOLKIT - WEB VERSION
echo ================================================
where gcc >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERROR: GCC was not found in PATH.
  echo Install MinGW-w64 / GCC and reopen this terminal.
  pause
  exit /b 1
)
echo Building C engine...
gcc -O2 -std=c99 -Wall -Wextra -o toolkit_web.exe web_runner.c util.c lexical_analyzer.c symbol_table.c assembler.c macro_processor.c linker_loader.c recursive_descent_parser.c quadruple.c optimizer.c
if errorlevel 1 (
  echo.
  echo Build failed. Check the compiler messages above.
  pause
  exit /b 1
)
where node >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERROR: Node.js was not found in PATH.
  echo Install Node.js LTS and reopen this terminal.
  pause
  exit /b 1
)
echo Starting website...
echo Open http://localhost:3000 in your browser.
echo Press Ctrl+C in this window to stop the server.
node server.js
endlocal
