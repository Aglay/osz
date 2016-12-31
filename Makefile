include Config

all: clrdd todogen util boot system apps images

clrdd:
	@rm bin/disk.dd 2>/dev/null || true

todogen:
	@grep -ni 'TODO:' `find . 2>/dev/null` 2>/dev/null | grep -v Binary | grep -v grep >TODO.txt || true

boot: loader/bootboot.bin loader/bootboot.efi

loader/kernel.img:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/rpi-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.bin:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/mb-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.efi:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/efi-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

util: tools
	@date +'#define OSZ_BUILD "%Y-%m-%d %H:%M:%S UTC"' >etc/include/lastbuild.h
	@echo '#define OSZ_ARCH "$(ARCH)"' >>etc/include/lastbuild.h
	@echo "TOOLS"
	@make --no-print-directory -e -C tools all | grep -v 'Nothing to be done' || true

system: src
	@echo "CORE"
	@make -e --no-print-directory -C src system | grep -v 'Nothing to be done'

apps: src
	@echo "USERSPACE"
	@make -e --no-print-directory -C src libs | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C src apps | grep -v 'Nothing to be done' || true
	@echo "DRIVERS"
	@make -e --no-print-directory -C src drivers | grep -v 'Nothing to be done' || true
ifeq ($(DEBUG),1)
	@make -e --no-print-directory -C src gensyms 2>&1 | grep -v 'Nothing to be done' | grep -v 'No rule to make target' || true
endif

images: tools
	@echo "IMAGES"
	@make -e --no-print-directory -C tools images | grep -v 'Nothing to be done' | grep -v 'lowercase' || true

vdi: images
	@make -e --no-print-directory -C tools vdi | grep -v 'Nothing to be done' || true

vdmk: images
	@make -e --no-print-directory -C tools vdmk | grep -v 'Nothing to be done' || true

clean:
	@make -e --no-print-directory -C loader/efi-x86_64/zlib_inflate clean
	@make -e --no-print-directory -C src clean
	@make -e --no-print-directory -C tools clean
	@make -e --no-print-directory -C tools imgclean

test: testq

testefi:
	@echo "TEST"
	@echo
	qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda fat:bin/ESP -option-rom loader/bootboot.rom -d guest_errors -global isa-debugcon.iobase=0x402 -debugcon file:bin/ovmf.log -monitor stdio
	@qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/disk.dd -option-rom loader/bootboot.rom -d guest_errors -global isa-debugcon.iobase=0x402 -debugcon file:bin/ovmf.log -monitor stdio

testq:
	@echo "TEST"
	@echo
	qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -hda bin/disk.dd -option-rom loader/bootboot.bin -monitor stdio

testb:
	@echo "TEST"
	@echo
ifneq ($(wildcard /usr/local/bin/bochs),)
	/usr/local/bin/bochs -f etc/bochs.rc -q
else
	bochs -f etc/bochs.rc -q
endif
