@echo off

set BuildPath=build
IF NOT EXIST %BuildPath% mkdir %BuildPath%
pushd %BuildPath%

rem Delete pdb file spam
del *.pdb

set WinSourcePath=..\code\tt_main.cpp


rem ~~~~~~~~~~RELEASE MODE~~~~~~~~~~
set Release=/O2 /fp:fast

rem ~~~~~~~~~~DEBUG MODE~~~~~~~~~~
set Debug=/Od

rem ~~~~~~~~~~SELECT MODE~~~~~~~~~~
set Mode=%Debug%
if "%1" == "release" (set Mode=%release% 
echo ===_===_=== OPTIMIZED MODE ===_===_===) else (echo === debug mode ===)



rem ~~~~~~~~~~WARININGS~~~~~~~~~~
set opts=/WX /W4 /wd4100 /wd4201 /wd4189 /wd4505 /wd4101 /wd4996

rem ~~~~~~~~~~FLAGS~~~~~~~~~~
set opts=%opts% /nologo /GR- /FC
set opts=%opts% /MTd /EHa- /Oi /Zi
set opts=%opts% /DBUILD_INTERNAL=1
set opts=%opts% /DOS_WINDOWS=1

set link_opts=/link /incremental:no /opt:ref
set link_opts=%link_opts% shell32.lib


rem ~~~~~~~~~~BUILD x64~~~~~~~~~~

cl %opts% %Mode% %WinSourcePath% %link_opts%


popd



