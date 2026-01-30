@echo off
echo Building Weather Station for Windows...

mkdir build 2>nul
cd build

set QT_PATH=C:\msys64\mingw64
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_PATH%" ..

cmake --build .

echo Copying Qt DLLs...
copy "%QT_PATH%\bin\Qt6Core.dll" . 2>nul
copy "%QT_PATH%\bin\Qt6Gui.dll" . 2>nul
copy "%QT_PATH%\bin\Qt6Widgets.dll" . 2>nul
copy "%QT_PATH%\bin\Qt6Network.dll" . 2>nul
copy "%QT_PATH%\bin\Qt6Charts.dll" . 2>nul
copy "%QT_PATH%\bin\libgcc_s_seh-1.dll" . 2>nul
copy "%QT_PATH%\bin\libstdc++-6.dll" . 2>nul
copy "%QT_PATH%\bin\libwinpthread-1.dll" . 2>nul

mkdir platforms 2>nul
copy "%QT_PATH%\share\qt6\plugins\platforms\qwindows.dll" platforms\ 2>nul

cd ..
echo Build completed!