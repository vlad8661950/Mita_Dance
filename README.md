# Mita Dance
The program is based on Konata Dance but uses SDL2. It has some rendering issues.



Compile with static SDL2 libraries:

g++ -o Mita.exe Mita.cpp -ISDL2 -Ipng -LSDL2/lib -lmingw32 -lSDL2 -lSDL2_image -lsetupapi -lole32 -lgdi32 -limm32 -lwinmm -loleaut32 -lversion -static-libgcc -static -static-libstdc++ -mwindows -Os

SDL2_VS - dunamic libraries for Visual studio

Use xxd -i miside00.png > miside00.h to transorm png in headers
