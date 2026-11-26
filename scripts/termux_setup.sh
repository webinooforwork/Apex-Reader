#!/data/data/com.termux/files/usr/bin/sh
set -eu
pkg update
pkg install -y cmake ninja clang pkg-config libzip sqlite qt6-qtbase
