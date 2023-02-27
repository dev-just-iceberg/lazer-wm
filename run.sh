#!/usr/bin/env sh

Xephyr -ac -br -reset -screen 1920x1080 :7 &

sleep 1s

export DISPLAY=:7

./src/bin/lazer-wm
