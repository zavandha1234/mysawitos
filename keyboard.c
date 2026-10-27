/* keyboard.c - PS/2 keyboard scancode set 1 (polling) */
#include "mysawit.h"
static const char map[58]={0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']',10,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
static int q[16],qh,qt;
void kb_feed(u8 sc){ if(sc&0x80||sc>=58) return; char c=map[sc]; if(c){ q[qt]=c; qt=(qt+1)&15; } }
int kb_getkey(void){ if(qh==qt) return 0; int c=q[qh]; qh=(qh+1)&15; return c; }
