/* logo.c - menggambar logo sawit dari logo.h */
#include "mysawit.h"
#include "logo.h"
void draw_logo(int x,int y,int scale){
  for(int j=0;j<LOGO_H_PX;j++) for(int i=0;i<LOGO_W;i++){
    int p=logo_pixels[j*LOGO_W+i]; if(p) fill_rect(x+i*scale,y+j*scale,scale,scale,logo_palette[p]);
  }
}
/* nearest-neighbour, untuk ikon kecil */
void draw_logo_size(int x,int y,int size){
  for(int j=0;j<size;j++) for(int i=0;i<size;i++){
    int p=logo_pixels[(j*LOGO_H_PX/size)*LOGO_W+(i*LOGO_W/size)]; if(p) put_pixel(x+i,y+j,logo_palette[p]);
  }
}
/* boot logo: 80x80 pada layar 320x200 lewat put_pixel_320 -> put_pixel */
void draw_logo_320(int vx,int vy){
  for(int j=0;j<LOGO_H_PX;j++) for(int i=0;i<LOGO_W;i++){
    int p=logo_pixels[j*LOGO_W+i]; if(p) put_pixel_320(vx+i,vy+j,logo_palette[p]);
  }
}
