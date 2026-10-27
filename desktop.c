/* desktop.c - Desktop mirip Windows 11: wallpaper, ikon, taskbar, Start menu, window, File Explorer, Sawit Farm */
#include "mysawit.h"
#include "cursor.h"
int lang=0;

#define TASK_Y 452
#define TASK_H 28
#define TITLE_H 24
#define C_WALL   0x0078D4
#define C_TASK   0x101010
#define C_TITLE  0x0078D4
#define C_TITLE_IN 0x7FA7C9
#define C_WHITE  0xFFFFFF
#define MAXW 8
enum { W_EXPLORER, W_FARM, W_MSG, W_ABOUT };
typedef struct {
  int id,type,x,y,w,h,min,max,px,py,pw,ph;
  char title[40], path[64], text[96];
  int sel,nent; FsEntry ents[24];
  int farm_n,farm_rp;
} Win;
static Win wins[MAXW]; static int nwin, next_id=1;
static int dirty=1, start_open=0, start_hover=-1, sel_icon=-1;
static int drag=0, drag_dx,drag_dy,drag_right; /* 1 move, 2 resize-left, 3 resize-right */
static u32 last_click_t; static int last_click_id=-1;
static u8 exebuf[256*1024];

/* ---------------- util gambar ---------------- */
static int inr(int x,int y,int rx,int ry,int rw,int rh){ return x>=rx&&y>=ry&&x<rx+rw&&y<ry+rh; }
static void button(int x,int y,int w,int h,const char*l,int hover){
  fill_rect(x,y,w,h,hover?0xE5F1FB:0xE1E1E1); draw_rect(x,y,w,h,hover?0x0078D4:0xADADAD);
  draw_text(x+(w-text_width(l,1))/2,y+(h-8)/2,l,0x000000,1);
}
static void icon_folder(int x,int y){ fill_rect(x,y+1,6,3,0xE8B53A); fill_rect(x,y+3,14,10,0xF4C84B); fill_rect(x,y+5,14,1,0xFBE08A); }
static void icon_file(int x,int y,u32 c){ fill_rect(x+1,y,11,14,0xFFFFFF); draw_rect(x+1,y,11,14,0x707070); fill_rect(x+1,y+3,11,4,c); }
static void icon_big(int kind,int x,int y){
  if(kind==0){ /* This PC */
    fill_rect(x+4,y+6,40,28,0xDDDDDD); fill_rect(x+7,y+9,34,22,0x2B88D8); fill_rect(x+7,y+9,34,6,0x5BA8E8);
    fill_rect(x+20,y+34,8,5,0xBBBBBB); fill_rect(x+12,y+39,24,3,0xDDDDDD);
  } else if(kind==1){ draw_logo_size(x,y,48); }
  else { /* Install */
    fill_round(x+4,y+4,40,40,6,0xFFFFFF); fill_rect(x+20,y+10,8,14,0x0078D4);
    for(int i=0;i<8;i++) fill_rect(x+14+i,y+24+i,20-2*i,1,0x0078D4);
    fill_rect(x+12,y+36,24,3,0x2E9B3A);
  }
}
typedef struct { int x,y; const char*l1,*l2; int kind; } DIcon;
static const DIcon icons[3]={{20,16,"This PC","",0},{20,100,"Sawit Farm","",1},{20,184,"Install My","Sawit OS",2}};

/* ---------------- window mgmt ---------------- */
static Win *win_open(int type,const char*title,int x,int y,int w,int h){
  if(nwin==MAXW) return NULL;
  Win*n=&wins[nwin++]; memset(n,0,sizeof *n);
  n->id=next_id++; n->type=type; n->x=x;n->y=y;n->w=w;n->h=h; strncpy0(n->title,title,39); dirty=1; return n;
}
static int bring_front(int i){ Win t=wins[i]; for(int k=i;k<nwin-1;k++) wins[k]=wins[k+1]; wins[nwin-1]=t; dirty=1; return nwin-1; }
static void close_win(int i){ for(int k=i;k<nwin-1;k++) wins[k]=wins[k+1]; nwin--; dirty=1; }
static int active_idx(void){ for(int i=nwin-1;i>=0;i--) if(!wins[i].min) return i; return -1; }
static int win_at(int x,int y){ for(int i=nwin-1;i>=0;i--) if(!wins[i].min&&inr(x,y,wins[i].x,wins[i].y,wins[i].w,wins[i].h)) return i; return -1; }
/* -1 none, 1 kiri, 2 kanan  (area border window untuk cursor resize) */
static int edge_hit(int x,int y){
  for(int i=nwin-1;i>=0;i--){ Win*w=&wins[i]; if(w->min) continue;
    if(inr(x,y,w->x-3,w->y,w->w+6,w->h)){ if(w->max) return -1;
      if(x<=w->x+2) return 1; if(x>=w->x+w->w-3) return 2; return -1; } }
  return -1;
}
static void msgbox(const char*title,const char*text){
  Win*w=win_open(W_MSG,title,170+(nwin%4)*20,130+(nwin%4)*20,300,130); if(!w) return; strncpy0(w->text,text,95);
}
static u32 api_ticks(void){ return millis(); }
static const SawitAPI api={1,msgbox,api_ticks};

/* ---------------- Explorer ---------------- */
static void exp_load(Win*w){
  w->sel=-1;
  if(!w->path[0]){ strcpy(w->ents[0].name,"My Sawit (C:)"); w->ents[0].isdir=1; w->ents[0].size=0; w->nent=1; return; }
  int n=vfs_list(w->path,w->ents,24); w->nent=n<0?0:n;
}
static void launch_explorer(const char*path){
  Win*w=win_open(W_EXPLORER,"File Explorer",60+(nwin%5)*24,40+(nwin%5)*24,420,300); if(!w) return;
  strcpy(w->path,path); exp_load(w);
}
static void launch_farm(void){ win_open(W_FARM,"Sawit Farm",150,40,340,300); }
static void launch_about(void){ win_open(W_ABOUT,"About My Sawit OS",170,90,300,200); }
static void run_exe(const char*path){
  set_cursor_type(CURSOR_LOADING); u32 t=millis(); while(millis()-t<900) ui_pump();
  int n=vfs_read(path,exebuf,sizeof exebuf); const char*err="";
  if(n<=0) msgbox("Error","Tidak bisa membaca file."); else if(pe_run(exebuf,(u32)n,&api,&err)) msgbox("Cannot run",err);
  set_cursor_type(CURSOR_ARROW); dirty=1;
}
static void exp_up(Win*w){
  int n=strlen(w->path); if(!n) return; int i=n-1; while(i>0&&w->path[i]!='/') i--;
  if(i==0) { if(n==1) w->path[0]=0; else strcpy(w->path,"/"); } else w->path[i]=0;
  exp_load(w);
}
static void exp_open(Win*w,int i){
  FsEntry*e=&w->ents[i]; char p[64];
  if(!w->path[0]){ strcpy(w->path,"/"); exp_load(w); return; }
  strcpy(p,w->path); if(p[strlen(p)-1]!='/') strcpy(p+strlen(p),"/"); strcpy(p+strlen(p),e->name);
  if(e->isdir){ strcpy(w->path,p); exp_load(w); }
  else { int l=strlen(e->name); if(l>4&&!stricmp(e->name+l-4,".EXE")) run_exe(p); else msgbox("File Explorer","Tidak ada aplikasi untuk membuka file ini."); }
}
static void fmt_size(char*o,u32 s){ int n=u2s(s/1024+(s?1:0),o); strcpy(o+n," KB"); }
static void exp_draw(Win*w){
  int cx=w->x+1,cy=w->y+TITLE_H,cw=w->w-2,ch=w->h-TITLE_H-1;
  fill_rect(cx,cy,cw,26,0xF3F3F3); button(cx+4,cy+4,22,18,"<",inr(mouse_x,mouse_y,cx+4,cy+4,22,18));
  fill_rect(cx+32,cy+4,cw-38,18,0xFFFFFF); draw_rect(cx+32,cy+4,cw-38,18,0xA0A0A0);
  char addr[80]; if(!w->path[0]) strcpy(addr,"This PC"); else { strcpy(addr,"C:"); strcpy(addr+2,w->path); for(char*c=addr;*c;c++) if(*c=='/')*c='\\'; if(!w->path[1]) strcpy(addr,"C:\\"); }
  draw_text(cx+38,cy+9,addr,0x000000,1);
  fill_rect(cx,cy+26,100,ch-26,0xF3F3F3); fill_rect(cx+100,cy+26,1,ch-26,0xD0D0D0);
  const char*nav[3]={"This PC","My Sawit (C:)","Apps"};
  for(int i=0;i<3;i++){ int ny=cy+32+i*22; if(inr(mouse_x,mouse_y,cx,ny,100,20)) fill_rect(cx+2,ny,96,20,0xE0E8F0);
    if(i) icon_folder(cx+6,ny+3); else fill_rect(cx+6,ny+3,14,10,0x2B88D8); draw_text(cx+24,ny+6,nav[i],0x000000,1); }
  int lx=cx+101,lw=cw-101; fill_rect(lx,cy+26,lw,ch-26,0xFFFFFF);
  fill_rect(lx,cy+26,lw,16,0xFFFFFF); fill_rect(lx,cy+41,lw,1,0xD8D8D8);
  draw_text(lx+26,cy+30,TR("Name","Nama"),0x606060,1); draw_text(lx+lw-70,cy+30,"Size",0x606060,1);
  for(int i=0;i<w->nent&&i<14;i++){ FsEntry*e=&w->ents[i]; int ry=cy+44+i*18;
    if(i==w->sel) fill_rect(lx,ry,lw,18,0xCCE8FF); else if(inr(mouse_x,mouse_y,lx,ry,lw,18)) fill_rect(lx,ry,lw,18,0xE9F3FC);
    int l=strlen(e->name); if(e->isdir) icon_folder(lx+6,ry+2); else icon_file(lx+6,ry+1,(l>4&&!stricmp(e->name+l-4,".EXE"))?0x2E9B3A:0x0078D4);
    draw_text(lx+26,ry+5,e->name,0x000000,1);
    if(!e->isdir){ char s[20]; fmt_size(s,e->size); draw_text(lx+lw-70,ry+5,s,0x404040,1); } }
  if(!w->nent) draw_text(lx+10,cy+60,TR("This folder is empty.","Folder ini kosong."),0x808080,1);
  char st[40]; int n=u2s(w->nent,st); strcpy(st+n,TR(" items"," item")); fill_rect(cx,cy+ch-14,cw,14,0xF3F3F3); draw_text(cx+6,cy+ch-11,st,0x404040,1);
}
static void exp_click(Win*w,int mx,int my,int dbl){
  int cx=w->x+1,cy=w->y+TITLE_H;
  if(inr(mx,my,cx+4,cy+4,22,18)){ exp_up(w); dirty=1; return; }
  for(int i=0;i<3;i++) if(inr(mx,my,cx,cy+32+i*22,100,20)){ strcpy(w->path,i==0?"":(i==1?"/":"/APPS")); exp_load(w); dirty=1; return; }
  int lx=cx+101; if(mx>=lx&&my>=cy+44){ int i=(my-(cy+44))/18; if(i<w->nent&&i<14){ dirty=1; if(dbl&&w->sel==i) exp_open(w,i); else w->sel=i; } else { w->sel=-1; dirty=1; } }
}

/* ---------------- window drawing ---------------- */
static void draw_window(Win*w,int active){
  int x=w->x,y=w->y,ww=w->w,hh=w->h;
  fill_rect(x+4,y+4,ww,hh,0x0A4A80);                       /* bayangan */
  fill_rect(x,y,ww,hh,0xFFFFFF); draw_rect(x,y,ww,hh,active?C_TITLE:0x808080);
  fill_rect(x,y,ww,TITLE_H,active?C_TITLE:C_TITLE_IN);
  draw_logo_size(x+5,y+4,16); char t[28]; strncpy0(t,w->title,(ww-120)/8>27?27:(ww-120)/8); draw_text(x+26,y+8,t,C_WHITE,1);
  for(int k=0;k<3;k++){ int bx=x+ww-29-k*28,by=y+1; int hv=inr(mouse_x,mouse_y,bx,by,28,TITLE_H-1);
    if(hv) fill_rect(bx,by,28,TITLE_H-1,k==0?0xC42B1C:0x3A94E0);
    if(k==0){ draw_line(bx+9,by+7,bx+18,by+16,C_WHITE); draw_line(bx+10,by+7,bx+19,by+16,C_WHITE); draw_line(bx+18,by+7,bx+9,by+16,C_WHITE); draw_line(bx+19,by+7,bx+10,by+16,C_WHITE); }
    else if(k==1){ if(w->max){ draw_rect(bx+11,by+6,9,9,C_WHITE); draw_rect(bx+9,by+8,9,9,C_WHITE); } else draw_rect(bx+9,by+7,10,10,C_WHITE); }
    else fill_rect(bx+9,by+14,10,1,C_WHITE); }
  int cx=x+1,cy=y+TITLE_H,cw=ww-2,ch=hh-TITLE_H-1;
  if(w->type==W_EXPLORER) exp_draw(w);
  else if(w->type==W_FARM){
    fill_rect(cx,cy,cw,ch,0x87CEEB); fill_rect(cx,cy+ch-70,cw,70,0x5DAA3A);
    draw_logo(cx+cw/2-80,cy+10,2);
    char s[48]; strcpy(s,"Tandan: "); u2s(w->farm_n,s+8); fill_rect(cx+8,cy+ch-62,150,16,0xFFFFFF); draw_text(cx+12,cy+ch-58,s,0x000000,1);
    strcpy(s,"Rp "); u2s(w->farm_rp,s+3); fill_rect(cx+8,cy+ch-42,150,16,0xFFFFFF); draw_text(cx+12,cy+ch-38,s,0x000000,1);
    button(cx+cw-110,cy+ch-50,100,26,TR("Sell","Jual"),inr(mouse_x,mouse_y,cx+cw-110,cy+ch-50,100,26));
    draw_text_c(cx+cw/2,cy+ch-14,TR("Click the tree to harvest!","Klik pohon untuk panen!"),0x103010,1);
  } else if(w->type==W_MSG){
    for(int i=0;i<3;i++){ char ln[32]; int o=i*30; if((int)strlen(w->text)<=o) break; strncpy0(ln,w->text+o,30); draw_text(cx+12,cy+14+i*14,ln,0x000000,1); }
    button(cx+cw-80,cy+ch-34,64,24,"OK",inr(mouse_x,mouse_y,cx+cw-80,cy+ch-34,64,24));
  } else if(w->type==W_ABOUT){
    draw_logo(cx+10,cy+12,1); draw_text(cx+100,cy+20,"My Sawit OS",0x1B5E20,2); draw_text(cx+100,cy+44,"Version 1.0",0x000000,1);
    draw_text(cx+100,cy+58,"By My Sawit Studio",0x000000,1); draw_text(cx+10,cy+108,TR("Lightweight OS, familiar UI.","OS ringan dengan tampilan akrab."),0x303030,1);
    draw_text(cx+10,cy+124,"Windows-like. Made in Indonesia.",0x303030,1);
  }
}

/* ---------------- render desktop ---------------- */
static const char*menu_items[6]={"File Explorer","Sawit Farm","Install My Sawit OS","About My Sawit OS","Restart","Shut down"};
static int cmos(int r){ outb(0x70,r); return inb(0x71); }
static void get_time(int*h,int*m){ int hh=cmos(4),mm=cmos(2); if(!(cmos(0x0B)&4)){ hh=(hh&15)+(hh>>4)*10; mm=(mm&15)+(mm>>4)*10; } *h=hh%24; *m=mm; }
static void tb_order(int*o){ int n=nwin; for(int i=0;i<n;i++) o[i]=i; for(int i=1;i<n;i++){ int k=o[i],j=i-1; while(j>=0&&wins[o[j]].id>wins[k].id){ o[j+1]=o[j]; j--; } o[j+1]=k; } }
#define MENU_W 220
#define MENU_H (44+6*28+8)
static void render(void){
  fill_rect(0,0,SCREEN_W,SCREEN_H,C_WALL);
  for(int i=0;i<3;i++){ const DIcon*d=&icons[i]; if(i==sel_icon) fill_rect(d->x-8,d->y-6,80,74,0x3E94E0);
    icon_big(d->kind,d->x+4,d->y); draw_text_c(d->x+28,d->y+52,d->l1,0xFFFFFF,1); if(d->l2[0]) draw_text_c(d->x+28,d->y+61,d->l2,0xFFFFFF,1); }
  int act=active_idx();
  for(int i=0;i<nwin;i++) if(!wins[i].min) draw_window(&wins[i],i==act);
  fill_rect(0,TASK_Y,SCREEN_W,TASK_H,C_TASK); fill_rect(0,TASK_Y,SCREEN_W,1,0x303030);
  int sh=inr(mouse_x,mouse_y,4,TASK_Y+3,72,22); if(start_open||sh) fill_round(4,TASK_Y+3,72,22,4,0x3A3A3A);
  draw_logo_size(8,TASK_Y+6,16); draw_text(30,TASK_Y+10,"Sawit",C_WHITE,1);
  int o[MAXW]; tb_order(o);
  for(int k=0;k<nwin;k++){ Win*w=&wins[o[k]]; int bx=86+k*104; u32 c=(o[k]==act)?0x4A4A4A:0x2A2A2A;
    fill_round(bx,TASK_Y+3,100,22,3,c); if(o[k]==act) fill_rect(bx+20,TASK_Y+TASK_H-3,60,2,0x60CDFF);
    draw_logo_size(bx+4,TASK_Y+6,14); char t[11]; strncpy0(t,w->title,10); draw_text(bx+22,TASK_Y+10,t,w->min?0x909090:C_WHITE,1); }
  int h,m; get_time(&h,&m); char tm[8]; tm[0]='0'+h/10;tm[1]='0'+h%10;tm[2]=':';tm[3]='0'+m/10;tm[4]='0'+m%10;tm[5]=0;
  draw_text(SCREEN_W-52,TASK_Y+10,tm,C_WHITE,1);
  if(start_open){ int my=TASK_Y-MENU_H-4; fill_round(4,my,MENU_W,MENU_H,8,0x2B2B2B); draw_rect(4,my,MENU_W,MENU_H,0x505050);
    draw_logo_size(14,my+8,24); draw_text(46,my+16,"My Sawit OS",C_WHITE,1);
    for(int i=0;i<6;i++){ int iy=my+40+i*28; if(i==start_hover) fill_rect(10,iy,MENU_W-12,26,0x444444);
      if(i==0) icon_folder(18,iy+6); else if(i==1) draw_logo_size(18,iy+4,18); else if(i==2) fill_rect(18,iy+6,14,14,0x2E9B3A); else if(i==3) fill_rect(18,iy+6,14,14,0x0078D4); else fill_rect(18,iy+6,14,14,i==4?0xE8B53A:0xC42B1C);
      draw_text(44,iy+9,menu_items[i],C_WHITE,1); } }
}

/* ---------------- input ---------------- */
void ui_pump(void){ if(mouse_poll()) draw_cursor(mouse_x,mouse_y); cursor_tick(millis()); }
static void start_action(int i){
  start_open=0; dirty=1;
  if(i==0) launch_explorer("/"); else if(i==1) launch_farm();
  else if(i==2){ installer_run(); if(fat32_ready()) {} dirty=1; set_cursor_type(CURSOR_ARROW); }
  else if(i==3) launch_about(); else if(i==4) reboot(); else shutdown();
}
static void toggle_max(Win*w){
  if(!w->max){ w->px=w->x;w->py=w->y;w->pw=w->w;w->ph=w->h; w->x=0;w->y=0;w->w=SCREEN_W;w->h=TASK_Y; w->max=1; }
  else { w->x=w->px;w->y=w->py;w->w=w->pw;w->h=w->ph; w->max=0; }
  dirty=1;
}
static void on_down(void){
  int mx=mouse_x,my=mouse_y; u32 now=millis();
  if(start_open){ int mtop=TASK_Y-MENU_H-4;
    if(inr(mx,my,4,mtop,MENU_W,MENU_H)){ for(int i=0;i<6;i++) if(inr(mx,my,10,mtop+40+i*28,MENU_W-12,26)){ start_action(i); return; } return; }
    start_open=0; dirty=1; if(inr(mx,my,4,TASK_Y+3,72,22)) return; }
  if(my>=TASK_Y){
    if(inr(mx,my,4,TASK_Y+3,72,22)){ start_open=1; start_hover=-1; dirty=1; return; }
    int o[MAXW]; tb_order(o); int act=active_idx();
    for(int k=0;k<nwin;k++) if(inr(mx,my,86+k*104,TASK_Y+3,100,22)){ int i=o[k];
      if(wins[i].min){ wins[i].min=0; bring_front(i); } else if(i==act) wins[i].min=1; else bring_front(i); dirty=1; return; }
    return; }
  int e=edge_hit(mx,my);
  if(e>0){ int i=win_at(mx+(e==1?3:-3),my); if(i<0){ for(i=nwin-1;i>=0;i--) if(inr(mx,my,wins[i].x-3,wins[i].y,wins[i].w+6,wins[i].h)) break; }
    if(i>=0){ i=bring_front(i); drag=e==1?2:3; drag_right=wins[i].x+wins[i].w; return; } }
  int i=win_at(mx,my);
  if(i>=0){ i=bring_front(i); Win*w=&wins[i];
    for(int k=0;k<3;k++){ int bx=w->x+w->w-29-k*28; if(inr(mx,my,bx,w->y+1,28,TITLE_H-1)){ if(k==0) close_win(i); else if(k==1) toggle_max(w); else { w->min=1; dirty=1; } return; } }
    if(my<w->y+TITLE_H){ if(last_click_id==w->id&&now-last_click_t<400&&w->type!=W_MSG){ toggle_max(w); last_click_id=-1; return; }
      last_click_id=w->id; last_click_t=now; if(!w->max){ drag=1; drag_dx=mx-w->x; drag_dy=my-w->y; } return; }
    int cx=w->x+1,cy=w->y+TITLE_H,cw=w->w-2,ch=w->h-TITLE_H-1; int dbl=(last_click_id==w->id+1000&&now-last_click_t<450);
    last_click_id=w->id+1000; last_click_t=now; dirty=1;
    if(w->type==W_EXPLORER) exp_click(w,mx,my,dbl);
    else if(w->type==W_MSG){ if(inr(mx,my,cx+cw-80,cy+ch-34,64,24)) close_win(i); }
    else if(w->type==W_FARM){ if(inr(mx,my,cx+cw-110,cy+ch-50,100,26)){ w->farm_rp+=w->farm_n*5000; w->farm_n=0; } else if(inr(mx,my,cx+cw/2-80,cy+10,160,160)) w->farm_n++; }
    return; }
  for(int k=0;k<3;k++){ const DIcon*d=&icons[k]; if(inr(mx,my,d->x-8,d->y-6,80,74)){ int dbl=(last_click_id==-2-k&&now-last_click_t<450);
      last_click_id=-2-k; last_click_t=now; sel_icon=k; dirty=1;
      if(dbl){ sel_icon=-1; last_click_id=-1; if(k==0) launch_explorer(""); else if(k==1) launch_farm(); else start_action(2); } return; } }
  if(sel_icon>=0){ sel_icon=-1; dirty=1; }
}
static void on_move(void){
  if(drag&&nwin){ Win*w=&wins[nwin-1];
    if(drag==1){ w->x=mouse_x-drag_dx; w->y=mouse_y-drag_dy; if(w->y<0)w->y=0; if(w->y>TASK_Y-TITLE_H)w->y=TASK_Y-TITLE_H; }
    else if(drag==2){ int nx=mouse_x; if(drag_right-nx<160) nx=drag_right-160; w->x=nx; w->w=drag_right-nx; }
    else { int nw=mouse_x-w->x; w->w=nw<160?160:nw; }
    dirty=1; return; }
  if(start_open){ int mtop=TASK_Y-MENU_H-4,h=-1; for(int i=0;i<6;i++) if(inr(mouse_x,mouse_y,10,mtop+40+i*28,MENU_W-12,26)) h=i; if(h!=start_hover){ start_hover=h; dirty=1; } }
}
static void update_cursor(void){
  if(get_cursor_type()==CURSOR_LOADING) return;
  int t=CURSOR_ARROW;
  if(drag>=2||(!drag&&!start_open&&mouse_y<TASK_Y&&edge_hit(mouse_x,mouse_y)>0)) t=CURSOR_RESIZE;
  else if(!drag){ int i=win_at(mouse_x,mouse_y); if(i>=0&&wins[i].type==W_EXPLORER&&inr(mouse_x,mouse_y,wins[i].x+33,wins[i].y+TITLE_H+4,wins[i].w-40,18)) t=CURSOR_TEXT; }
  set_cursor_type(t);
}
void desktop_run(void){
  int pb=0,lx=-1,ly=-1; u32 lastsec=0; dirty=1;
  for(;;){
    u32 now=millis(); int moved=mouse_poll(); int k;
    while((k=kb_getkey())) if(k==KEY_ESC&&start_open){ start_open=0; dirty=1; }
    if(moved){
      if(mouse_x!=lx||mouse_y!=ly){ lx=mouse_x;ly=mouse_y; draw_cursor(lx,ly); on_move(); }
      int nb=mouse_btn&1; if(nb&&!pb) on_down(); if(!nb&&pb){ drag=0; } pb=nb;
      static int hov_t; int hv=win_at(mouse_x,mouse_y)*100+(mouse_y>=TASK_Y); if(hv!=hov_t){ hov_t=hv; dirty=1; }
    }
    update_cursor();
    if(now/1000!=lastsec){ lastsec=now/1000; if((lastsec%5)==0) dirty=1; }
    if(dirty){ dirty=0; render(); vga_present(); cursor_after_present(); }
    cursor_tick(now);
  }
}
