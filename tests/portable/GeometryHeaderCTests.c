#include "portable/NxScalarGeometry.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
static float from_word(uint32_t w) { float f; memcpy(&f,&w,4);return f; }
int main(void) {
    float a[3]={3,4,0}; nxScalarNormalizeDirection3(a);
    if(a[0]!=0.6f||a[1]!=0.8f||a[2]!=0) return 1;
    a[0]=-0.0f;a[1]=0;a[2]=0;nxScalarNormalizeDirection3(a);
    if(!signbit(a[0])||a[1]!=0||a[2]!=0) return 2;
    a[0]=from_word(1);a[1]=0;a[2]=0;nxScalarNormalizeDirection3(a);
    if(a[0]!=1||a[1]!=0||a[2]!=0) return 3;
    a[0]=from_word(0x7f7fffff);a[1]=0;a[2]=0;nxScalarNormalizeDirection3(a);
    if(a[0]!=1) return 4;
    a[0]=NAN;a[1]=0;a[2]=1;nxScalarNormalizeDirection3(a);
    if(!isnan(a[0])||!isnan(a[1])||!isnan(a[2])) return 5;
    return 0;
}
