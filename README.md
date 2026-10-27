# My Sawit OS v1.0 — By My Sawit Studio

OS ringan (kernel ±58 KB, ISO ±15–30 MB karena image GRUB) dengan UI mirip Windows 11.

## Build (Debian/Ubuntu)
    sudo apt install build-essential gcc-multilib nasm python3 grub-pc-bin grub-common xorriso mtools
    ./build.sh          # -> MySawitOS-v1.0.iso

## VirtualBox
- Type: Other / Other (32-bit), RAM >= 128 MB, Display: VBoxVGA
- Storage: controller **IDE** (bukan SATA!): HDD baru (>= 64 MB) di Primary Master, ISO di Secondary Master
- System > Pointing Device: **PS/2 Mouse** (klik jendela VM untuk capture mouse)

## Alur
GRUB -> logo sawit 3 detik (cursor loading) -> Desktop (cursor panah) -> ikon "Install My Sawit OS"
-> Language, License, Disk, Copying, Finish -> keluarkan ISO, Restart -> boot dari HDD.

## File
boot.asm, linker.ld, kernel.c, vga.c, logo.c/logo.h, cursor.c/cursor.h (bitmap `cursor_arrow[16][11]`),
mouse.c, keyboard.c, ata.c, fat32.c, vfs.c, pe.c, desktop.c, installer.c, build.sh, iso/boot/grub/grub.cfg,
tools/gen_logo.py, tools/make_exe.py (membuat hello.exe & panen.exe PE32 tanpa compiler Windows)

## Catatan teknis
- Mode grafis: framebuffer linear 640x480x32 via Multiboot (GRUB). Boot logo digambar pada layar virtual
  320x200 lewat put_pixel_320() -> put_pixel() (tiap pixel 2x2).
- Cursor digambar di framebuffer depan, background disimpan/restore -> tidak flicker.
- PE loader: PE32 x86 tanpa import/relocation, ImageBase >= 0x01000000 (contoh 0x02000000),
  entry `int entry(const SawitAPI*)`; API: msgbox(title,text), ticks(). Berjalan di ring 0.
- Jam taskbar memakai RTC apa adanya (atur TZ di VM / host).

## Tanpa Linux? (build ISO otomatis)
- **GitHub Actions:** upload folder ini ke repo GitHub -> tab Actions -> "Build My Sawit OS ISO" -> Run workflow -> download `MySawitOS-v1.0-iso` (isinya MySawitOS-v1.0.iso).
- **Docker:** `./build-docker.sh`
