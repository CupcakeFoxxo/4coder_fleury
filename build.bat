@echo off
call ..\bin\buildsuper_x64-win.bat .\4coder_fleury.cpp release
copy .\custom_4coder.dll ..\..\custom_4coder.dll
copy .\custom_4coder.pdb ..\..\custom_4coder.pdb
copy .\bindings.4coder ..\..\bindings.4coder
copy .\config.4coder ..\..\config.4coder
copy .\theme-cupcake.4coder ..\..\themes\theme-cupcake.4coder