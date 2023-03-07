//  sync_client_type_private.h
//  sync_client
//  Created by housisong on 2019-09-17.
/*
 The MIT License (MIT)
 Copyright (c) 2019-2020 HouSisong
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef sync_client_type_private_h
#define sync_client_type_private_h
#include "sync_client_type.h"
#include "../../libHDiffPatch/HDiff/private_diff/limit_mem_diff/adler_roll.h"

namespace sync_private{

hpatch_inline static void
TNewDataSyncInfo_init(TNewDataSyncInfo* self) { memset(self,0,sizeof(*self)); }
    
hpatch_inline static
hpatch_StreamPos_t TNewDataSyncInfo_blockCount(const TNewDataSyncInfo* self){
        return getSyncBlockCount(self->newDataSize,self->kSyncBlockSize); }

hpatch_inline static
uint32_t TNewDataSyncInfo_newDataBlockSize(const TNewDataSyncInfo* self,uint32_t blockIndex){
    uint32_t blockCount=(uint32_t)TNewDataSyncInfo_blockCount(self);
    if (blockIndex+1!=blockCount)
        return self->kSyncBlockSize;
    else
        return (uint32_t)(self->newDataSize-self->kSyncBlockSize*blockIndex);
}
hpatch_inline static
bool TNewDataSyncInfo_syncBlockIsCompressed(const TNewDataSyncInfo* self,uint32_t blockIndex){
    return (self->savedSizes)&&(self->savedSizes[blockIndex]);
}
hpatch_inline static
uint32_t TNewDataSyncInfo_syncBlockSize(const TNewDataSyncInfo* self,uint32_t blockIndex){
    if (TNewDataSyncInfo_syncBlockIsCompressed(self,blockIndex))
        return self->savedSizes[blockIndex];
    else
        return TNewDataSyncInfo_newDataBlockSize(self,blockIndex);
}
    
inline static uint64_t roll_hash_start(uint64_t*,const adler_data_t* pdata,size_t n){
                                        return fast_adler64_start(pdata,n); }
inline static uint64_t roll_hash_roll(uint64_t adler,size_t blockSize,
                                      adler_data_t out_data,adler_data_t in_data){
                                        return fast_adler64_roll(adler,blockSize,out_data,in_data); }
    
#define kStrongChecksumBits_min    (4*8)
    
hpatch_inline static
void toPartChecksum(unsigned char* out_partChecksum,size_t outPartBits,
                    const unsigned char* checksum,size_t kChecksumByteSize){
    const size_t outPartBytes=_bitsToBytes(outPartBits);
    assert(outPartBytes<=kChecksumByteSize);
    if (out_partChecksum!=checksum)
        memmove(out_partChecksum,checksum,outPartBytes);
    size_t lasti=(outPartBits>>3);
    if (outPartBytes>lasti)
        out_partChecksum[lasti]&=((1<<(outPartBits&7))-1);
}

static hpatch_inline
size_t checkChecksumBufByteSize(size_t kStrongChecksumByteSize){
        return kStrongChecksumByteSize*2; }
static hpatch_inline
void checkChecksumInit(unsigned char* checkChecksumBuf,size_t kStrongChecksumByteSize){
        assert(kStrongChecksumByteSize>0);
        assert((kStrongChecksumByteSize%4)==0);
        memset(checkChecksumBuf+kStrongChecksumByteSize,0,
                    checkChecksumBufByteSize(kStrongChecksumByteSize)-kStrongChecksumByteSize); }

    static hpatch_inline
    uint32_t _readUInt32(const unsigned char* src){ return src[0] | (src[1]<<8) | (src[2]<<16)| (src[3]<<24); }
    static hpatch_inline
    void _writeUInt32(unsigned char* dst,uint32_t v){
        dst[0]=(unsigned char)v;       dst[1]=(unsigned char)(v>>8);
        dst[2]=(unsigned char)(v>>16); dst[3]=(unsigned char)(v>>24); }

static hpatch_inline
void checkChecksumAppendData(unsigned char* checkChecksumBuf,uint32_t checksumIndex,
                             const unsigned char* strongChecksum,size_t kStrongChecksumByteSize){
    unsigned char* d_xor=checkChecksumBuf+kStrongChecksumByteSize;
    unsigned char* d_xor_end=d_xor+kStrongChecksumByteSize;
    static const uint32_t _checksumStart=fast_adler32_start(0,0);
    for (;d_xor<d_xor_end;strongChecksum+=sizeof(uint32_t),d_xor+=sizeof(uint32_t)){
        unsigned char _checksumIndex[sizeof(uint32_t)]; _writeUInt32(_checksumIndex,checksumIndex);
        checksumIndex=fast_adler32_append(_checksumStart,_checksumIndex,sizeof(_checksumIndex));
        uint32_t src_v = _readUInt32(strongChecksum);
        _writeUInt32(d_xor,_readUInt32(d_xor) ^ (src_v+checksumIndex));
    }
}
static hpatch_inline
void checkChecksumEndTo(unsigned char* dst,
                        const unsigned char* checkChecksumBuf,size_t kStrongChecksumByteSize){
    const unsigned char* d_xor=checkChecksumBuf+kStrongChecksumByteSize;
    const unsigned char* d_xor_end=d_xor+kStrongChecksumByteSize;
    while (d_xor<d_xor_end){
        *dst++=(*d_xor++);
    }
}
static hpatch_inline
void checkChecksumEnd(unsigned char* checkChecksumBuf,size_t kStrongChecksumByteSize){
    return checkChecksumEndTo(checkChecksumBuf,checkChecksumBuf,kStrongChecksumByteSize); }

    
static hpatch_inline
void writeRollHashBytes(uint8_t* out_part,uint64_t partRollHash,size_t savedRollHashByteSize){
    switch (savedRollHashByteSize) {
        case 8: *out_part++=(uint8_t)(partRollHash>>56);
        case 7: *out_part++=(uint8_t)(partRollHash>>48);
        case 6: *out_part++=(uint8_t)(partRollHash>>40);
        case 5: *out_part++=(uint8_t)(partRollHash>>32);
        case 4: *out_part++=(uint8_t)(partRollHash>>24);
        case 3: *out_part++=(uint8_t)(partRollHash>>16);
        case 2: *out_part++=(uint8_t)(partRollHash>> 8);
        case 1: *out_part=(uint8_t)(partRollHash);
            return;
        default:
            assert(false);
            return;
    }
}
  
static hpatch_inline
uint64_t readRollHashBytes(const uint8_t* part,size_t savedRollHashByteSize){
    uint64_t partRollHash=0;
    switch (savedRollHashByteSize) {
        case 8: partRollHash|=((uint64_t)*part++)<<56;
        case 7: partRollHash|=((uint64_t)*part++)<<48;
        case 6: partRollHash|=((uint64_t)*part++)<<40;
        case 5: partRollHash|=((uint64_t)*part++)<<32;
        case 4: partRollHash|=((uint64_t)*part++)<<24;
        case 3: partRollHash|=((uint64_t)*part++)<<16;
        case 2: partRollHash|=((uint64_t)*part++)<< 8;
        case 1: partRollHash|=((uint64_t)*part);
            return partRollHash;
        default:
            assert(false);
            return partRollHash;
    }
}

static hpatch_inline
uint64_t toSavedPartRollHash(uint64_t rollHash,size_t savedRollHashBits){
    if (savedRollHashBits<64)
        return ((rollHash>>savedRollHashBits)^rollHash) & ((((uint64_t)1)<<savedRollHashBits)-1);
    else
        return rollHash;
}

hpatch_inline static unsigned int upper_ilog2(uint64_t v){
    const unsigned int _kMaxBit=sizeof(v)*8;
    unsigned int bit=1;
    while ((((uint64_t)1)<<bit)<v){ 
        ++bit;
        if (bit==_kMaxBit) break;
    }
    return bit;
}
    
hpatch_inline static
unsigned int getBetterCacheBlockTableBit(uint32_t blockCount){
    const unsigned int kMinBit = 8;
    const unsigned int kMaxBit = 23; //for limit cache memory size
    int result=(int)upper_ilog2((1<<kMinBit)+(uint64_t)blockCount)-2;
    result=(result<kMinBit)?kMinBit:result;
    result=(result>kMaxBit)?kMaxBit:result;
    return result;
}

} //namespace sync_private
#endif //sync_client_type_private_h
