/* My Sawit OS v1.0 - By My Sawit Studio - common header */
#ifndef MYSAWIT_H
#define MYSAWIT_H
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef signed char s8; typedef short s16; typedef int s32;
#define NULL ((void*)0)
#define SCREEN_W 640
#define SCREEN_H 480

static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw(u16 p,u16 v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}
static inline void insw(u16 p,void*a,u32 n){__asm__ volatile("rep insw":"+D"(a),"+c"(n):"d"(p):"memory");}
static inline void outsw(u16 p,const void*a,u32 n){__asm__ volatile("rep outsw":"+S"(a),"+c"(n):"d"(p));}

/* multiboot info */
typedef struct {
  u32 flags,mem_lower,mem_upper,boot_device,cmdline,mods_count,mods_addr;
  u32 syms[4]; u32 mmap_length,mmap_addr,drives_length,drives_addr,config_table,boot_loader_name,apm_table;
  u32 vbe_control_info,vbe_mode_info; u16 vbe_mode,vbe_if_seg,vbe_if_off,vbe_if_len;
  u32 fb_addr_lo,fb_addr_hi,fb_pitch,fb_width,fb_height; u8 fb_bpp,fb_type;
} __attribute__((packed)) mbi_t;

/* kernel.c */
void *memcpy(void*,const void*,u32); void *memset(void*,int,u32); int memcmp(const void*,const void*,u32);
u32 strlen(const char*); int strcmp(const char*,const char*); int stricmp(const char*,const char*);
void strcpy(char*,const char*); void strncpy0(char*,const char*,u32);
int u2s(u32 v,char*b);
void timer_init(void); u32 millis(void);
void reboot(void); void shutdown(void);
extern u32 mem_end;
/* modules (files loaded by GRUB, copied to RAM) */
typedef struct { char name[24]; u8*data; u32 size; } Mod;
extern Mod mods[16]; extern int nmods;
Mod *mod_find(const char*name);

/* vga.c */
extern u32 backbuf[SCREEN_W*SCREEN_H];
void vga_init(u32 addr,u32 pitch,u32 bpp);
void put_pixel(int x,int y,u32 c);
void put_pixel_320(int x,int y,u32 c);
void fill_rect(int x,int y,int w,int h,u32 c);
void draw_rect(int x,int y,int w,int h,u32 c);
void fill_round(int x,int y,int w,int h,int r,u32 c);
void draw_line(int x0,int y0,int x1,int y1,u32 c);
void draw_char(int x,int y,char ch,u32 c,int scale);
void draw_text(int x,int y,const char*s,u32 c,int scale);
void draw_text_c(int cx,int y,const char*s,u32 c,int scale);
int text_width(const char*s,int scale);
void vga_present(void);
u32 fb_get(int x,int y); void fb_put(int x,int y,u32 c);

/* logo.c */
void draw_logo(int x,int y,int scale);
void draw_logo_size(int x,int y,int size);
void draw_logo_320(int vx,int vy);

/* cursor.c */
enum { CURSOR_ARROW=0, CURSOR_LOADING=1, CURSOR_RESIZE=2, CURSOR_TEXT=3 };
void cursor_init(void);
void draw_cursor(int x,int y);
void hide_cursor(void);
void set_cursor_type(int t);
int  get_cursor_type(void);
void cursor_tick(u32 ms);          /* animasi loading */
void cursor_after_present(void);   /* panggil setelah vga_present() */

/* mouse.c / keyboard.c */
extern int mouse_x,mouse_y,mouse_btn;
void mouse_init(void);
int  mouse_poll(void);             /* 1 jika ada paket mouse baru */
void kb_feed(u8 sc); int kb_getkey(void);
#define KEY_ENTER 10
#define KEY_ESC 27

/* ata.c, fat32.c, vfs.c */
typedef struct { char name[16]; u8 isdir; u32 size; } FsEntry;
int ata_init(void); u32 ata_sectors(void);
int ata_read(u32 lba,u32 n,void*buf); int ata_write(u32 lba,u32 n,const void*buf);
int fat32_mount(void); int fat32_ready(void);
int fat32_mkfs(u32 lba,u32 total,void(*prog)(int));
int fat32_mkdir(const char*parent,const char*name);
int fat32_writefile(const char*dir,const char*name,const u8*data,u32 size);
int fat32_list(const char*path,FsEntry*out,int max);
int fat32_read(const char*path,u8*buf,u32 max);
int vfs_list(const char*path,FsEntry*out,int max);
int vfs_read(const char*path,u8*buf,u32 max);

/* pe.c */
typedef struct { u32 version; void (*msgbox)(const char*title,const char*text); u32 (*ticks)(void); } SawitAPI;
int pe_run(const u8*img,u32 size,const SawitAPI*api,const char**err);

/* desktop.c / installer.c */
void desktop_run(void);
void ui_pump(void);                /* poll mouse, update cursor (dipakai installer) */
int installer_run(void);           /* 1 = selesai install */
extern int lang;                   /* 0 English, 1 Indonesia */
#define TR(a,b) (lang?(b):(a))
#endif
