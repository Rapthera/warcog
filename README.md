#community
irc.warcog.org

#git
git://warcog.org/warcog
https://github.com/warcog/warcog

#license
all files are GPLv3, with some exceptions:
1. ./gameclient/xz/*: public domain (http://tukaani.org/xz/embedded.html)
2. ./gameclient/audio/hrtf.c: LGPLv2+ (http://kcat.strangesoft.net/openal.html)
3. ./gameclient/text/freetype.c: FTL/GPLv2 (http://www.freetype.org/license.html)

#notes
use latest MESA
will need to reorganize code at some point

#build client
gcc -o ./bin/game *.c ./audio/*.c ./text/*.c ./xz/*.c ./xlib/*.c -DNAME=\"warcog\" -DXLIB -Wall -pthread -lm -lGL -lX11 -lasound -Ofast -flto -s

#cross compile client
x86_64-w64-mingw32-gcc -o ./bin/game.exe *.c ./audio/*.c ./win32/*.c ./xz/*.c ./text/*.c -DGAME_FPS=60 -DNAME=\"warcog\" -DWIN32 -Wall -lopengl32 -lws2_32 -lgdi32 -lwinmm -lole32 -Ofast -flto -s
