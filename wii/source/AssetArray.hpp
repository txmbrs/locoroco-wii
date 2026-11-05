#pragma once
#include <cstdlib>
#include <cstddef>
#include <malloc.h>
template<class T> class AssetArray {
    T* data_=nullptr;size_t size_=0;
public:
    bool allocate(size_t n){data_=n?static_cast<T*>(memalign(32,n*sizeof(T))):nullptr;size_=n;return n==0||data_;}
    operator const T*()const{return data_;}
    template<class I>T& operator[](I i){return data_[i];}
    template<class I>const T& operator[](I i)const{return data_[i];}
    const T* begin()const{return data_;}
    const T* end()const{return size_?data_+size_:data_;}
    size_t size()const{return size_;}
};
