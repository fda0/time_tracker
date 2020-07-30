@echo off
powershell "Measure-Command{misc\\build.bat | Out-Default} | findstr -i TotalMilliseconds"