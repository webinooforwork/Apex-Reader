#!/data/data/com.termux/files/usr/bin/sh
set -eu
pkg update
pkg install -y cmake ninja clang pkg-config libzip qt6-qtbase
