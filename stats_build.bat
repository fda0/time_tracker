@echo off
powershell "Measure-Command{./build.bat %1 | Out-Default} | findstr -i TotalMilliseconds"