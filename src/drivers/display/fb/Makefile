TARGET = display/fb.so
EXTOBJS = shapes.o
include ../../Makefile.rules

shapes.o: shapes.tga
	@dd if=shapes.tga of=shapes bs=1 skip=18 2>/dev/null
	@ld -r -b binary -o shapes.o shapes
	@rm shapes
