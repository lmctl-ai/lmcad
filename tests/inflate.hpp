#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
// Independent, deliberately simple test decoder for stored/fixed DEFLATE.
inline std::vector<unsigned char> inflate_test(const std::vector<unsigned char>& z) {
  auto require=[](bool ok){if(!ok)throw std::runtime_error("invalid test zlib stream");};
  require(z.size()>=6&&(unsigned(z[0])*256+z[1])%31==0&&z[0]==0x78&&!(z[1]&32));
  size_t bit=16;auto get=[&](unsigned n){unsigned v=0;for(unsigned k=0;k<n;k++){require(bit<(z.size()-4)*8);v|=((z[bit/8]>>(bit%8))&1)<<k;++bit;}return v;};
  auto symbol=[&](){unsigned code=0;for(unsigned n=1;n<=9;n++){code=(code<<1)|get(1);
    if(n==7&&code<24)return code+256;
    if(n==8&&code>=0x30&&code<=0xbf)return code-0x30;
    if(n==8&&code>=0xc0&&code<=0xc7)return code-0xc0+280;
    if(n==9&&code>=0x190&&code<=0x1ff)return code-0x190+144;
  }throw std::runtime_error("bad fixed code");};
  const unsigned lb[]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
  const unsigned le[]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
  const unsigned db[]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
  const unsigned de[]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
  std::vector<unsigned char> raw;bool final=false;
  do{final=get(1);unsigned type=get(2);require(type<2);
    if(type==0){bit=(bit+7)&~size_t(7);unsigned len=get(16),inv=get(16);require((len^inv)==65535);while(len--)raw.push_back(get(8));}
    else for(;;){unsigned s=symbol();if(s==256)break;if(s<256){raw.push_back(s);continue;}
      require(s<=285);unsigned n=lb[s-257]+get(le[s-257]),d=0;for(int k=0;k<5;k++)d=(d<<1)|get(1);
      require(d<30);unsigned distance=db[d]+get(de[d]);require(distance<=raw.size());
      while(n--)raw.push_back(raw[raw.size()-distance]);
    }
  }while(!final);
  require((bit+7)/8==z.size()-4);uint32_t a=1,b=0;for(auto v:raw){a=(a+v)%65521;b=(b+a)%65521;}
  uint32_t sum=0;for(size_t p=z.size()-4;p<z.size();p++)sum=(sum<<8)|z[p];require(sum==((b<<16)|a));return raw;
}
