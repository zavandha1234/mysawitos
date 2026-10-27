/* kernel.c - My Sawit OS v1.0 kernel entry, util, timer, boot logo */
#include "mysawit.h"
void *memcpy(void*d,const void*s,u32 n){ u8*a=d;const u8*b=s; while(n--)*a++=*b++; return d; }
void *memset(void*d,int c,u32 n){ u8*a=d; while(n--)*a++=(u8)c; return d; }
int memcmp(const void*a,const void*b,u32 n){ const u8*x=a,*y=b; while(n--){ if(*x!=*y) return *x-*y; x++;y++; } return 0; }
u32 strlen(const char*s){ u32 n=0; while(s[n])n++; return n; }
int strcmp(const char*a,const char*b){ while(*a&&*a==*b){a++;b++;} return (u8)*a-(u8)*b; }
static char up(char c){ return (c>='a'&&c<='z')?c-32:c; }
int stricmp(const char*a,const char*b){ while(*a&&up(*a)==up(*b)){a++;b++;} return up(*a)-up(*b); }
void strcpy(char*d,const char*s){ while((*d++=*s++)); }
void strncpy0(char*d,const char*s,u32 n){ u32 i=0; for(;i<n&&s[i];i++) d[i]=s[i]; d[i]=0; }
int u2s(u32 v,char*b){ char t[12]; int n=0,i=0; if(!v) t[n++]='0'; while(v){ t[n++]='0'+v%10; v/=10; } while(n) b[i++]=t[--n]; b[i]=0; return i; }

/* ---- timer: PIT channel 0 di-poll (tanpa interrupt) ---- */
static u32 ms_total, acc; static u16 last_cnt=1193;
void timer_init(void){ outb(0x43,0x34); outb(0x40,1193&0xFF); outb(0x40,1193>>8); }
u32 millis(void){
  outb(0x43,0x00); u16 c=inb(0x40); c|=(u16)inb(0x40)<<8;
  u32 d=(last_cnt>=c)?(u32)(last_cnt-c):(u32)(last_cnt+1193-c); last_cnt=c;
  acc+=d; ms_total+=acc/1193; acc%=1193; return ms_total;
}
void reboot(void){ outb(0x64,0xFE); for(;;) __asm__ volatile("hlt"); }
void shutdown(void){ outw(0x4004,0x3400); outw(0x604,0x2000); outw(0xB004,0x2000); for(;;) __asm__ volatile("hlt"); }

/* ---- modul GRUB disalin ke RAM kita (aman dari tabrakan alamat saat PE di-load) ---- */
u32 mem_end=0x04000000;
Mod mods[16]; int nmods;
static u8 modstore[1536*1024];
Mod *mod_find(const char*n){ for(int i=0;i<nmods;i++) if(!stricmp(mods[i].name,n)) return &mods[i]; return NULL; }
static void load_modules(mbi_t*m){
  if(!(m->flags&8)) return; u32 *e=(u32*)m->mods_addr, off=0;
  for(u32 i=0;i<m->mods_count&&nmods<16;i++,e+=4){
    u32 sz=e[1]-e[0]; if(off+sz>sizeof modstore) break;
    const char*s=(const char*)e[2],*b=s; for(const char*p=s;*p&&*p!=' ';p++) if(*p=='/') b=p+1;
    int n=0; while(b[n]&&b[n]!=' '&&n<23){ mods[nmods].name[n]=b[n]; n++; } mods[nmods].name[n]=0;
    memcpy(modstore+off,(void*)e[0],sz); mods[nmods].data=modstore+off; mods[nmods].size=sz; off+=(sz+15)&~15u; nmods++;
  }
}

/* ---- boot screen: VGA 320x200 virtual via put_pixel() selama 3 detik ---- */
static void boot_screen(void){
  mouse_x=320; mouse_y=330;
  fill_rect(0,0,SCREEN_W,SCREEN_H,0x000000);
  draw_logo_320(120,25);                                         /* logo 80x80 */
  draw_text_c(320,2*115+40,"My Sawit OS v1.0",0x2ECC40,2);       /* hijau, di bawah logo */
  draw_text_c(320,2*140+40,"By My Sawit Studio - BOOTING...",0x808080,1);
  vga_present(); set_cursor_type(CURSOR_LOADING); draw_cursor(mouse_x,mouse_y);
  u32 t0=millis();
  while(millis()-t0<3000){ mouse_poll(); cursor_tick(millis()); }
  set_cursor_type(CURSOR_ARROW);
}

void kmain(u32 magic,mbi_t*mbi){
  if(magic!=0x2BADB002||!(mbi->flags&(1<<12))) for(;;) __asm__ volatile("hlt");
  vga_init(mbi->fb_addr_lo,mbi->fb_pitch,mbi->fb_bpp);
  if(mbi->flags&1) mem_end=0x100000+mbi->mem_upper*1024;
  load_modules(mbi);
  timer_init(); mouse_init(); cursor_init();
  if(ata_init()) fat32_mount();
  boot_screen();
  desktop_run();
}
