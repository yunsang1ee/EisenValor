pushd %~dp0
@echo off

flatc --cpp Enums.fbs
flatc --cpp Structs.fbs
flatc --cpp Tables.fbs

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enums.fbs "../../../EisenValor-Server/Packets/Binaries"
XCOPY /Y Enums_generated.h "../../EisenValor-ClientFramework/include"

XCOPY /Y Structs.fbs "../../../EisenValor-Server/Packets/Binaries"
XCOPY /Y Structs_generated.h "../../EisenValor-ClientFramework/include"

XCOPY /Y Tables.fbs "../../../EisenValor-Server/Packets/Binaries"
XCOPY /Y Tables_generated.h "../../EisenValor-ClientFramework/include"