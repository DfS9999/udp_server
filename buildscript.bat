@echo off

gcc udp_implementation.c -Wall -Wextra -lwsock32 -o udp_server.exe

if %errorlevel% equ 0 (
    echo build successful
) else (
    echo build failed 
)
