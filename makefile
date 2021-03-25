-include makefile.inc

AUDIO_LIBS = -lgme -lalogg -lalmp3 -laldmb -ldumb
#IMAGE_LIBS = -ljpgal -lldpng -lpng -lz
#LINKOPTS = -pg -g
#OPTS = -pg -g
OPTS = -O3
#COMPRESS = 1

#CFLAG = -pedantic -Wno-long-long -Wall
CFLAG = -pedantic -Wall
#-W -Wshadow -Wpointer-arith

ifdef COMPILE_FOR_WIN
  ALLEG_LIB = -lalleg
  SFLAG = -s
  WINFLAG = -mwindows
  PLATEXT = -w
  EXEEXT = .exe
  ZC_ICON = zc_icon$(PLATEXT).o
  ZQ_ICON = zq_icon$(PLATEXT).o
  ZELDA_PREFIX = zelda
  ZC_PLATFORM = Windows
  CC = g++
  LIBDIR = -L./libs/mingw
else
ifdef COMPILE_FOR_LINUX
  PLATEXT =
  ALLEG_LIB = `allegro-config --libs --static`
  ZC_PLATFORM = Linux
  ZELDA_PREFIX = zelda
  CC = g++
  LIBDIR = -L./libs/linux
else
ifdef COMPILE_FOR_DOS
  ALLEG_LIB = -lalleg
  STDCXX_LIB = -lstdcxx
  EXEEXT = .exe
  ZC_PLATFORM = DOS
  ZELDA_PREFIX = zelda
  CC = gpp
else
ifdef COMPILE_FOR_MACOSX
  PLATEXT = -m
  ALLEG_LIB = -framework Cocoa -framework Allegro -lalleg-main
  ZC_PLATFORM = Mac OS X
  ZELDA_PREFIX = zelda
  CFLAG = -pedantic -Wno-long-long -Wall -Wno-long-double
  CC = g++
  LIBDIR= -L./libs/osx
else
ifdef COMPILE_FOR_GP2X
  PLATEXT = -g
  EXEEXT = .gpe
  ALLEG_LIB = -lalleg -lpthread -static
  ZC_PLATFORM = GP2X
  ZELDA_PREFIX = zelda
  #CFLAG = -pedantic -Wno-long-long -Wall -I/devkitGP2X/include
  CFLAG = -Wno-long-long -Wall -I/devkitGP2X/include
  CC = arm-linux-g++
  AUDIO_LIBS = -L/devkitGP2X/lib -lalspc -lalogg -lalmp3 -laldmb -ldumb
  IMAGE_LIBS = -L/devkitGP2X/lib -ljpgal -lldpng -lpng -lz
endif
endif
endif
endif
endif

ZELDA_EXE = zelda$(PLATEXT)$(EXEEXT)

ZELDA_OBJECTS = decorations$(PLATEXT).o defdata$(PLATEXT).o ending$(PLATEXT).o guys$(PLATEXT).o items$(PLATEXT).o link$(PLATEXT).o maps$(PLATEXT).o pal$(PLATEXT).o particles$(PLATEXT).o qst$(PLATEXT).o sprite$(PLATEXT).o subscr$(PLATEXT).o tiles$(PLATEXT).o title$(PLATEXT).o weapons$(PLATEXT).o zc_sys$(PLATEXT).o zcmusic$(PLATEXT).o zelda$(PLATEXT).o $(ZC_ICON)

.PHONY: default veryclean clean all msg dos win windows linux gp2x test done

default: all
msg:
	@echo Compiling Zelda Classic for $(ZC_PLATFORM)...
done:
	@echo Done!
clean:
#	rm -f $(ZELDA_OBJECTS)
	rm -f *.o
veryclean: clean
	rm -f $(ZELDA_EXE)

test:
ifndef COMPILE_FOR_WIN
ifndef COMPILE_FOR_DOS
ifndef COMPILE_FOR_LINUX
ifndef COMPILE_FOR_MACOSX
ifndef COMPILE_FOR_GP2X
	#change this if you want to change the default platform
	@make win
endif
endif
endif
endif
endif


dos:
	@echo COMPILE_FOR_DOS=1 > makefile.inc
	@make
windows: win
win:
	@echo COMPILE_FOR_WIN=1 > makefile.inc
	@make
linux:
	@echo COMPILE_FOR_LINUX=1 > makefile.inc
	@make
macosx:
	@echo COMPILE_FOR_MACOSX=1 > makefile.inc
	@make
gp2x:
	@echo COMPILE_FOR_GP2X=1 > makefile.inc
	@make

all: test msg $(ZELDA_EXE) done

$(ZELDA_EXE): $(ZELDA_OBJECTS)
	$(CC) $(LINKOPTS) -o $(ZELDA_EXE) $(ZELDA_OBJECTS) $(LIBDIR) $(IMAGE_LIBS) $(AUDIO_LIBS) $(ALLEG_LIB) $(STDCXX_LIB) $(ZC_ICON) $(SFLAG) $(WINFLAG)
ifdef COMPRESS
	upx --best $(ZELDA_EXE)
endif
ifdef COMPILE_FOR_MACOSX
	rm -rf "Zelda Classic.app"
	fixbundle $(ZELDA_EXE) -e
	cp Info1.plist $(ZELDA_EXE).app/Contents/tempinfo
	echo '	<key>CFBundleExecutable</key>' >> $(ZELDA_EXE).app/Contents/tempinfo
	echo '	<string>Zelda Classic</string>' >> $(ZELDA_EXE).app/Contents/tempinfo
	echo '	<key>CFBundleIconFile</key>' >> $(ZELDA_EXE).app/Contents/tempinfo
	echo '	<string>zc_icon.icns</string>' >> $(ZELDA_EXE).app/Contents/tempinfo
	cat $(ZELDA_EXE).app/Contents/tempinfo Info2.plist > $(ZELDA_EXE).app/Contents/Info.plist
	rm $(ZELDA_EXE).app/Contents/tempinfo
	cp "zc_icon.icns" $(ZELDA_EXE).app/Contents/Resources/
	cp zelda.dat $(ZELDA_EXE).app/
	cp sfx.dat $(ZELDA_EXE).app/
	cp fonts.dat $(ZELDA_EXE).app/
	cp qst.dat $(ZELDA_EXE).app/
	cp 1st.qst $(ZELDA_EXE).app/
	cp 2nd.qst $(ZELDA_EXE).app/
	cp 3rd.qst $(ZELDA_EXE).app/
	mv $(ZELDA_EXE).app/Contents/MacOS/$(ZELDA_EXE) "$(ZELDA_EXE).app/Contents/MacOS/Zelda Classic"
	mv $(ZELDA_EXE).app "Zelda Classic.app"
endif

decorations$(PLATEXT).o: decorations.cpp decorations.h maps.h sfx.h sprite.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c decorations.cpp -o decorations$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
defdata$(PLATEXT).o: defdata.cpp defdata.h guys.h items.h sprite.h weapons.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c defdata.cpp -o defdata$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
ending$(PLATEXT).o: ending.cpp ending.h guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h title.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c ending.cpp -o ending$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
guys$(PLATEXT).o: guys.cpp guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c guys.cpp -o guys$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
items$(PLATEXT).o: items.cpp items.h sprite.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c items.cpp -o items$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
link$(PLATEXT).o: link.cpp decorations.h guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c link.cpp -o link$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
maps$(PLATEXT).o: maps.cpp guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c maps.cpp -o maps$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
pal$(PLATEXT).o: pal.cpp items.h maps.h pal.h sfx.h sprite.h subscr.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c pal.cpp -o pal$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
particles$(PLATEXT).o: particles.cpp particles.h sprite.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c particles.cpp -o particles$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
qst$(PLATEXT).o: qst.cpp defdata.h guys.h items.h qst.h sprite.h tiles.h weapons.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c qst.cpp -o qst$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
sprite$(PLATEXT).o: sprite.cpp sprite.h tiles.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c sprite.cpp -o sprite$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
subscr$(PLATEXT).o: subscr.cpp guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c subscr.cpp -o subscr$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
tiles$(PLATEXT).o: tiles.cpp tiles.h zdefs.h
	$(CC) $(OPTS) $(CFLAG) -c tiles.cpp -o tiles$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
title$(PLATEXT).o: title.cpp items.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c title.cpp -o title$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
weapons$(PLATEXT).o: weapons.cpp link.h maps.h pal.h qst.h sfx.h sprite.h tiles.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c weapons.cpp -o weapons$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
zc_icon$(PLATEXT).o: zc_icon.rc
	windres --use-temp-file -I rc -O coff -i zc_icon.rc -o zc_icon$(PLATEXT).o
zc_sys$(PLATEXT).o: zc_sys.cpp guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h title.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c zc_sys.cpp -o zc_sys$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
zcmusic$(PLATEXT).o: zcmusic.cpp zcmusic.h
	$(CC) $(OPTS) $(CFLAG) -c zcmusic.cpp -o zcmusic$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
zelda$(PLATEXT).o: zelda.cpp ending.h fontsdat.h guys.h items.h link.h maps.h pal.h qst.h sfx.h sprite.h subscr.h tiles.h title.h weapons.h zc_sys.h zcmusic.h zdefs.h zelda.h zeldadat.h
	$(CC) $(OPTS) $(CFLAG) -c zelda.cpp -o zelda$(PLATEXT).o $(SFLAG) $(WINFLAG) $(LDFLAG)
