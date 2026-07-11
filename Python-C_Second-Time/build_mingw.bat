@echo off
setlocal

if not exist build mkdir build

set CFLAGS=-std=c99 -Wall -Wextra -Iinclude
set SRC=src\rs485_transport_win32.c src\pedal_platform_win32.c src\servo_axis.c src\pedal_controller.c src\ids830abs_utils.c

echo Building retract_dual_pedals.exe...
gcc %CFLAGS% %SRC% examples\retract_dual_pedals.c -o build\retract_dual_pedals.exe
if errorlevel 1 exit /b 1

echo Building read_status.exe...
gcc %CFLAGS% %SRC% examples\read_status.c -o build\read_status.exe
if errorlevel 1 exit /b 1

echo Building emergency_stop.exe...
gcc %CFLAGS% %SRC% examples\emergency_stop.c -o build\emergency_stop.exe
if errorlevel 1 exit /b 1

echo Done.
