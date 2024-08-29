@echo off
gcc -g ^
-L./libs/raylib/lib/win_mingw64 -lraylib ^
-I./libs/raylib/include ^
src/main.c ^
-o game.exe