/* ata.c - ATA PIO LBA28, primary master (IDE controller di VirtualBox) */
#include "mysawit.h"
#define P 0x1F0
static u32 total=0; static int present=0;
static int wait_bsy(void){ for(u32 i=0;i<4000000;i++) if(!(inb(P+7)&0x80)) return 0; return -1; }
static int wait_drq(void){ for(u32 i=0;i<4000000;i++){ u8 s=inb(P+7); if(s&1) return -1; if(!(s&0x80)&&(s&8)) return 0; } return -1; }
int ata_init(void){
  if(inb(P+7)==0xFF) return 0;
  outb(P+6,0xA0); for(int i=0;i<16;i++) inb(0x3F6);
  outb(P+2,0);outb(P+3,0);outb(P+4,0);outb(P+5,0); outb(P+7,0xEC);
  if(inb(P+7)==0) return 0;
  if(wait_bsy()) return 0;
  if(inb(P+4)||inb(P+5)) return 0;      /* ATAPI/SATA */
  if(wait_drq()) return 0;
  u16 id[256]; insw(P,id,256);
  total=id[60]|((u32)id[61]<<16); present=total>0; return present;
}
u32 ata_sectors(void){ return present?total:0; }
static void setup(u32 lba,u8 cmd){
  wait_bsy(); outb(P+6,0xE0|((lba>>24)&0xF)); outb(P+2,1);
  outb(P+3,lba); outb(P+4,lba>>8); outb(P+5,lba>>16); outb(P+7,cmd);
}
int ata_read(u32 lba,u32 n,void*buf){
  if(!present) return -1; u8*b=buf;
  for(u32 i=0;i<n;i++){ setup(lba+i,0x20); if(wait_drq()) return -1; insw(P,b+i*512,256); }
  return 0;
}
int ata_write(u32 lba,u32 n,const void*buf){
  if(!present) return -1; const u8*b=buf;
  for(u32 i=0;i<n;i++){ setup(lba+i,0x30); if(wait_drq()) return -1; outsw(P,b+i*512,256); if(wait_bsy()) return -1; }
  outb(P+7,0xE7); wait_bsy(); return 0;
}
