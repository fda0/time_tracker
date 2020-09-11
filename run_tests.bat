@echo off

set BuildPath=tests
IF NOT EXIST %BuildPath% mkdir %BuildPath%
pushd %BuildPath%


set WinSourcePath=..\code\tests.cpp


rem ~~~~~~~~~~WARININGS~~~~~~~~~~
set opts=/WX /W4 /wd4100 /wd4201 /wd4189 /wd4505 /wd4101 /wd4996

rem ~~~~~~~~~~FLAGS~~~~~~~~~~
set opts=%opts% /nologo /GR- /FC

set link_opts=/link /incremental:no /opt:ref


rem ~~~~~~~~~~BUILD x64~~~~~~~~~~

cl %opts% %Mode% %WinSourcePath% %link_opts%


tests.exe
del tests.exe
del tests.obj
del *.pdb

popd



