#!/usr/bin/env bash
mkdir build_linux 2>/dev/null
pushd build_linux >/dev/null

vars="-DDef_Linux=1 -DDef_Slow=1 -DDef_Internal=1 -fdiagnostics-absolute-paths -Wno-writable-strings -fno-exceptions -g"


clang ../code/main.cpp $vars -pthread -o tt.out

popd >/dev/null