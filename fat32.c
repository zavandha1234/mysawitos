/* fat32.c - driver FAT32 sederhana (nama 8.3): mount, list, read, mkfs, mkdir, write file */
#include "mysawit.h"
static struct { int ok; u32 lba,spc,rsvd,fatsz,root,data,nclus,nextfree; } F;
static u8 sec[512], sec2[512], fcache[512], zero8k[8192];
static u32 fcache_lba=0xFFFFFFFF;
static u32 c2l(u32 c){ return F.data+(c-2)*F.spc; }
static int rd(u32 l,void*b){ return ata_read(l,1,b); }
static int wr(u32 l,const void*b){ return ata_write(l,1,b); }
int fat32_ready(void){ return F.ok; }

static u32 fat_get(u32 c){
  u32 off=c*4, s=F.lba+F.rsvd+off/512;
  if(s!=fcache_lba){ if(rd(s,fcache)) return 0x0FFFFFFF; fcache_lba=s; }
  return (*(u32*)(fcache+off%512))&0x0FFFFFFF;
}
static int fat_set(u32 c,u32 v){
  u32 off=c*4;
  for(int i=0;i<2;i++){ u32 s=F.lba+F.rsvd+i*F.fatsz+off/512; if(rd(s,sec2)) return -1; *(u32*)(sec2+off%512)=v; if(wr(s,sec2)) return -1; }
  fcache_lba=0xFFFFFFFF; return 0;
}
static u32 alloc(void){
  for(u32 c=F.nextfree;c<F.nclus+2;c++) if(fat_get(c)==0){ fat_set(c,0x0FFFFFFF); F.nextfree=c+1; return c; }
  return 0;
}
static void zero_cluster(u32 c){ memset(sec,0,512); for(u32 i=0;i<F.spc;i++) wr(c2l(c)+i,sec); }

int fat32_mount(void){
  F.ok=0; fcache_lba=0xFFFFFFFF;
  if(!ata_sectors()) return 0;
  if(rd(0,sec)) return 0;
  if(sec[510]!=0x55||sec[511]!=0xAA) return 0;
  u8 t=sec[446+4]; if(t!=0x0B&&t!=0x0C) return 0;
  u32 lba=*(u32*)(sec+446+8);
  if(rd(lba,sec)) return 0;
  if(sec[510]!=0x55||sec[511]!=0xAA||*(u16*)(sec+11)!=512||memcmp(sec+82,"FAT32",5)) return 0;
  F.lba=lba; F.spc=sec[13]; F.rsvd=*(u16*)(sec+14); F.fatsz=*(u32*)(sec+36); F.root=*(u32*)(sec+44);
  u32 tot=*(u32*)(sec+32); F.data=lba+F.rsvd+2*F.fatsz; F.nclus=(tot-F.rsvd-2*F.fatsz)/F.spc; F.nextfree=3; F.ok=1;
  return 1;
}

/* ---------- baca ---------- */
static void name_from(const u8*e,char*o){
  int n=0; for(int i=0;i<8&&e[i]!=' ';i++) o[n++]=e[i];
  if(e[8]!=' '){ o[n++]='.'; for(int i=8;i<11&&e[i]!=' ';i++) o[n++]=e[i]; } o[n]=0;
}
/* iterasi direktori; cb dipanggil per entri valid; return != 0 untuk berhenti */
typedef int (*dircb)(const u8*e,void*ud);
static void dir_iter(u32 clus,dircb cb,void*ud){
  while(clus>=2&&clus<0x0FFFFFF8){
    for(u32 s=0;s<F.spc;s++){ if(rd(c2l(clus)+s,sec)) return;
      for(int i=0;i<16;i++){ const u8*e=sec+i*32; if(e[0]==0) return; if(e[0]==0xE5||e[11]==0x0F||(e[11]&8)||e[0]=='.') continue;
        u8 copy[32]; memcpy(copy,e,32); if(cb(copy,ud)) return; if(rd(c2l(clus)+s,sec)) return; } }
    clus=fat_get(clus);
  }
}
typedef struct { const char*want; u32 clus,size; int isdir,found; } FindCtx;
static int find_cb(const u8*e,void*ud){ FindCtx*f=ud; char n[16]; name_from(e,n);
  if(!stricmp(n,f->want)){ f->found=1; f->isdir=(e[11]&0x10)!=0; f->clus=(*(u16*)(e+20)<<16)|*(u16*)(e+26); f->size=*(u32*)(e+28); return 1; } return 0; }
/* path "/A/B.EXT" -> cluster; return 1 ok */
static int resolve(const char*path,u32*clus,u32*size,int*isdir){
  u32 cur=F.root; *size=0; *isdir=1; char comp[16];
  while(*path){
    while(*path=='/') path++; if(!*path) break;
    int n=0; while(*path&&*path!='/'&&n<15) comp[n++]=*path++; comp[n]=0;
    FindCtx f={comp,0,0,0,0}; dir_iter(cur,find_cb,&f); if(!f.found) return 0;
    cur=f.clus?f.clus:F.root; *size=f.size; *isdir=f.isdir;
  }
  *clus=cur; return 1;
}
typedef struct { FsEntry*out; int n,max; } ListCtx;
static int list_cb(const u8*e,void*ud){ ListCtx*l=ud; if(l->n>=l->max) return 1; FsEntry*f=&l->out[l->n++];
  name_from(e,f->name); f->isdir=(e[11]&0x10)!=0; f->size=*(u32*)(e+28); return 0; }
int fat32_list(const char*path,FsEntry*out,int max){
  u32 c,sz; int d; if(!F.ok||!resolve(path,&c,&sz,&d)||!d) return -1;
  ListCtx l={out,0,max}; dir_iter(c,list_cb,&l); return l.n;
}
int fat32_read(const char*path,u8*buf,u32 max){
  u32 c,sz; int d; if(!F.ok||!resolve(path,&c,&sz,&d)||d) return -1;
  if(sz>max) sz=max; u32 done=0;
  while(done<sz&&c>=2&&c<0x0FFFFFF8){
    for(u32 s=0;s<F.spc&&done<sz;s++){ rd(c2l(c)+s,sec); u32 n=sz-done>512?512:sz-done; memcpy(buf+done,sec,n); done+=n; }
    c=fat_get(c);
  }
  return (int)sz;
}

/* ---------- tulis ---------- */
static void to83(const char*n,char*o){
  memset(o,' ',11); int i=0; while(*n&&*n!='.'&&i<8){ char c=*n++; o[i++]=(c>='a'&&c<='z')?c-32:c; }
  while(*n&&*n!='.') n++; if(*n=='.'){ n++; int j=8; while(*n&&j<11){ char c=*n++; o[j++]=(c>='a'&&c<='z')?c-32:c; } }
}
static int dir_add(u32 dir,const char*name,u8 attr,u32 first,u32 size){
  char n83[11]; to83(name,n83);
  while(dir>=2&&dir<0x0FFFFFF8){
    for(u32 s=0;s<F.spc;s++){ if(rd(c2l(dir)+s,sec)) return -1;
      for(int i=0;i<16;i++){ u8*e=sec+i*32; if(e[0]==0||e[0]==0xE5){
        memset(e,0,32); memcpy(e,n83,11); e[11]=attr; *(u16*)(e+20)=first>>16; *(u16*)(e+26)=first&0xFFFF;
        *(u16*)(e+16)=*(u16*)(e+24)=*(u16*)(e+18)=0x5D59; *(u32*)(e+28)=size; return wr(c2l(dir)+s,sec); } } }
    dir=fat_get(dir);
  }
  return -1;
}
static int dir_clus(const char*p,u32*c){ u32 sz; int d; if(!resolve(p,c,&sz,&d)||!d) return -1; return 0; }
int fat32_mkdir(const char*parent,const char*name){
  u32 pc; if(dir_clus(parent,&pc)) return -1;
  u32 c=alloc(); if(!c) return -1; zero_cluster(c);
  rd(c2l(c),sec); memset(sec,0,512);
  memset(sec,' ',11); sec[0]='.'; sec[11]=0x10; *(u16*)(sec+20)=c>>16; *(u16*)(sec+26)=c;
  memset(sec+32,' ',11); sec[32]='.'; sec[33]='.'; sec[32+11]=0x10; u32 pp=(pc==F.root)?0:pc; *(u16*)(sec+32+20)=pp>>16; *(u16*)(sec+32+26)=pp;
  wr(c2l(c),sec);
  return dir_add(pc,name,0x10,c,0);
}
int fat32_writefile(const char*dir,const char*name,const u8*data,u32 size){
  u32 pc; if(dir_clus(dir,&pc)) return -1;
  u32 csz=F.spc*512, n=(size+csz-1)/csz, first=0, prev=0, off=0;
  for(u32 i=0;i<n;i++){
    u32 c=alloc(); if(!c) return -1; if(prev) fat_set(prev,c); else first=c; prev=c;
    for(u32 s=0;s<F.spc;s++){ memset(sec,0,512); if(off<size){ u32 k=size-off>512?512:size-off; memcpy(sec,data+off,k); off+=k; } wr(c2l(c)+s,sec); }
  }
  return dir_add(pc,name,0x20,first,size);
}

/* ---------- format ---------- */
int fat32_mkfs(u32 lba,u32 total,void(*prog)(int)){
  u32 spc=0,fatsz=0,rsvd=32;
  for(u32 t=8;t>=1;t>>=1){ u32 fs=(((total/t)+2)*4+511)/512; u32 nc=(total-rsvd-2*fs)/t; if(nc>=65525){ spc=t; fatsz=fs; break; } }
  if(!spc) return -1;
  memset(sec,0,512);
  sec[0]=0xEB;sec[1]=0x58;sec[2]=0x90; memcpy(sec+3,"MYSAWIT ",8);
  *(u16*)(sec+11)=512; sec[13]=spc; *(u16*)(sec+14)=rsvd; sec[16]=2; sec[21]=0xF8;
  *(u16*)(sec+24)=63; *(u16*)(sec+26)=255; *(u32*)(sec+28)=lba; *(u32*)(sec+32)=total; *(u32*)(sec+36)=fatsz;
  *(u32*)(sec+44)=2; *(u16*)(sec+48)=1; *(u16*)(sec+50)=6; sec[64]=0x80; sec[66]=0x29; *(u32*)(sec+67)=0x5A17;
  memcpy(sec+71,"MYSAWIT OS ",11); memcpy(sec+82,"FAT32   ",8); sec[510]=0x55; sec[511]=0xAA;
  if(wr(lba,sec)||wr(lba+6,sec)) return -1;
  memset(sec,0,512); *(u32*)(sec)=0x41615252; *(u32*)(sec+484)=0x61417272; *(u32*)(sec+488)=0xFFFFFFFF; *(u32*)(sec+492)=0xFFFFFFFF; *(u32*)(sec+508)=0xAA550000;
  wr(lba+1,sec); wr(lba+7,sec);
  memset(sec,0,512); for(u32 i=2;i<rsvd;i++) if(i!=6&&i!=7) wr(lba+i,sec);
  u32 totfat=2*fatsz, done=0;
  while(done<totfat){ u32 n=totfat-done>16?16:totfat-done; if(ata_write(lba+rsvd+done,n,zero8k)) return -1; done+=n; if(prog) prog((int)((done*100)/totfat)); }
  memset(sec,0,512); ((u32*)sec)[0]=0x0FFFFFF8; ((u32*)sec)[1]=0x0FFFFFFF; ((u32*)sec)[2]=0x0FFFFFFF;
  wr(lba+rsvd,sec); wr(lba+rsvd+fatsz,sec);
  F.lba=lba; F.spc=spc; F.rsvd=rsvd; F.fatsz=fatsz; F.root=2; F.data=lba+rsvd+2*fatsz; F.nclus=(total-rsvd-2*fatsz)/spc; F.nextfree=3; F.ok=1; fcache_lba=0xFFFFFFFF;
  zero_cluster(2); return 0;
}
