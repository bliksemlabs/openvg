INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/src/hello_pi/libs/ilclient -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -Wno-deprecated-declarations -fPIC
LIBFLAGS=-L/opt/vc/lib -lEGL -lGLESv2
FONTLIB=/usr/share/fonts/truetype/ttf-dejavu
FONTFILES=DejaVuSans.inc  DejaVuSansMono.inc DejaVuSerif.inc
all:	font2openvg fonts library	

libshapes.o:	libshapes.c shapes.h fontinfo.h fonts
	gcc -O2 -Wall $(INCLUDEFLAGS) -c libshapes.c

lodepng.o: lodepng.c lodepng.h
	gcc -O2 -Wall $(INCLUDEFLAGS) -c lodepng.c

jpeg.o: jpeg.c jpeg.h
	gcc -O2 -Wall $(INCLUDEFLAGS) -c jpeg.c

gopenvg:	openvg.go
	go install .

oglinit.o:	oglinit.c
	gcc -O2 -Wall $(INCLUDEFLAGS) -c oglinit.c

font2openvg:	fontutil/font2openvg.cpp
	g++ -I/usr/include/freetype2 fontutil/font2openvg.cpp -o font2openvg -lfreetype

fonts:	$(FONTFILES)

DejaVuSans.inc: font2openvg $(FONTLIB)/DejaVuSans.ttf
	./font2openvg $(FONTLIB)/DejaVuSans.ttf DejaVuSans.inc DejaVuSans

DejaVuSerif.inc: font2openvg $(FONTLIB)/DejaVuSerif.ttf
	./font2openvg $(FONTLIB)/DejaVuSerif.ttf DejaVuSerif.inc DejaVuSerif

DejaVuSansMono.inc: font2openvg $(FONTLIB)/DejaVuSansMono.ttf
	./font2openvg $(FONTLIB)/DejaVuSansMono.ttf DejaVuSansMono.inc DejaVuSansMono

clean:
	rm -f *.o *.inc *.so font2openvg *.c~ *.h~
	indent -linux -c 60 -brf -l 132  libshapes.c oglinit.c shapes.h fontinfo.h

library: oglinit.o lodepng.o jpeg.o libshapes.o
	gcc $(LIBFLAGS) -shared -o libshapes.so oglinit.o lodepng.o jpeg.o libshapes.o

install:
	install -m 755 -p font2openvg /usr/bin/
	install -m 755 -p libshapes.so /usr/lib/libshapes.so.1.0.0
	strip --strip-unneeded /usr/lib/libshapes.so.1.0.0
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1.0
	install -m 644 -p shapes.h /usr/include/
	install -m 644 -p fontinfo.h /usr/include/

uninstall:
	rm -f /usr/bin/font2openvg
	rm -f /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1.0 /usr/lib/libshapes.so.1 /usr/lib/libshapes.so
	rm -f /usr/include/shapes.h /usr/include/fontinfo.h
