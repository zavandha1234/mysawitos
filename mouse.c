/* mouse.c - PS/2 mouse (polling i8042). Byte keyboard diteruskan ke kb_feed(). */
#include "mysawit.h"
int mouse_x=320,mouse_y=240,mouse_btn=0;
static u8 pk[3]; static int pi;
static void wait_in(void){ for(int i=0;i<100000;i++) if(!(inb(0x64)&2)) return; }
static void wait_out(void){ for(int i=0;i<100000;i++) if(inb(0x64)&1) return; }
static void mwrite(u8 v){ wait_in(); outb(0x64,0xD4); wait_in(); outb(0x60,v); wait_out(); if(inb(0x64)&1) inb(0x60); /* ACK */ }
void mouse_init(void){
  while(inb(0x64)&1) inb(0x60);
  wait_in(); outb(0x64,0xA8);
  wait_in(); outb(0x64,0x20); wait_out(); u8 st=inb(0x60);
  st&=~0x20; wait_in(); outb(0x64,0x60); wait_in(); outb(0x60,st);
  mwrite(0xF6); mwrite(0xF4);
  pi=0;
}
int mouse_poll(void){
  int got=0;
  while(inb(0x64)&1){
    u8 st=inb(0x64); u8 d=inb(0x60);
    if(!(st&0x20)){ kb_feed(d); continue; }
    if(pi==0 && !(d&8)) continue;           /* resync */
    pk[pi++]=d;
    if(pi==3){ pi=0;
      int dx=pk[1]-((pk[0]<<4)&0x100), dy=pk[2]-((pk[0]<<3)&0x100);
      mouse_x+=dx; mouse_y-=dy;
      if(mouse_x<0)mouse_x=0; if(mouse_x>SCREEN_W-1)mouse_x=SCREEN_W-1;
      if(mouse_y<0)mouse_y=0; if(mouse_y>SCREEN_H-1)mouse_y=SCREEN_H-1;
      mouse_btn=pk[0]&7; got=1;
    }
  }
  return got;
}
