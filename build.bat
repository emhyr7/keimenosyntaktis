@echo off
setlocal
cd /D "%~dp0"

set WFLAGS=-Wno-format-security -Wno-int-to-void-pointer-cast
set CFLAGS=-std=c11 -O0 -g %WFLAGS%

clang %CFLAGS% -o kmst.exe code/kmst.c
