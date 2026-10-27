#!/usr/bin/env python3
"""Buat .exe PE32 minimal untuk My Sawit OS (tanpa compiler Windows).
   Kode: api->msgbox(title,text). Pakai: make_exe.py out.exe "Judul" "Teks" """
import struct,sys
def make(path,title,text):
    BASE=0x02000000; RVA=0x1000
    t=title.encode()+b'\0'; x=text.encode()+b'\0'
    code_len=4+5+5+3+3+2+1
    t_addr=BASE+RVA+code_len; x_addr=t_addr+len(t)
    code=bytes([0x8B,0x44,0x24,0x04])            # mov eax,[esp+4]
    code+=b'\x68'+struct.pack('<I',x_addr)       # push text
    code+=b'\x68'+struct.pack('<I',t_addr)       # push title
    code+=bytes([0xFF,0x50,0x04])                # call [eax+4]  (api->msgbox)
    code+=bytes([0x83,0xC4,0x08])                # add esp,8
    code+=bytes([0x31,0xC0,0xC3])                # xor eax,eax ; ret
    assert len(code)==code_len
    sect=(code+t+x).ljust(0x200,b'\0')
    dos=bytearray(64); dos[0:2]=b'MZ'; struct.pack_into('<I',dos,0x3C,0x40)
    fh=struct.pack('<HHIIIHH',0x14C,1,0,0,0,0xE0,0x0102)
    oh=struct.pack('<HBBIIIIIIIIIHHHHHHIIIIHHIIIIII',0x10B,1,0,0x200,0,0,RVA,RVA,RVA,BASE,0x1000,0x200,4,0,0,0,4,0,0,0x2000,0x200,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
    oh+=b'\0'*(16*8)
    assert len(oh)==0xE0,len(oh)
    sh=b'.text\0\0\0'+struct.pack('<IIIIIIHHI',len(code+t+x),RVA,0x200,0x200,0,0,0,0,0x60000020)
    hdr=bytes(dos)+b'PE\0\0'+fh+oh+sh
    open(path,'wb').write(hdr.ljust(0x200,b'\0')+sect)
if __name__=='__main__': make(*sys.argv[1:4])
