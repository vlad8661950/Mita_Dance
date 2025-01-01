# Mita Dance
![Mita](png/miside00.png)

The program is based on Konata Dance but uses SDL2. It has some rendering issues.


Compilation command with static SDL2 libraries:

`g++ -o Mita.exe Mita.cpp -ISDL2 -Ipng -LSDL2/lib -lmingw32 -lSDL2 -lSDL2_image -lsetupapi -lole32 -lgdi32 -limm32 -lwinmm -loleaut32 -lversion -static-libgcc -static -static-libstdc++ -mwindows -Os`

SDL2_VS - dynamic libraries for Visual Studio.

PNG to Header Conversion:

<<<<<<< HEAD
`xxd -i miside00.png > miside00.h`
=======
`xxd -i miside00.png > miside00.h`
>>>>>>> 5d0c69af54a7bd2594974ff060f66bb115a8536a
