pushd %~dp0
@echo off

XCOPY ..\ServerEngine\*.h ..\Libraries\inc\ServerEngine\ /S /I /Y

IF ERRORLEVEL 1 PAUSE

PAUSE