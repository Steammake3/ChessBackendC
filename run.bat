@echo off
title Chess Bot CLI
cd /d "%~dp0"

echo Enter arguments for the engine:
set /p args=

echo Starting engine...
echo -------------------

bot.exe %args%

echo -------------------
echo Program exited.
pause