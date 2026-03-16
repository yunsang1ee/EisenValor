pushd %~dp0
@echo off

flatc --cpp Enums.fbs
flatc --cpp Structs.fbs
flatc --cpp Tables.fbs

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enums.fbs "../../../EisenValor-Client/Packets/Binaries"
XCOPY /Y Enums.fbs "../../../EisenValor-DummyClient/Packets/Binaries"
XCOPY /Y Enums.fbs "../../../EisenValor-LobbyServer/Packets/Binaries"
XCOPY /Y Enums_generated.h "../../Server"

XCOPY /Y Structs.fbs "../../../EisenValor-Client/Packets/Binaries"
XCOPY /Y Structs.fbs "../../../EisenValor-DummyClient/Packets/Binaries"
XCOPY /Y Structs.fbs "../../../EisenValor-LobbyServer/Packets/Binaries"
XCOPY /Y Structs_generated.h "../../Server"

XCOPY /Y Tables.fbs "../../../EisenValor-Client/Packets/Binaries"
XCOPY /Y Tables.fbs "../../../EisenValor-DummyClient/Packets/Binaries"
XCOPY /Y Tables.fbs "../../../EisenValor-LobbyServer/Packets/Binaries"
XCOPY /Y Tables_generated.h "../../Server"