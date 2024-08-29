#!/bin/sh

gcc -g \
-L./libs/raylib/lib/linux_amd64 -lraylib \
-I./libs/raylib/include \
src/main.c \
-o game.x86_64