astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --recursive *.h
astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --recursive *.cpp
astyle.exe -v --formatted --options=astyle-options.ini --recursive ../doomclassic/*.h
astyle.exe -v --formatted --options=astyle-options.ini --recursive ../doomclassic/*.cpp

pause