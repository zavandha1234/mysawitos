/* pe.c - PE32 loader sederhana: map section ke ImageBase, tanpa import/relocation.
   Entry dipanggil cdecl: int entry(const SawitAPI*). Program berjalan di ring 0 (OS sederhana). */
#include "mysawit.h"
int pe_run(const u8*img,u32 size,const SawitAPI*api,const char**err){
  if(size<0x100||img[0]!='M'||img[1]!='Z'){ *err="Bukan file MZ/PE"; return -1; }
  u32 pe=*(u32*)(img+0x3C);
  if(pe+0x18>size||memcmp(img+pe,"PE\0\0",4)){ *err="Signature PE salah"; return -1; }
  const u8*fh=img+pe+4; u16 machine=*(u16*)fh, ns=*(u16*)(fh+2), osz=*(u16*)(fh+16);
  const u8*oh=fh+20;
  if(machine!=0x14C||*(u16*)oh!=0x10B){ *err="Harus PE32 x86"; return -1; }
  u32 entry=*(u32*)(oh+16), base=*(u32*)(oh+28), simg=*(u32*)(oh+56), shdr=*(u32*)(oh+60);
  u32 impsz=*(u32*)(oh+96+8+4);                      /* DataDirectory[1].Size */
  if(impsz){ *err="Import tidak didukung (gunakan SawitAPI)"; return -1; }
  if(base<0x01000000||simg==0||base+simg>mem_end){ *err="ImageBase di luar RAM"; return -1; }
  memset((void*)base,0,simg);
  memcpy((void*)base,img,shdr<size?shdr:size);
  const u8*sh=oh+osz;
  for(int i=0;i<ns;i++,sh+=40){
    u32 va=*(u32*)(sh+12), rs=*(u32*)(sh+16), rp=*(u32*)(sh+20);
    if(rp+rs>size||va+rs>simg){ *err="Section rusak"; return -1; }
    memcpy((void*)(base+va),img+rp,rs);
  }
  int (*ep)(const SawitAPI*)=(int(*)(const SawitAPI*))(base+entry);
  ep(api); return 0;
}
