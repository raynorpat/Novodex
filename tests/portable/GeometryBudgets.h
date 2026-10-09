#ifndef NX_PORTABLE_GEOMETRY_BUDGETS_H
#define NX_PORTABLE_GEOMETRY_BUDGETS_H
#include <cmath>
#include "FixtureSupport.h"
// Measured on the fixed [-8,8] domain before acceptance: all finite maxima 0.
// Budgets are specific to output units; zero, sentinel, endpoint and discrete
// observations have exact predicates and do not enter these comparisons.
struct GeometryBudget { double absolute, relative; const char* units; };
static GeometryBudget geometryBudget(unsigned op,unsigned field) {
    const GeometryBudget squared={1e-8,2e-7,"length squared"};
    const GeometryBudget parameter={1e-7,2e-7,"parameter"};
    const GeometryBudget length={1e-6,2e-7,"length"};
    const GeometryBudget normal={2e-7,2e-7,"normal component"};
    if(op==0) return parameter;
    if(op==1||op==2) return field==0?squared:parameter;
    if(op==3||op==5) return field==0?squared:field==1?parameter:length;
    if(op==4) return field==0?squared:length;
    if(op==6) return length;
    if(op>=7&&op<=9) return field<=3?length:field<=6?normal:parameter;
    return normal;
}
static const double geometryNormalAngleBudget=2e-6; // radians
static const double geometryNormalResidualBudget=2e-7; // |norm-1|
static bool geometryOutputAccepted(unsigned op,unsigned field,double actual,double reference) {
    bool endpoint=((op<=2&&field>0)||(op==3&&field==1))&&reference==1;
    bool integer=(op>=7&&op<=9)&&field==7;
    if(reference==0||reference==-123.25||endpoint||integer)return actual==reference;
    GeometryBudget b=geometryBudget(op,field);
    return nxWithinBudget(actual,reference,b.absolute,b.relative);
}
static bool geometryNormalAccepted(const double* a,const double* b) {
    double aa=(a[0]*a[0]+a[1]*a[1])+a[2]*a[2];
    double bb=(b[0]*b[0]+b[1]*b[1])+b[2]*b[2];
    if(!std::isfinite(aa)||!std::isfinite(bb))return true; // explicit class predicate owns these rows
    if(bb==0)return aa==0;
    double x=a[1]*b[2]-a[2]*b[1],y=a[2]*b[0]-a[0]*b[2],z=a[0]*b[1]-a[1]*b[0];
    double angle=std::atan2(std::sqrt((x*x+y*y)+z*z),(a[0]*b[0]+a[1]*b[1])+a[2]*b[2]);
    return angle<=geometryNormalAngleBudget&&std::fabs(std::sqrt(aa)-1)<=geometryNormalResidualBudget;
}
#endif
