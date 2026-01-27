@echo off

set compiler=g++
set include=include
set files=src/*.cpp include/bmp/image_io_bmp.cpp include/netpbm/image_io_pbm.cpp
set output=build/main.exe
set flags=-O2

echo Starting Compilation with compiler %compiler% compiling %files% including %include% outputting %output% with flags %flags%

%compiler% -I%include% %files% -o %output% %flags%

