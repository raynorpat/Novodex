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
    const bool physicsDomain=argc==3 && std::strcmp(argv[2],"--physics-domain")==0;
    const bool rotationDomain=argc==3 && std::strcmp(argv[2],"--rotation-domain")==0;
    if (argc!=2 && !physicsDomain && !rotationDomain) { std::fprintf(stderr,"usage: NxPortableExportSharedMath <new-output> [--physics-domain|--rotation-domain]\n"); return 2; }
    // Historical fixture is immutable, including when invoked outside its wrapper.
    if (std::strstr(argv[1],"shared-math-x87.nxpf")) {
        std::fprintf(stderr,"refusing to overwrite immutable Task 1 fixture\n"); return 2;
    }
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
        for (unsigned op=0; op<18; ++op) for (unsigned c=0; c<(physicsDomain?130u:rotationDomain?32u:10u); ++c) {
            double args[8];
            word(payload,op,4); word(payload,control,4);
            if (rotationDomain) {
                static const std::uint32_t angles[]={0,0x3c800000,0x3f000000,0x3f800000,
                    0x3fc90fda,0x3fc90fdb,0x3fc90fdc,0x40490fda,0x40490fdb,0x40490fdc,
                    0x40c90fda,0x40c90fdb,0x40c90fdc,0xc0490fdb,0xc0c90fdb,0x80000000};
                const std::uint32_t input=angles[c%16];float angle;std::memcpy(&angle,&input,4);
                for(unsigned i=0;i<8;++i) args[i]=0.5;
                if(op==11 || op==12) {
                    args[0]=c<16?angle:((input&0x80000000)?-0.015625:0.015625);
                    args[1]=0.5;args[2]=c<16?2.0:(angle<0?-angle:angle)*128.0;
                    args[3]=args[4]=0.0;
                }
            } else if (!physicsDomain) {
                for (unsigned i=0;i<8;++i) args[i]=fromBits(inputs[c][i]);
            } else if(c==128 || c==129) {
                // Finite float inputs, finite double norm/angle, but FSIN/FCOS
                // refuses range reduction at |angle| >= 2^63. Separate probes.
                for(unsigned i=0;i<8;++i) args[i]=1.0;
                args[0]=fromBits(c==128?0x43e0000000000000ULL:0x43d0000000000000ULL);
                args[1]=1.0; args[2]=1.0; args[3]=args[4]=0.0;
            } else {
                // Fixed PRNG integer words expanded from positive binary32.
                // No random runtime/decimal compiler rounding generates inputs.
                std::uint32_t seed=0x91e10da5u ^ (c*0x9e3779b9u);
                for(unsigned i=0;i<8;++i) {
                    seed=seed*1664525u+1013904223u;
                    const std::uint32_t input=((110u+((seed>>23)%21u))<<23)|(seed&0x7fffffu);
                    float f; std::memcpy(&f,&input,4); args[i]=f;
                }
                if(op==4) args[0]+=args[1];
                if(op==5) args[0]+=args[1]+args[2];
                if(op==6) args[2]*=0.000001;
                if(op==11 || op==12) { args[0]=0.015625; args[1]=0.5; }
                if(op==13) args[2]=static_cast<double>(static_cast<int>(seed&0xffffu)-32768)/32768.0;
                if(op>=14) args[0]=static_cast<double>(static_cast<int>(seed&0xffffu)-32768)/32768.0;
            }
            for(unsigned i=0;i<8;++i) word(payload,bits(args[i]),8);
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
    std::printf("shared_math records=%u width=80 provenance=reconstructed-reference domain=%s\n",
        static_cast<unsigned>(payload.size()/80),physicsDomain?"physics-finite-and-fsin-limit-probes":rotationDomain?"rotation-endpoints-and-adjacent-floats":"task1-repeat");
    return 0;
}
