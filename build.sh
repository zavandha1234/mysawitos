#!/bin/bash
# My Sawit OS v1.0 - build.sh  ->  MySawitOS-v1.0.iso
# Butuh: gcc (multilib 32-bit), nasm, ld, python3, grub-pc-bin, grub-common, xorriso, mtools
#   sudo apt install build-essential gcc-multilib nasm python3 grub-pc-bin grub-common xorriso mtools
set -e
cd "$(dirname "$0")"
miss=""; for t in gcc nasm ld python3 objcopy grub-mkimage grub-mkrescue grub-file xorriso mformat; do command -v $t >/dev/null || miss="$miss $t"; done
[ -f /usr/lib/grub/i386-pc/boot.img ] || miss="$miss grub-pc-bin"
echo 'int main(){return 0;}' | gcc -m32 -x c - -c -o /dev/null 2>/dev/null || miss="$miss gcc-multilib"
if [ -n "$miss" ]; then echo "KURANG:$miss"; echo "Install: sudo apt install build-essential gcc-multilib nasm python3 grub-pc-bin grub-common xorriso mtools"; echo "Atau tanpa Linux: ./build-docker.sh  /  GitHub Actions (lihat README)"; exit 1; fi
CC=${CC:-gcc}
CFLAGS="-m32 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector -fno-tree-loop-distribute-patterns -mno-sse -mno-mmx -O2 -Wall -Wno-unused-function"
rm -rf build && mkdir -p build iso/apps iso/boot
python3 tools/gen_logo.py
python3 tools/make_exe.py iso/apps/hello.exe "Hello EXE" "Halo! Ini program .exe PE32 yang dijalankan My Sawit OS."
python3 tools/make_exe.py iso/apps/panen.exe "Panen Sawit" "Panen sawit hari ini: 1000 tandan. Mantap!"

# --- GRUB boot.img + core.img (dipakai installer untuk menulis ke MBR) ---
GRUBLIB=/usr/lib/grub/i386-pc
cp $GRUBLIB/boot.img build/boot.img
grub-mkimage -O i386-pc -o build/core.img -p '(hd0,msdos1)/boot/grub' \
  biosdisk part_msdos fat multiboot normal configfile boot echo all_video vbe video video_fb
(cd build && for f in boot.img core.img; do objcopy -I binary -O elf32-i386 -B i386 $f ${f%.img}_img.o; done)

nasm -f elf32 boot.asm -o build/boot.o
for f in kernel vga logo cursor mouse keyboard ata fat32 vfs pe desktop installer; do $CC $CFLAGS -c $f.c -o build/$f.o; done
ld -m elf_i386 -T linker.ld -o build/mysawit.bin build/boot.o build/kernel.o build/vga.o build/logo.o build/cursor.o \
   build/mouse.o build/keyboard.o build/ata.o build/fat32.o build/vfs.o build/pe.o build/desktop.o build/installer.o \
   build/boot_img.o build/core_img.o
grub-file --is-x86-multiboot build/mysawit.bin && echo "Multiboot OK"
cp build/mysawit.bin iso/boot/mysawit.bin
# ISO > 10 MB: grub-mkrescue menyertakan image GRUB BIOS (+ EFI jika tersedia)
grub-mkrescue -o MySawitOS-v1.0.iso iso
ls -lh MySawitOS-v1.0.iso
