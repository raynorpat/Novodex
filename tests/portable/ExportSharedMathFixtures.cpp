// Capture the existing reconstructed helpers, not shipped-DLL oracle rows.
#include "X87Sqrt.h"
#include "core/JointAcos.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
static void word(std::vector<unsigned char>& out, std::uint64_t value, unsigned width) {
    for (unsigned b=0; b<width; ++b) out.push_back(static_cast<unsigned char>(value >> (8*b)));
}
static double fromBits(std::uint64_t bits) { double value; std::memcpy(&value,&bits,8); return value; }
static std::uint64_t bits(double value) { std::uint64_t result; std::memcpy(&result,&value,8); return result; }
static double run(unsigned op, const double* a) {
    switch(op) {
    case 0: return x87Fsqrt(a[0]);
    case 1: return x87FsqrtSum2(a[0],a[1]);
    case 2: return x87FsqrtSum3(a[0],a[1],a[2]);
    case 3: return x87FsqrtSum4(a[0],a[1],a[2],a[3]);
    case 4: return x87FsqrtDiffSum(a[0],a[1],a[2]);
    case 5: return x87FsqrtDiag(a[0],a[1],a[2]);
    case 6: return x87FsqrtMulSub(a[0],a[1],a[2]);
    case 7: return x87FsqrtDot2(a[0],a[1],a[2],a[3]);
    case 8: return x87FsqrtDot3(a[0],a[1],a[2],a[3],a[4],a[5]);
    case 9: return x87FsqrtDot4(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
    case 10: return x87FsqrtQuotDot3(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
    case 11: return x87FsinHalfOverNorm3(a[0],a[1],a[2],a[3],a[4]);
    case 12: return x87FcosHalfNorm3(a[0],a[1],a[2],a[3],a[4]);
    case 13: return x87RateOverRoot(a[0],a[1],a[2]);
    case 14: return x87CIacos(a[0]);
    case 15: return x87AcosRateOverRoot(a[0],a[1]);
    case 16: return jointCIacos(a[0]);
    default: return jointAcos(static_cast<float>(a[0]));
    }
}
int main(int argc, char** argv) {
    if (argc!=2) { std::fprintf(stderr,"usage: NxPortableExportSharedMath <output>\n"); return 2; }
    static const std::uint64_t inputs[][8] = {
        {0x3fe0000000000000ULL,0x3fe0000000000000ULL,0x3fd0000000000000ULL,0x3fd0000000000000ULL,0x3fd0000000000000ULL,0x3fd0000000000000ULL,0x3fd0000000000000ULL,0x3fd0000000000000ULL},
        {0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL,0x3ff0000000000000ULL},
        {0,0,0,0,0,0,0,0},
        {0x8000000000000000ULL,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1},
        {0x3ff0000000000001ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL,0x3ca0000000000000ULL},
        {0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL,0x7fefffffffffffffULL},
        {0xbff0000000000000ULL,0x3ff0000000000000ULL,0x3fe0000000000000ULL,0,0,0,0,0},
        {0x7ff0000000000000ULL,0,0,0,0,0,0,0},
        {0x7ff8000000000123ULL,0x7ff8000000000456ULL,0,0,0,0,0,0}
    };
    std::vector<unsigned char> payload;
    unsigned short saved;
    __asm fnstcw saved
    const unsigned short controls[] = {0x027f,0x0f7f};
    for (unsigned cw=0; cw<2; ++cw) {
        unsigned short control=controls[cw];
        __asm fldcw control
        for (unsigned op=0; op<18; ++op) for (unsigned c=0; c<10; ++c) {
            double args[8];
            word(payload,op,4); word(payload,control,4);
            for (unsigned i=0;i<8;++i) { args[i]=fromBits(inputs[c][i]); word(payload,inputs[c][i],8); }
            const double result=run(op,args); // explicit binary64 observation store
            word(payload,bits(result),8);
        }
    }
    __asm fldcw saved
    FILE* file=std::fopen(argv[1],"wb");
    if (!file) return 1;
    std::vector<unsigned char> header;
    header.push_back('N'); header.push_back('X'); header.push_back('P'); header.push_back('F');
    word(header,1,4); word(header,payload.size(),4); word(header,80,4);
    const bool ok=std::fwrite(header.data(),1,header.size(),file)==header.size() &&
        std::fwrite(payload.data(),1,payload.size(),file)==payload.size();
    const int closed=std::fclose(file);
    if (!ok || closed) return 1;
    std::printf("shared_math records=360 width=80 provenance=reconstructed-reference\n");
    return 0;
}
