// Host test for the exact firmware persistence/validation core. No USB.
#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
static rs_record disk[RS_SLOTS][2];
static int cut=-1,calls;
static int save(unsigned slot,unsigned bank,const rs_record *r,void *ctx) {
    calls++;if(cut>=0){memset(&disk[slot][bank],0,sizeof(*r));memcpy(&disk[slot][bank],r,(size_t)cut);return -EIO;}
    disk[slot][bank]=*r;return 0;
}
static void boot(rs_store *s){rs_init(s);for(unsigned i=0;i<RS_SLOTS;i++)for(unsigned b=0;b<2;b++)rs_load(s,i,b,&disk[i][b]);}
static rs_record load(const char *p) {rs_record r;FILE *f=fopen(p,"rb");assert(f);assert(fread(&r,1,sizeof(r),f)==sizeof(r));assert(fgetc(f)==EOF);fclose(f);return r;}
int main(int argc,char **argv) {
    assert(argc==3);rs_record first=load(argv[1]),next=load(argv[2]);assert(rs_valid(&first,0)&&rs_valid(&next,0));
    rs_store s;rs_init(&s);assert(sizeof(rs_record)==168);assert(rs_valid(&s.records[0],1));
    assert(!rs_commit(&s,0,0,&first,save,NULL));assert(!memcmp(&s.records[0],&first,sizeof(first)));
    int previous_calls=calls;assert(rs_commit(&s,0,0,&next,save,NULL)==-ESTALE);assert(calls==previous_calls);
    rs_record good_disk[RS_SLOTS][2];memcpy(good_disk,disk,sizeof(disk));
    for(cut=0;cut<=RS_SIZE;cut++) {
        memcpy(disk,good_disk,sizeof(disk));boot(&s);
        assert(rs_commit(&s,0,1,&next,save,NULL)==-EIO);
        assert(rs_u32(s.records[0].b+4)==1);
        boot(&s);uint32_t generation=rs_u32(s.records[0].b+4);
        assert(generation==1||generation==2); // interrupted save yields old or entire new, never partial
        assert(rs_valid(&s.records[0],0));
        for(unsigned i=1;i<RS_SLOTS;i++)assert(!rs_u32(s.records[i].b+4));
    }
    cut=-1;memcpy(disk,good_disk,sizeof(disk));boot(&s);
    assert(!rs_commit(&s,0,1,&next,save,NULL));boot(&s);assert(!memcmp(&s.records[0],&next,sizeof(next)));
    for(unsigned i=0;i<RS_SIZE;i++){rs_record bad=next;bad.b[i]^=1;assert(!rs_valid(&bad,0));}
    for(unsigned layout=0;layout<2;layout++)for(unsigned c=32;c<=126;c++)printf("%u\n",rs_key(c,layout));
    return 0;
}
