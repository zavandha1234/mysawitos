/* cursor.c - cursor mirip Windows 11: arrow, loading (spinner biru), resize <->, I-beam.
   Digambar langsung ke framebuffer depan; background di bawah cursor disimpan lalu
   di-restore saat bergerak (tidak flicker karena scene tidak digambar ulang). */
#include "mysawit.h"
#include "cursor.h"

const char cursor_arrow[16][12]={
 "1..........","11.........","121........","1221.......","12221......","122221.....",
 "1222221....","12222221...","122222221..","1222222221.","12222211111","1221221....",
 "121.1221...","11..1221...","1....1221..",".....1221.."};
static const char cursor_resize[9][17]={
 "...1........1...","..11........11..",".121........121.","1221111111111221",
 "1222222222222221","1221111111111221",".121........121.","..11........11..","...1........1..."};
static const char cursor_text[15][8]={
 "1111111","1222221","1112111","..121..","..121..","..121..","..121..","..121..",
 "..121..","..121..","..121..","1112111","1222221","1111111","......."};

#define CW 16
static u32 img[CW*CW], saved[CW*CW];
static int cx=320,cy=240,hx=0,hy=0,vis=0,type=CURSOR_ARROW,frame=0,sx0,sy0;
static u32 last_tick=0;

static void put_img(int x,int y,u32 c){ if((u32)x<CW&&(u32)y<CW) img[y*CW+x]=c; }
static void build(void){
  memset(img,0,sizeof img);
  if(type==CURSOR_ARROW){ hx=hy=0;
    for(int r=0;r<16;r++) for(int c=0;c<11;c++){ char k=cursor_arrow[r][c]; if(k=='1')put_img(c,r,0x000001); else if(k=='2')put_img(c,r,0xFFFFFF); }
  } else if(type==CURSOR_RESIZE){ hx=8;hy=4;
    for(int r=0;r<9;r++) for(int c=0;c<16;c++){ char k=cursor_resize[r][c]; if(k=='1')put_img(c,r,0x000001); else if(k=='2')put_img(c,r,0xFFFFFF); }
  } else if(type==CURSOR_TEXT){ hx=3;hy=8;
    for(int r=0;r<15;r++) for(int c=0;c<7;c++){ char k=cursor_text[r][c]; if(k=='1')put_img(c,r,0x000001); else if(k=='2')put_img(c,r,0xFFFFFF); }
  } else { /* LOADING: cincin 8 titik biru berputar (gaya Windows 11) */
    hx=8;hy=8;
    static const s8 ox[8]={0,4,6,4,0,-4,-6,-4}, oy[8]={-6,-4,0,4,6,4,0,-4};
    static const u32 col[8]={0x0050C8,0x1E78E6,0x4690EC,0x78AEF2,0xA0C6F6,0xC0D8F8,0xD6E4FA,0xE6EEFB};
    for(int i=0;i<8;i++){ u32 c=col[(i+8-(frame&7))&7];
      for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++) put_img(8+ox[i]+dx,8+oy[i]+dy,c); }
  }
}
static void save_bg(void){ sx0=cx-hx; sy0=cy-hy; for(int j=0;j<CW;j++)for(int i=0;i<CW;i++) saved[j*CW+i]=fb_get(sx0+i,sy0+j); }
static void restore_bg(void){ for(int j=0;j<CW;j++)for(int i=0;i<CW;i++) fb_put(sx0+i,sy0+j,saved[j*CW+i]); }
static void blit(void){ for(int j=0;j<CW;j++)for(int i=0;i<CW;i++){ u32 c=img[j*CW+i]; if(c) fb_put(sx0+i,sy0+j,c==0x000001?0:c);} }

void cursor_init(void){ type=CURSOR_ARROW; vis=0; build(); }
void hide_cursor(void){ if(vis){ restore_bg(); vis=0; } }
void draw_cursor(int x,int y){ hide_cursor(); cx=x; cy=y; build(); save_bg(); blit(); vis=1; }
void set_cursor_type(int t){ if(t==type) return; type=t; draw_cursor(cx,cy); }
int  get_cursor_type(void){ return type; }
void cursor_tick(u32 ms){ if(type==CURSOR_LOADING && ms-last_tick>=90){ last_tick=ms; frame++; draw_cursor(cx,cy); } }
void cursor_after_present(void){ vis=0; draw_cursor(cx,cy); }
