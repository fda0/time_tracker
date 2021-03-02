mkdir build
pushd build
cl "..\code\main.cpp" /nologo /FC /GR- /Oi /WX /D Def_Windows=1 /D Def_Linux=0 /W4 /wd4100 /wd4189 /wd4201 /wd4505 /wd4996 /Ox /fp:fast /MT /link /INCREMENTAL:no /OPT:REF /RELEASE /OUT:"tt.exe"
popd
