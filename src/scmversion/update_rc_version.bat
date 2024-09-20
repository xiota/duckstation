@echo off

FOR /F "tokens=* USEBACKQ" %%g IN (`git describe --dirty`) do (SET "TAG=%%g")

powershell -Command "(gc ..\librestation-qt\librestation-qt.rc) -replace '1,0,0,1', '"%TAG:~0,1%","%TAG:~2,1%","%TAG:~4,4%",0' | Out-File -encoding ASCII ..\librestation-qt\librestation-qt.rc"
powershell -Command "(gc ..\librestation-qt\librestation-qt.rc) -replace '1.0.0.1', '"%TAG:~0,1%"."%TAG:~2,1%"."%TAG:~4,4%"' | Out-File -encoding ASCII ..\librestation-qt\librestation-qt.rc"
