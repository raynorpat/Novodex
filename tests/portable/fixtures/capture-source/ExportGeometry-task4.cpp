#include "GeometryDomain.h"
#include <float.h>
int main(int argc,char** argv) {
    if(argc!=2||(_controlfp(0,0)&(_MCW_PC|_MCW_RC))!=(_PC_53|_RC_NEAR))return 1;
    return nxGeometryExport(argv[1]);
}
