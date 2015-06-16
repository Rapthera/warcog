#Community

We've got a IRC server were most of the developers hang out over at irc.warcog.org.

#License

All of our code is licensed under GPLv3 with a few exceptions:

* [/gameclient/xz/*](http://tukaani.org/xz/embedded.html) is licensed under public domain.
* [/gameclient/audio/hrtf.c](http://kcat.strangesoft.net/openal.html) is licensed under LGPLv2+.
* [/gameclient/text/freetype.c](http://www.freetype.org/license.html) is licensed under FTL/GPLv2.


#Notes
Use latest MESA.

Code re-organization needed at some point.

#Build Client
`gcc -o ./bin/game *.c ./audio/*.c ./text/*.c ./xz/*.c ./xlib/*.c -DNAME=\"warcog\" -DXLIB -Wall -pthread -lm -lGL -lX11 -lasound -Ofast -flto -s`

#Cross-compile Client
`x86_64-w64-mingw32-gcc -o ./bin/game.exe *.c ./audio/*.c ./win32/*.c ./xz/*.c ./text/*.c -DGAME_FPS=60 -DNAME=\"warcog\" -DWIN32 -Wall -lopengl32 -lws2_32 -lgdi32 -lwinmm -lole32 -Ofast -flto -s`
