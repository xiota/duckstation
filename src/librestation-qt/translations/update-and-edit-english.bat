@echo off

set "linguist=..\..\..\dep\msvc\deps-x64\bin"
set context=../ ../../core/ ../../util/ -tr-function-alias QT_TRANSLATE_NOOP+=TRANSLATE,QT_TRANSLATE_NOOP+=TRANSLATE_SV,QT_TRANSLATE_NOOP+=TRANSLATE_STR,QT_TRANSLATE_NOOP+=TRANSLATE_FS,QT_TRANSLATE_N_NOOP3+=TRANSLATE_FMT,QT_TRANSLATE_NOOP+=TRANSLATE_NOOP,translate+=TRANSLATE_PLURAL_STR,translate+=TRANSLATE_PLURAL_SSTR,translate+=TRANSLATE_PLURAL_FS -pluralonly -no-obsolete

"%linguist%\lupdate.exe" %context% -ts librestation-qt_en.ts
pause

cd "%linguist%"
start /B linguist.exe "%~dp0\librestation-qt_en.ts"
