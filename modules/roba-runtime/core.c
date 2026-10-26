// SPDX-License-Identifier: MIT
#include "core.h"
#include <string.h>
#include <errno.h>
#include "keycodes.h"
uint32_t rs_u32(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
uint16_t rs_u16(const uint8_t *p) { return (uint16_t)p[0]|(uint16_t)p[1]<<8; }
uint32_t rs_crc(const uint8_t *p, size_t n) {
    uint32_t c=~0u;
    while(n--) { c^=*p++; for(unsigned i=0;i<8;i++) c=(c>>1)^(0xedb88320u & (0u-(c&1))); }
    return ~c;
}
static void put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
void rs_empty(rs_record *r) { memset(r,0,sizeof(*r));memcpy(r->b,"RSM1",4);r->b[31]=1;put32(r->b+164,rs_crc(r->b,164)); }
void rs_init(rs_store *s) { memset(s,0,sizeof(*s));for(unsigned i=0;i<RS_SLOTS;i++)rs_empty(&s->records[i]); }
static int all_zero(const uint8_t *p,size_t n) { while(n--)if(*p++)return 0;return 1; }
static int utf8_name(const uint8_t *p,size_t n) {
    size_t i=0;
    while(i<n) {
        uint8_t c=p[i++];if(c<32||c==127)return 0;
        if(c<128)continue;
        unsigned left;uint32_t cp,min;
        if(c>=0xc2&&c<=0xdf){left=1;cp=c&31;min=128;}
        else if(c>=0xe0&&c<=0xef){left=2;cp=c&15;min=2048;}
        else if(c>=0xf0&&c<=0xf4){left=3;cp=c&7;min=65536;}
        else return 0;
        if(i+left>n)return 0;
        while(left--){c=p[i++];if((c&0xc0)!=0x80)return 0;cp=(cp<<6)|(c&63);}
        if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return 0;
    }
    return 1;
}
int rs_valid(const rs_record *r,int allow_empty) {
    const uint8_t *b=r->b;
    if(memcmp(b,"RSM1",4)||rs_u32(b+164)!=rs_crc(b,164))return 0;
    if(!rs_u32(b+4)) { rs_record empty;rs_empty(&empty);return allow_empty&&!memcmp(r,&empty,sizeof(empty)); }
    if(!rs_u32(b+8)||rs_u32(b+8)>0x7fffffff||all_zero(b+12,16)||b[28]>1||!b[29]||b[29]>63||!b[30]||b[30]>RS_TEXT||b[31]>1)return 0;
    unsigned wait=rs_u16(b+32),tap=rs_u16(b+34);
    if(wait>1000||!tap||tap>1000||b[30]*(wait+tap)>30000)return 0;
    if(!utf8_name(b+100,b[29])||!all_zero(b+100+b[29],64-b[29])||!all_zero(b+36+b[30],64-b[30]))return 0;
    for(unsigned i=0;i<b[30];i++)if(b[36+i]<32||b[36+i]>126)return 0;
    return 1;
}
int rs_load(rs_store *s,unsigned slot,unsigned bank,const rs_record *r) {
    if(slot>=RS_SLOTS||bank>1||!rs_valid(r,0))return -EINVAL;
    uint32_t gen=rs_u32(r->b+4),old=rs_u32(s->records[slot].b+4);
    if(gen>old){s->records[slot]=*r;s->banks[slot]=bank;}
    else if(gen==old&&memcmp(r,&s->records[slot],sizeof(*r)))return -EUCLEAN;
    return 0;
}
int rs_commit(rs_store *s,unsigned slot,uint32_t expected,const rs_record *next,rs_persist persist,void *ctx) {
    if(slot>=RS_SLOTS||!rs_valid(next,0))return -EINVAL;
    const rs_record *old=&s->records[slot];
    if(rs_u32(old->b+4)!=expected)return -ESTALE;
    if(expected==0xffffffff||rs_u32(next->b+4)!=expected+1)return -EINVAL;
    int same=!memcmp(old->b+12,next->b+12,16);
    if(expected&&same&&rs_u32(old->b+8)!=rs_u32(next->b+8))return -EINVAL;
    if(expected&&!same&&(!old->b[31]||rs_u32(old->b+8)==rs_u32(next->b+8)))return -EBUSY;
    for(unsigned i=0;i<RS_SLOTS;i++)if(i!=slot&&rs_u32(s->records[i].b+4)&&!memcmp(s->records[i].b+12,next->b+12,16))return -EEXIST;
    unsigned bank=1-s->banks[slot];
    int rc=persist(slot,bank,next,ctx);if(rc)return rc;
    s->records[slot]=*next;s->banks[slot]=bank;return 0;
}
uint32_t rs_key(uint8_t ascii,uint8_t layout) { return ascii>=32&&ascii<=126&&layout<2?rs_codes[layout][ascii-32]:0; }
