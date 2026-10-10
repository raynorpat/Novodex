// Reconstructed storage semantics from wuFistp255/sceneDumpRound, not shipped
// DLL oracle outputs. Instruction bodies are checked by the capture wrapper.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
static const float gWu255=255.0f;
static __declspec(naked) int __cdecl wuFistp255(double /*x*/) {
    __asm {
        fld qword ptr [esp+4]
        fmul dword ptr [gWu255]
        fistp qword ptr [esp+4]
        mov eax,dword ptr [esp+4]
        ret
    }
}
static int sceneDumpRound(float value) {
    __int64 result;
    __asm { fld value
        fistp result }
    return (int)result;
}
static void word(std::vector<unsigned char>& out,std::uint64_t v,unsigned n) { for(unsigned i=0;i<n;++i) out.push_back(static_cast<unsigned char>(v>>(i*8))); }
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    const std::uint64_t inputs[]={
        0,0x8000000000000000ULL,0x3fe0000000000000ULL,0xbfe0000000000000ULL,
        0x3ff8000000000000ULL,0xbff8000000000000ULL,0x4004000000000000ULL,0xc004000000000000ULL,
        0x41dfffffffc00000ULL,0xc1e0000000000000ULL,0x41e0000000000000ULL,0xc1e0000000200000ULL,
        0x41efffffffe00000ULL,0x41f0000000000000ULL,0x43dfffffffffffffULL,0x43e0000000000000ULL,
        0xc3e0000000000000ULL,0xc3e0000000000001ULL,
        0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8000000000001ULL,1,
        0x3f60101010101010ULL,0x3f78181818181818ULL,0x3f84141414141414ULL
    };
    unsigned short saved; __asm fnstcw saved
    const unsigned short controls[]={0x027f,0x0f7f}; std::vector<unsigned char> payload;
    for(unsigned ci=0;ci<2;++ci) { unsigned short cw=controls[ci]; __asm fldcw cw
        for(unsigned op=0;op<2;++op) for(unsigned i=0;i<sizeof(inputs)/sizeof(inputs[0]);++i) {
            double x; std::memcpy(&x,&inputs[i],8);
            // float narrowing is an actual SceneDump storage boundary; the
            // literal promoted input bytes below are what it consumes.
            if(op==1) x=static_cast<float>(x);
            std::uint64_t input; std::memcpy(&input,&x,8);
            const int result=op==0?wuFistp255(x):sceneDumpRound(static_cast<float>(x));
            word(payload,op,4);word(payload,cw,4);word(payload,input,8);
            word(payload,static_cast<std::uint32_t>(result),4);
        }
    }
    __asm fldcw saved
    std::vector<unsigned char> out={'N','X','P','F'};
    word(out,1,4);word(out,payload.size(),4);word(out,20,4);out.insert(out.end(),payload.begin(),payload.end());
    FILE* f=std::fopen(argv[1],"wb");if(!f)return 1;
    const bool ok=std::fwrite(out.data(),1,out.size(),f)==out.size();const int closed=std::fclose(f);
    return ok && !closed?0:1;
}
