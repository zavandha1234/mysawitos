/* installer.c - Installer mirip Windows 11 (biru #0078D4, kotak putih rounded).
   Language -> License -> Disk -> Copying files -> Finish. Cursor Windows tetap tampil.
   Tulis: MBR (boot.img GRUB) + core.img GRUB di sektor 1.. + FAT32 + /BOOT/MYSAWIT.BIN + grub.cfg + /APPS */
#include "mysawit.h"
extern const u8 _binary_boot_img_start[],_binary_boot_img_end[],_binary_core_img_start[],_binary_core_img_end[];
#define BX 100
#define BY 50
#define BW 440
#define BH 380
static int page=0, accepted=0, pct=0, failed=0; static const char*status="";
static const char*grubcfg=
 "set timeout=0\nset default=0\nmenuentry \"My Sawit OS v1.0\" {\n  multiboot /boot/mysawit.bin\n  module /boot/mysawit.bin /boot/mysawit.bin\n  boot\n}\n";
static int inr(int x,int y,int rx,int ry,int rw,int rh){ return x>=rx&&y>=ry&&x<rx+rw&&y<ry+rh; }
static void btn(int x,int y,int w,const char*l,int primary,int enabled){
  int hv=enabled&&inr(mouse_x,mouse_y,x,y,w,30);
  u32 bg=!enabled?0xD0D0D0:primary?(hv?0x106EBE:0x0078D4):(hv?0xE5E5E5:0xF3F3F3);
  fill_round(x,y,w,30,4,bg); draw_text(x+(w-text_width(l,1))/2,y+11,l,!enabled?0x888888:primary?0xFFFFFF:0x000000,1);
}
static void render(void){
  fill_rect(0,0,SCREEN_W,SCREEN_H,0x0078D4);
  fill_round(BX+4,BY+4,BW,BH,10,0x0A5CA0); fill_round(BX,BY,BW,BH,10,0xFFFFFF);
  draw_logo_size(BX+20,BY+14,40); draw_text(BX+72,BY+20,"My Sawit OS Setup",0x000000,2);
  char st[24]; strcpy(st,TR("Step ","Langkah ")); u2s(page+1,st+strlen(st)); strcpy(st+strlen(st)," / 5"); draw_text(BX+72,BY+42,st,0x707070,1);
  fill_rect(BX+20,BY+62,BW-40,1,0xE0E0E0);
  int cx=BX+24,cy=BY+76;
  if(page==0){
    draw_text(cx,cy,TR("Select your language","Pilih bahasa Anda"),0x000000,1);
    const char*l[2]={"English","Bahasa Indonesia"};
    for(int i=0;i<2;i++){ int y=cy+24+i*34; fill_round(cx,y,BW-48,28,4,lang==i?0xCCE8FF:0xF3F3F3);
      draw_rect(cx+10,y+8,12,12,0x404040); if(lang==i) fill_rect(cx+13,y+11,6,6,0x0078D4); draw_text(cx+34,y+10,l[i],0x000000,1); }
  } else if(page==1){
    draw_text(cx,cy,TR("License terms","Syarat lisensi"),0x000000,1);
    fill_rect(cx,cy+20,BW-48,150,0xFAFAFA); draw_rect(cx,cy+20,BW-48,150,0xA0A0A0);
    const char*L[]={"MY SAWIT OS v1.0 - LICENSE","","My Sawit OS is provided by My Sawit","Studio AS IS, without warranty.","You may use, copy and share it","freely for personal and commercial","use. The sawit logo remains the","property of My Sawit Studio.","","Terima kasih telah memakai produk","buatan Indonesia."};
    for(int i=0;i<11;i++) draw_text(cx+8,cy+28+i*13,L[i],0x202020,1);
    draw_rect(cx,cy+184,14,14,0x404040); if(accepted){ draw_line(cx+3,cy+191,cx+6,cy+195,0x0078D4); draw_line(cx+6,cy+195,cx+11,cy+187,0x0078D4); draw_line(cx+3,cy+192,cx+6,cy+196,0x0078D4); draw_line(cx+6,cy+196,cx+11,cy+188,0x0078D4); }
    draw_text(cx+22,cy+188,TR("I accept the license terms","Saya setuju dengan syarat lisensi"),0x000000,1);
  } else if(page==2){
    draw_text(cx,cy,TR("Where do you want to install My Sawit OS?","Di mana My Sawit OS akan dipasang?"),0x000000,1);
    u32 mb=ata_sectors()/2048; fill_round(cx,cy+30,BW-48,50,6,mb?0xCCE8FF:0xFDE7E9);
    if(mb){ char s[48]; strcpy(s,"Disk 0 (IDE) - "); u2s(mb,s+15); strcpy(s+strlen(s)," MB"); draw_text(cx+14,cy+44,s,0x000000,1); draw_text(cx+14,cy+58,TR("Unallocated - will be formatted FAT32","Akan diformat FAT32"),0x404040,1);
      draw_text(cx,cy+100,TR("WARNING: all data on this disk will be erased!","PERINGATAN: semua data di disk ini akan dihapus!"),0xC42B1C,1);
      if(mb<40) draw_text(cx,cy+118,TR("Disk too small (min 40 MB).","Disk terlalu kecil (min 40 MB)."),0xC42B1C,1);
    } else { draw_text(cx+14,cy+44,TR("No hard disk found.","Tidak ada hard disk."),0xC42B1C,1); draw_text(cx,cy+100,TR("Attach an IDE disk (primary master) in VirtualBox.","Pasang disk IDE (primary master) di VirtualBox."),0x404040,1); }
  } else if(page==3){
    draw_text(cx,cy,TR("Installing My Sawit OS...","Memasang My Sawit OS..."),0x000000,1);
    fill_round(cx,cy+60,BW-48,18,4,0xE0E0E0); if(pct>0) fill_round(cx,cy+60,(BW-48)*pct/100<8?8:(BW-48)*pct/100,18,4,0x0078D4);
    char s[8]; u2s(pct,s); strcpy(s+strlen(s),"%"); draw_text_c(BX+BW/2,cy+90,s,0x000000,2);
    draw_text(cx,cy+130,status,0x404040,1); draw_text(cx,cy+170,TR("Do not turn off the computer.","Jangan matikan komputer."),0x707070,1);
  } else {
    draw_logo_size(BX+BW/2-40,cy,80);
    if(!failed){ draw_text_c(BX+BW/2,cy+100,TR("Installation complete!","Instalasi selesai!"),0x1B5E20,2);
      draw_text_c(BX+BW/2,cy+130,TR("Remove the ISO from the virtual drive,","Keluarkan ISO dari drive virtual,"),0x303030,1);
      draw_text_c(BX+BW/2,cy+144,TR("then restart to boot My Sawit OS.","lalu restart untuk masuk My Sawit OS."),0x303030,1);
    } else { draw_text_c(BX+BW/2,cy+100,TR("Installation failed","Instalasi gagal"),0xC42B1C,2); draw_text_c(BX+BW/2,cy+130,status,0x303030,1); }
  }
  int by=BY+BH-46;
  btn(BX+20,by,90,TR("Cancel","Batal"),0,page!=3&&page!=4);
  btn(BX+BW-210,by,90,TR("Back","Kembali"),0,page>0&&page<3);
  const char*nl=page==2?TR("Install","Pasang"):page==4?TR("Restart","Restart"):TR("Next","Lanjut");
  btn(BX+BW-110,by,90,nl,1,page!=3 && !(page==1&&!accepted) && !(page==2&&ata_sectors()/2048<40));
}
static void present(void){ render(); vga_present(); cursor_after_present(); }
static void progress(int p,const char*s){ pct=p; status=s; ui_pump(); present(); }
static void fmt_prog(int p){ static int last=-10; if(p-last>=8||p==100){ last=p; progress(8+p*55/100,TR("Formatting disk (FAT32)...","Memformat disk (FAT32)...")); } }
static int do_install(void){
  u32 total=ata_sectors(); u32 pstart=2048, psize=total-pstart;
  progress(2,TR("Writing boot sector (GRUB)...","Menulis boot sector (GRUB)..."));
  u8 mbr[512]; memcpy(mbr,_binary_boot_img_start,512);
  *(u32*)(mbr+0x5C)=1; *(u32*)(mbr+0x60)=0; mbr[0x64]=0xFF;       /* core.img di sektor 1 */
  memset(mbr+446,0,64); u8*pt=mbr+446; pt[0]=0x80; pt[1]=0x20;pt[2]=0x21;pt[3]=0; pt[4]=0x0C; pt[5]=0xFE;pt[6]=0xFF;pt[7]=0xFF;
  *(u32*)(pt+8)=pstart; *(u32*)(pt+12)=psize; mbr[510]=0x55; mbr[511]=0xAA;
  u32 csz=(u32)(_binary_core_img_end-_binary_core_img_start), cs=(csz+511)/512;
  if(cs>2040) { status="core.img terlalu besar"; return -1; }
  static u8 core[2048*512]; memset(core,0,cs*512); memcpy(core,_binary_core_img_start,csz);
  u8*bl=core+512-12; *(u32*)(bl)=2; *(u32*)(bl+4)=0; *(u16*)(bl+8)=cs-1; *(u16*)(bl+10)=0x820;   /* blocklist diskboot */
  if(ata_write(1,cs,core)||ata_write(0,1,mbr)){ status="Gagal menulis disk"; return -1; }
  progress(8,TR("Formatting disk (FAT32)...","Memformat disk (FAT32)..."));
  if(fat32_mkfs(pstart,psize,fmt_prog)){ status="Gagal format FAT32"; return -1; }
  progress(66,TR("Creating folders...","Membuat folder..."));
  if(fat32_mkdir("/","BOOT")||fat32_mkdir("/BOOT","GRUB")||fat32_mkdir("/","APPS")){ status="Gagal membuat folder"; return -1; }
  progress(72,TR("Copying kernel...","Menyalin kernel..."));
  Mod*k=mod_find("mysawit.bin"); if(!k){ status="Kernel module tidak ada (jalankan dari ISO)"; return -1; }
  if(fat32_writefile("/BOOT","MYSAWIT.BIN",k->data,k->size)){ status="Gagal menyalin kernel"; return -1; }
  progress(84,TR("Writing boot configuration...","Menulis konfigurasi boot..."));
  if(fat32_writefile("/BOOT/GRUB","GRUB.CFG",(const u8*)grubcfg,strlen(grubcfg))){ status="Gagal menulis grub.cfg"; return -1; }
  int n=0; for(int i=0;i<nmods;i++) if(stricmp(mods[i].name,"mysawit.bin")) n++;
  int d=0; for(int i=0;i<nmods;i++){ if(!stricmp(mods[i].name,"mysawit.bin")) continue;
    progress(88+(d*10)/(n?n:1),TR("Copying applications...","Menyalin aplikasi...")); if(fat32_writefile("/APPS",mods[i].name,mods[i].data,mods[i].size)){ status="Gagal menyalin aplikasi"; return -1; } d++; }
  progress(100,TR("Finishing...","Menyelesaikan...")); return 0;
}
int installer_run(void){
  page=0; accepted=0; failed=0; pct=0; int pb=0; present();
  for(;;){
    int moved=mouse_poll(); int k;
    while((k=kb_getkey())) if(k==KEY_ESC&&page<3) return 0;
    if(moved){ draw_cursor(mouse_x,mouse_y); int nb=mouse_btn&1; int mx=mouse_x,my=mouse_y;
      /* cursor: I-beam di kotak lisensi, panah selain itu */
      set_cursor_type(page==1&&inr(mx,my,BX+24,BY+96,BW-48,150)?CURSOR_TEXT:CURSOR_ARROW);
      if(nb&&!pb){ int by=BY+BH-46; int cx=BX+24,cy=BY+76;
        if(page!=3&&page!=4&&inr(mx,my,BX+20,by,90,30)) return 0;
        if(page==0){ for(int i=0;i<2;i++) if(inr(mx,my,cx,cy+24+i*34,BW-48,28)) lang=i; }
        if(page==1&&inr(mx,my,cx,cy+180,300,22)) accepted=!accepted;
        if(page>0&&page<3&&inr(mx,my,BX+BW-210,by,90,30)) page--;
        if(inr(mx,my,BX+BW-110,by,90,30)){
          if(page==4) reboot();
          else if(page==1&&!accepted){} else if(page==2&&ata_sectors()/2048<40){}
          else if(page==2){ page=3; pct=0; status=""; set_cursor_type(CURSOR_LOADING); present();
            failed=do_install()?1:0; if(!failed){ fat32_mount(); } set_cursor_type(CURSOR_ARROW); page=4; }
          else page++; }
        present(); }
      else if(nb!=pb||moved) present();
      pb=nb; }
    cursor_tick(millis());
  }
}
