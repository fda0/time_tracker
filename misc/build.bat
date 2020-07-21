@echo off
set AbsolutePath=C:\Dev\TimeTracker

pushd %AbsolutePath%\build

cl -Zi -W4 -nologo -wd4996 -wd4505 -wd4189 -wd4100 %AbsolutePath%\code\tt_main.cpp


popd