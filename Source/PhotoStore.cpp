#include "PhotoStore.h"
#include <set>
using namespace juce;
PhotoStore::Snapshot PhotoStore::snapshot() const { std::lock_guard<std::mutex> l(mutex); return {photos, revision, totalBytes}; }
bool PhotoStore::imageDimensions(const void* src, size_t n, int& w, int& h) {
    auto p = static_cast<const unsigned char*>(src);
    if (!p || n < 24) return false;
    if (p[0]==137 && p[1]=='P' && p[2]=='N' && p[3]=='G' && p[4]==13 && p[5]==10 && p[6]==26 && p[7]==10 && std::memcmp(p+12,"IHDR",4)==0) {
        auto u32 = [](const unsigned char* b) { return (uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3]; };
        auto x=u32(p+16), y=u32(p+20); if(x>12000 || y>12000) return false; w=int(x); h=int(y);
    } else if(p[0]==255 && p[1]==216) {
        w=h=0; size_t i=2;
        while(i+3<n) {
            if(p[i++]!=255) return false;
            while(i<n && p[i]==255) ++i;
            if(i>=n) return false;
            auto marker=p[i++];
            if(marker==0xda || marker==0xd9) break;
            if(marker==0x01 || (marker>=0xd0 && marker<=0xd7)) continue;
            if(i+2>n) return false;
            size_t len=(size_t(p[i])<<8)|p[i+1]; if(len<2 || len>n-i) return false;
            if((marker>=0xc0 && marker<=0xcf) && marker!=0xc4 && marker!=0xc8 && marker!=0xcc) {
                if(len<8) return false; h=(int(p[i+3])<<8)|p[i+4]; w=(int(p[i+5])<<8)|p[i+6]; break;
            }
            i+=len;
        }
    } else return false;
    return w>0 && h>0 && w<=12000 && h<=12000 && int64_t(w)*h<=24000000;
}
String PhotoStore::add(const void* data, size_t size, const String& name, String* newId) {
    int w=0,h=0;
    if(size==0 || size>maxPhotoBytes) return "Photo trop volumineuse (8 Mo maximum).";
    if(!imageDimensions(data,size,w,h)) return "JPEG ou PNG requis, 24 megapixels maximum.";
    auto img=ImageFileFormat::loadFrom(data,size);
    if(!img.isValid()) return "Image illisible.";
    auto p=std::make_shared<Photo>(); p->id=Uuid().toString(); p->title=name.substring(0,160); p->bytes=MemoryBlock(data,size);
    auto scale=jmin(1.0,1600.0/jmax(w,h));
    p->preview=img.rescaled(jmax(1,int(w*scale)),jmax(1,int(h*scale)),Graphics::highResamplingQuality);
    std::lock_guard<std::mutex> l(mutex);
    if(photos.size()>=maxPhotos) return "Limite de 64 photos atteinte pour cette instance.";
    if(totalBytes+size>maxTotalBytes) return "Limite de 64 Mo atteinte pour cette instance.";
    photos.push_back(p); totalBytes+=size; ++revision;
    if(newId) *newId=p->id;
    return {};
}
bool PhotoStore::edit(const String& id,const String& title,const String& notes,int rotation) {
    std::lock_guard<std::mutex> l(mutex);
    for(auto& p:photos) if(p->id==id) {
        auto r=((rotation%4)+4)%4;
        if(p->title==title && p->notes==notes && p->rotation==r) return false;
        auto q=std::make_shared<Photo>(*p); q->title=title.substring(0,160); q->notes=notes.substring(0,16000); q->rotation=r; p=q; ++revision; return true;
    }
    return false;
}
bool PhotoStore::erase(const String& id) {
    std::lock_guard<std::mutex> l(mutex);
    for(auto it=photos.begin(); it!=photos.end(); ++it) if((*it)->id==id) { totalBytes-=(*it)->bytes.getSize(); photos.erase(it); ++revision; return true; }
    return false;
}
void PhotoStore::save(MemoryBlock& out) const {
    auto s=snapshot(); MemoryOutputStream o(out,false); o.writeInt(0x53504831); o.writeInt(1); o.writeInt(int(s.photos.size()));
    for(auto& p:s.photos) {
        o.writeString(p->id); o.writeString(p->title); o.writeString(p->notes); o.writeInt(p->rotation);
        o.writeInt(int(p->bytes.getSize())); o.write(p->bytes.getData(),p->bytes.getSize());
    }
}
bool PhotoStore::restore(const void* data,size_t size) {
    if(!data || size<12 || size>maxTotalBytes+5*1024*1024) return false;
    MemoryInputStream in(data,size,false);
    if(in.readInt()!=0x53504831 || in.readInt()!=1) return false;
    int count=in.readInt(); if(count<0 || count>int(maxPhotos)) return false;
    PhotoStore candidate; std::set<String> ids;
    for(int i=0;i<count;++i) {
        auto id=in.readString(),title=in.readString(),notes=in.readString(); int rot=in.readInt(); int bytes=in.readInt();
        if(id.isEmpty() || id.length()>64 || !ids.insert(id).second || title.length()>160 || notes.length()>16000 || rot<0 || rot>3 || bytes<=0 || size_t(bytes)>maxPhotoBytes || in.getNumBytesRemaining()<bytes) return false;
        MemoryBlock b; if(in.readIntoMemoryBlock(b,size_t(bytes))!=size_t(bytes)) return false;
        if(candidate.add(b.getData(),b.getSize(),title).isNotEmpty()) return false;
        auto p=std::make_shared<Photo>(*candidate.photos.back()); p->id=id; p->notes=notes; p->rotation=rot; candidate.photos.back()=p;
    }
    if(!in.isExhausted()) return false;
    std::lock_guard<std::mutex> l(mutex); photos=std::move(candidate.photos); totalBytes=candidate.totalBytes; ++revision; return true;
}
