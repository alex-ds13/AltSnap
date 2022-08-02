CC=gcc
WR=windres

WARNINGS=-Wall  \
	-Wformat-security \
	-Wstrict-overflow \
	-Wsign-compare \
	-Wclobbered \
	-Wempty-body \
	-Wignored-qualifiers \
	-Wuninitialized \
	-Wtype-limits \
	-Woverride-init \
	-Wlogical-op \
	-Wno-multichar \
	-Wno-attributes \
	-Wduplicated-cond \
	-Wduplicated-branches \
	-Wnull-dereference \
	-Wno-unused-function \
	-Wshadow \
	-Warray-bounds=2 \
	-Wstack-usage=4096 \
	-Werror=vla

# -Wunused-parameter
# -Wtraditional-conversion
# -fira-region=one/mixed
# -Wstack-usage=2048
# -finput-charset=UTF-8
# -Wc++-compat

CFLAGS=-Os -std=c99 \
	-finput-charset=UTF-8 \
	-m32 -march=i386 -mtune=i686 \
	-mno-stack-arg-probe \
	-mpreferred-stack-boundary=2 \
	-momit-leaf-frame-pointer \
	-fno-stack-check \
	-fno-stack-protector \
	-fno-ident \
	-fomit-frame-pointer \
	-fshort-enums \
	-fno-exceptions \
	-fno-dwarf2-cfi-asm \
	-fno-asynchronous-unwind-tables \
	-fgcse-sm \
	-fgcse-las \
	-fno-plt \
	-D__USE_MINGW_ANSI_STDIO=0 \
	-Wp,-D_FORTIFY_SOURCE=2 \
	$(WARNINGS)

LDFLAGS=-nostdlib \
	-lmsvcrt \
	-lkernel32 \
	-luser32 \
	-lgdi32 \
	-s \
	-Wl,-s,-dynamicbase \
	-Wl,-nxcompat \
	-Wl,--no-seh \
	-Wl,--relax \
	-Wl,--disable-runtime-pseudo-reloc \
	-Wl,--enable-auto-import \
	-Wl,--disable-stdcall-fixup

EXELD = $(LDFLAGS) \
	-Wl,--tsaware \
	-lcomctl32 \
	-ladvapi32 \
	-lshell32 \
	-Wl,--disable-reloc-section

default: AltSnap.exe hooks.dll

hooks.dll : hooks.c hooks.h hooksr.o unfuck.h nanolibc.h zones.c snap.c
	$(CC) -o hooks.dll hooks.c hooksr.o $(CFLAGS) $(LDFLAGS) -mdll -e_DllMain@12

AltSnap.exe : altsnapr.o altsnap.c hooks.h tray.c config.c languages.h languages.c unfuck.h nanolibc.h
	$(CC) -o AltSnap.exe altsnap.c altsnapr.o $(CFLAGS) $(EXELD) -mwindows -e_unfuckWinMain@0

altsnapr.o : altsnap.rc window.rc resource.h AltSnap.exe.manifest media/find.cur media/find.ico media/icon.ico media/tray-disabled.ico media/tray-enabled.ico
	$(WR) altsnap.rc altsnapr.o -Fpe-i386

hooksr.o: hooks.rc
	$(WR) hooks.rc hooksr.o -Fpe-i386

clean :
	rm altsnapr.o AltSnap.exe hooksr.o hooks.dll
