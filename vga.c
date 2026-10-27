/* vga.c - framebuffer (640x480x32 linear via GRUB), backbuffer, put_pixel, teks */
#include "mysawit.h"
#include "font.h"
u32 backbuf[SCREEN_W*SCREEN_H];
static u8 *fb; static u32 fb_pitch, fb_bpp;

void vga_init(u32 addr,u32 pitch,u32 bpp){ fb=(u8*)addr; fb_pitch=pitch; fb_bpp=bpp; }
void put_pixel(int x,int y,u32 c){ if((u32)x<SCREEN_W && (u32)y<SCREEN_H) backbuf[y*SCREEN_W+x]=c; }
/* layar virtual 320x200 -> tiap pixel jadi blok 2x2 pada 640x480 (offset vertikal 40) */
void put_pixel_320(int x,int y,u32 c){ int X=x*2,Y=y*2+40; put_pixel(X,Y,c);put_pixel(X+1,Y,c);put_pixel(X,Y+1,c);put_pixel(X+1,Y+1,c); }
void fill_rect(int x,int y,int w,int h,u32 c){
  int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>SCREEN_W?SCREEN_W:x+w,y1=y+h>SCREEN_H?SCREEN_H:y+h;
  for(int j=y0;j<y1;j++) for(int i=x0;i<x1;i++) backbuf[j*SCREEN_W+i]=c;
}
void draw_rect(int x,int y,int w,int h,u32 c){ fill_rect(x,y,w,1,c);fill_rect(x,y+h-1,w,1,c);fill_rect(x,y,1,h,c);fill_rect(x+w-1,y,1,h,c); }
void fill_round(int x,int y,int w,int h,int r,u32 c){
  fill_rect(x,y+r,w,h-2*r,c);
  for(int i=0;i<r;i++){
    int dy=r-i,xo=0; while((xo+1)*(xo+1)+dy*dy<=r*r) xo++;
    int inset=r-xo;
    fill_rect(x+inset,y+i,w-2*inset,1,c);
    fill_rect(x+inset,y+h-1-i,w-2*inset,1,c);
  }
}
void draw_line(int x0,int y0,int x1,int y1,u32 c){
  int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y0-y1:y1-y0, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx+dy;
  for(;;){ put_pixel(x0,y0,c); if(x0==x1&&y0==y1)break; int e2=2*e; if(e2>=dy){e+=dy;x0+=sx;} if(e2<=dx){e+=dx;y0+=sy;} }
}
void draw_char(int x,int y,char ch,u32 c,int scale){
  if(ch<32||ch>126) return;
  const u8*g=font8x8[ch-32];
  for(int r=0;r<8;r++) for(int b=0;b<8;b++) if(g[r]&(1<<b)) fill_rect(x+b*scale,y+r*scale,scale,scale,c);
}
void draw_text(int x,int y,const char*s,u32 c,int scale){ for(;*s;s++){ draw_char(x,y,*s,c,scale); x+=8*scale; } }
int text_width(const char*s,int scale){ return (int)strlen(s)*8*scale; }
void draw_text_c(int cx,int y,const char*s,u32 c,int scale){ draw_text(cx-text_width(s,scale)/2,y,s,c,scale); }

void vga_present(void){
  if(fb_bpp==32){ for(int y=0;y<SCREEN_H;y++) memcpy(fb+y*fb_pitch,&backbuf[y*SCREEN_W],SCREEN_W*4); }
  else { for(int y=0;y<SCREEN_H;y++){ u8*d=fb+y*fb_pitch; for(int x=0;x<SCREEN_W;x++){ u32 c=backbuf[y*SCREEN_W+x]; d[0]=c;d[1]=c>>8;d[2]=c>>16; d+=3; } } }
}
u32 fb_get(int x,int y){
  if((u32)x>=SCREEN_W||(u32)y>=SCREEN_H) return 0;
  u8*p=fb+y*fb_pitch+x*(fb_bpp/8);
  return fb_bpp==32? *(u32*)p : (p[0]|(p[1]<<8)|(p[2]<<16));
}
void fb_put(int x,int y,u32 c){
  if((u32)x>=SCREEN_W||(u32)y>=SCREEN_H) return;
  u8*p=fb+y*fb_pitch+x*(fb_bpp/8);
  if(fb_bpp==32) *(u32*)p=c; else {p[0]=c;p[1]=c>>8;p[2]=c>>16;}
}
