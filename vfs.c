/* vfs.c - FAT32 di HDD jika ada, kalau tidak (live ISO) pakai file modul GRUB sebagai /APPS */
#include "mysawit.h"
static int is_apps(const char*p){ while(*p=='/')p++; return !stricmp(p,"APPS")||!stricmp(p,"APPS/"); }
static int is_root(const char*p){ while(*p=='/')p++; return *p==0; }
int vfs_list(const char*path,FsEntry*out,int max){
  if(fat32_ready()) return fat32_list(path,out,max);
  int n=0;
  if(is_root(path)){ strcpy(out[0].name,"APPS"); out[0].isdir=1; out[0].size=0; return 1; }
  if(is_apps(path)){ for(int i=0;i<nmods&&n<max;i++){ if(!stricmp(mods[i].name,"mysawit.bin")) continue;
      strncpy0(out[n].name,mods[i].name,15); for(char*c=out[n].name;*c;c++) if(*c>='a'&&*c<='z')*c-=32; out[n].isdir=0; out[n].size=mods[i].size; n++; } return n; }
  return -1;
}
int vfs_read(const char*path,u8*buf,u32 max){
  if(fat32_ready()) return fat32_read(path,buf,max);
  const char*b=path; for(const char*p=path;*p;p++) if(*p=='/') b=p+1;
  Mod*m=mod_find(b); if(!m) return -1; u32 n=m->size>max?max:m->size; memcpy(buf,m->data,n); return (int)n;
}
