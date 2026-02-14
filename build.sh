#!/usr/bin/env bash

compiler=g++
include=include
files="src/*.cpp"
output=build/main
flags="-O2"

echo "Starting Compilation with compiler $compiler compiling $files including $include outputting $output with flags $flags"

$compiler -I"$include" $files -o "$output" $flags
