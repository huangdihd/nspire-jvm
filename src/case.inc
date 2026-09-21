/*
 * Copyright (c) 1999, 2016, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/*
 *
 * (C) Copyright Taligent, Inc. 1996, 1997 - All Rights Reserved
 * (C) Copyright IBM Corp. 1996 - 2002 - All Rights Reserved
 *
 * The original version of this source code and documentation
 * is copyrighted and owned by Taligent, Inc., a wholly-owned
 * subsidiary of IBM. These materials are provided under terms
 * of a License Agreement between Taligent and Sun. This technology
 * is protected by multiple US and International patents.
 *
 * This notice and attribution to Taligent may not be removed.
 * Taligent is a registered trademark of Taligent, Inc.
 */

/* Unicode case conversion and simple case-insensitive comparison.
 * The word-boundary state-machine traversal is adapted from OpenJDK 17
 * RuleBasedBreakIterator: Copyright (c) 1999, 2016, Oracle and/or its affiliates.
 * This file and the generated tables are GPL-2.0-only WITH Classpath-exception-2.0.
 * See vendor/openjdk17-casing/LICENSE, NOTICE and the preserved upstream source.
 * Other code in this file: Copyright (c) 2026 Nspire JVM contributors.
 */
typedef struct { uint32_t cp,lower,upper,title; } CaseMap;
typedef struct { uint32_t cp;unsigned upper,length;uint16_t units[3]; } CaseExpansion;
typedef struct { uint32_t first,last;int value; } CaseRange;
#include "../vendor/openjdk17-casing/data.inc"

static unsigned case_point(const unsigned char **p) {
    unsigned ch=utf_unit(p);
    if(ch>=0xd800&&ch<=0xdbff&&**p) {
        const unsigned char *q=*p;unsigned low=utf_unit(&q);
        if(low>=0xdc00&&low<=0xdfff){ch=0x10000+((ch-0xd800)<<10)+low-0xdc00;*p=q;}
    }
    return ch;
}
static size_t case_write(char *p,unsigned ch) {
    if(ch<=0xffff)return write_unit(p,ch);
    ch-=0x10000;size_t n=write_unit(p,0xd800+(ch>>10));return n+write_unit(p+n,0xdc00+(ch&1023));
}
static unsigned case_simple(unsigned cp,int direction) {
    size_t first=0,last=sizeof case_maps/sizeof *case_maps;
    while(first<last){size_t mid=first+(last-first)/2;const CaseMap *map=&case_maps[mid];
        if(cp<map->cp)last=mid;else if(cp>map->cp)first=mid+1;else return direction==0?map->lower:direction==1?map->upper:map->title;}
    return cp;
}
static int case_range(unsigned cp,const CaseRange *table,size_t count) {
    size_t first=0;while(first<count){size_t mid=first+(count-first)/2;
        if(cp<table[mid].first)count=mid;else if(cp>table[mid].last)first=mid+1;else return table[mid].value;}
    return 0;
}
static unsigned case_flags(unsigned cp) {return (unsigned)case_range(cp,case_properties,sizeof case_properties/sizeof *case_properties);}
static const CaseExpansion *case_expansion(unsigned cp,int upper) {
    size_t first=0,last=sizeof case_expansions/sizeof *case_expansions;uint32_t key=cp*2+(unsigned)upper;
    while(first<last){size_t mid=first+(last-first)/2;const CaseExpansion *e=&case_expansions[mid];uint32_t actual=e->cp*2+e->upper;
        if(key<actual)last=mid;else if(key>actual)first=mid+1;else return e;}
    return NULL;
}
/* Match JDK 17's UTF-16 isBoundary/following traversal, including starts on
 * surrogate halves. A list of forward boundaries is not equivalent to this API. */
typedef struct { const uint16_t *text;int size,pos,cached; } CaseWord;
static unsigned case_word_current(CaseWord *w) {
    if(w->pos==w->size)return 0xffff;
    unsigned ch=w->text[w->pos];
    if(ch>=0xd800&&ch<=0xdbff&&w->pos+1<w->size){unsigned low=w->text[w->pos+1];
        if(low>=0xdc00&&low<=0xdfff)return 0x10000+((ch-0xd800)<<10)+low-0xdc00;}
    return ch;
}
static int case_word_next_index(CaseWord *w) {int n=w->pos+(case_word_current(w)>0xffff?2:1);return n<w->size?n:w->size;}
static unsigned case_word_advance(CaseWord *w) {int n=case_word_next_index(w);if(n==w->size)return 0xffff;w->pos=n;return case_word_current(w);}
static unsigned case_word_retreat(CaseWord *w) {
    if(!w->pos)return 0xffff;unsigned low=w->text[--w->pos];
    if(low>=0xdc00&&low<=0xdfff&&w->pos){unsigned high=w->text[w->pos-1];
        if(high>=0xd800&&high<=0xdbff){w->pos--;return 0x10000+((high-0xd800)<<10)+low-0xdc00;}}
    return low;
}
static int case_word_category(unsigned cp) {return case_range(cp,case_word_categories,sizeof case_word_categories/sizeof *case_word_categories);}
static int case_word_next(CaseWord *w) {
    if(w->pos==w->size)return -1;
    int result=case_word_next_index(w),look=0;unsigned state=1,cp=case_word_current(w);
    while(cp!=0xffff&&state) {
        int category=case_word_category(cp);if(category!=-1)state=case_word_states[state*case_word_columns+(unsigned)category];
        if(case_word_lookahead[state]){if(case_word_ends[state])result=look;else look=case_word_next_index(w);}
        else if(case_word_ends[state])result=case_word_next_index(w);
        cp=case_word_advance(w);
    }
    if(cp==0xffff&&look==w->size)result=look;
    w->pos=result;return result;
}
static int case_word_previous(CaseWord *w) {
    unsigned state=1,cp=case_word_current(w);int category=0,last=0;
    while(cp!=0xffff&&state) {
        last=category;category=case_word_category(cp);
        if(category!=-1)state=case_word_backwards[state*case_word_columns+(unsigned)category];
        cp=case_word_retreat(w);
    }
    if(cp!=0xffff){if(last!=-1)case_word_advance(w);case_word_advance(w);}
    return w->pos;
}
static int case_word_boundary(CaseWord *w,int offset) {
    if(!offset)return 1;int from=offset-1;w->pos=from;
    if(!from){w->cached=case_word_next(w);return w->cached==offset;}
    int result=w->cached;
    if(result>=from||result<=-1)result=case_word_previous(w);else w->pos=result;
    while(result!=-1&&result<=from)result=case_word_next(w);
    w->cached=result;return result==offset;
}
static void case_final_sigmas(VM *v,const uint32_t *points,size_t count,unsigned char *final) {
    uint16_t *units=(uint16_t *)alloc(v,count*2*sizeof *units);int *offsets=(int *)alloc(v,(count+1)*sizeof *offsets);int length=0;
    for(size_t i=0;i<count;i++){unsigned cp=points[i];offsets[i]=length;
        if(cp>0xffff){cp-=0x10000;units[length++]=(uint16_t)(0xd800+(cp>>10));units[length++]=(uint16_t)(0xdc00+(cp&1023));}
        else units[length++]=(uint16_t)cp;}
    offsets[count]=length;
    for(size_t at=0;at<count;at++)if(points[at]==0x3a3) {
        CaseWord word={units,length,0,-1};int before=0;
        for(size_t i=at;i&&!case_word_boundary(&word,offsets[i]);i--)if(case_flags(points[i-1])&1){before=1;break;}
        if(before){final[at]=1;
            for(size_t i=at+1;i<count&&!case_word_boundary(&word,offsets[i]);i++)if(case_flags(points[i])&1){final[at]=0;break;}}
    }
    release(v,offsets);release(v,units);
}
static int case_after(const uint32_t *points,size_t at,int soft) {
    while(at){unsigned cp=points[--at],flags=case_flags(cp);
        if(soft?(flags&2)!=0:cp=='I')return 1;
        if(!(flags&8))return 0;}
    return 0;
}
static int case_before(const uint32_t *points,size_t at,size_t count,int dot) {
    while(++at<count){unsigned cp=points[at],flags=case_flags(cp);
        if(dot?cp==0x307:(flags&4)!=0)return 1;
        if(!(flags&8))return 0;}
    return 0;
}
static int case_language(VM *v,Object *locale,int use_default) {
    if(use_default) {
        Class *c=load(v,"java/util/Locale");initialize(v,c);if(v->exception)return -1;
        Method *m=method(c,"getDefault","()Ljava/util/Locale;");if(!m)fail(v,"Locale.getDefault is not implemented");
        locale=obj(execute(v,m,NULL,0));if(v->exception)return -1;
    }
    if(!locale){throwing(v,"java/lang/NullPointerException");return -1;}
    root(v,locale);Method *m=method(locale->cls,"getLanguage","()Ljava/lang/String;");if(!m)fail(v,"Locale.getLanguage is not implemented");
    Value arg=rv(locale),lang=execute(v,m,&arg,1);v->nr--;if(v->exception)return -1;
    const char *s=obj(lang)->text;
    if(!strcmp(s,"tr")||!strcmp(s,"az"))return 1;
    if(!strcmp(s,"lt"))return 2;
    /* Thai uses a dictionary word iterator: do not silently substitute ROOT. */
    if(!strcmp(s,"th"))return 3;
    return 0;
}
static Object *case_string(VM *v,Object *source,Object *locale,int use_default,int upper) {
    int language=case_language(v,locale,use_default);if(v->exception)return NULL;
    const unsigned char *text=(const unsigned char *)source->text,*p=text;size_t count=0;int sigma=0;
    while(*p){unsigned cp=case_point(&p);count++;if(cp==0x3a3)sigma=1;}
    if(!count)return source;
    if(count>(META_LIMIT-1)/9)fail(v,"case conversion exceeds metadata limit");
    uint32_t *points=(uint32_t *)alloc(v,count*sizeof *points);p=text;
    for(size_t i=0;i<count;i++)points[i]=case_point(&p);
    unsigned char *final=NULL;
    if(!upper&&sigma) {
        if(language==3)fail(v,"Thai dictionary word boundaries for final sigma are not implemented");
        final=(unsigned char *)alloc(v,count);case_final_sigmas(v,points,count,final);
    }
    char *result=(char *)alloc(v,count*9+1),*end=result;
    for(size_t i=0;i<count;i++) {
        unsigned cp=points[i],mapped=case_simple(cp,upper),units[3];size_t length=0;int custom=0;
        if(language==1) {
            if(upper&&cp=='i'){mapped=0x130;custom=1;}
            if(!upper&&cp==0x130){mapped='i';custom=1;}
            if(!upper&&cp=='I'&&!case_before(points,i,count,1)){mapped=0x131;custom=1;}
            if(!upper&&cp==0x307&&case_after(points,i,0))continue;
        } else if(language==2) {
            if(upper&&cp==0x307&&case_after(points,i,1))continue;
            if(!upper&&(cp=='I'||cp=='J'||cp==0x12e)&&case_before(points,i,count,0)){units[0]=mapped;units[1]=0x307;length=2;}
            if(!upper&&(cp==0xcc||cp==0xcd||cp==0x128)){units[0]='i';units[1]=0x307;units[2]=cp==0xcc?0x300:cp==0xcd?0x301:0x303;length=3;}
        }
        if(!upper&&cp==0x3a3&&final[i]){mapped=0x3c2;custom=1;}
        const CaseExpansion *expansion=!custom&&!length?case_expansion(cp,upper):NULL;
        if(expansion){length=expansion->length;for(size_t j=0;j<length;j++)units[j]=expansion->units[j];}
        if(length)for(size_t j=0;j<length;j++)end+=write_unit(end,units[j]);else end+=case_write(end,mapped);
    }
    *end=0;Object *out=!strcmp(result,source->text)?source:string(v,result);
    release(v,result);release(v,final);release(v,points);return out;
}
static int case_compare(const unsigned char *p,const unsigned char *q) {
    int countp=0,countq=0;const unsigned char *a=p,*b=q;
    while(*a){utf_unit(&a);countp++;}while(*b){utf_unit(&b);countq++;}
    while(*p&&*q){unsigned x=case_point(&p),y=case_point(&q);
        if(x==y)continue;x=case_simple(x,1);y=case_simple(y,1);if(x==y)continue;
        x=case_simple(x,0);y=case_simple(y,0);if(x!=y)return (int)x-(int)y;}
    return countp-countq;
}
