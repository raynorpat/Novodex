#include "Bounds3d.h"
#include <algorithm>
#include <cmath>

void Bounds3d::include(const Vec3d & v)
{
    if (empty) { min = max = v; empty = false; return; }
    min.x=std::min(min.x,v.x); min.y=std::min(min.y,v.y); min.z=std::min(min.z,v.z);
    max.x=std::max(max.x,v.x); max.y=std::max(max.y,v.y); max.z=std::max(max.z,v.z);
}
void Bounds3d::intersect(const Bounds3d & b)
{
    if (empty || b.empty) { empty=true; return; }
    min.x=std::max(min.x,b.min.x); min.y=std::max(min.y,b.min.y); min.z=std::max(min.z,b.min.z);
    max.x=std::min(max.x,b.max.x); max.y=std::min(max.y,b.max.y); max.z=std::min(max.z,b.max.z);
    empty = max.x<min.x || max.y<min.y || max.z<min.z;
}
void Bounds3d::combine(const Bounds3d & b)
{
    if (b.empty) return;
    include(b.min); include(b.max);
}
void Bounds3d::transform(const Mat33d & orientation, const Vec3d & translation)
{
    if (empty) return;
    const Vec3d center = orientation*((min+max)*0.5)+translation;
    const Vec3d extent = (max-min)*0.5;
    Vec3d transformedExtent;
    transformedExtent.Set(
        std::fabs(orientation.M33[0][0])*extent.x+std::fabs(orientation.M33[1][0])*extent.y+std::fabs(orientation.M33[2][0])*extent.z,
        std::fabs(orientation.M33[0][1])*extent.x+std::fabs(orientation.M33[1][1])*extent.y+std::fabs(orientation.M33[2][1])*extent.z,
        std::fabs(orientation.M33[0][2])*extent.x+std::fabs(orientation.M33[1][2])*extent.y+std::fabs(orientation.M33[2][2])*extent.z);
    min=center-transformedExtent; max=center+transformedExtent;
}
bool Bounds3d::intersects(const Bounds3d & b) const
{
    return !empty && !b.empty && min.x<=b.max.x && max.x>=b.min.x &&
        min.y<=b.max.y && max.y>=b.min.y && min.z<=b.max.z && max.z>=b.min.z;
}
bool Bounds3d::contain(const Vec3d & v) const
{
    return !empty && v.x>=min.x && v.x<=max.x && v.y>=min.y && v.y<=max.y && v.z>=min.z && v.z<=max.z;
}
