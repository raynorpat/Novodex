// Scalar definitions of the actual ConvexHull.cpp rows. Included only by its
// backend0 branch; shared integer gather/extract bodies remain in that TU.
#include <cmath>
#include <cfloat>
#include <new>
#include <cstdlib>
#include <cstring>

void nxIceVectorConstruct(void* array, NxU32 size, NxU32 count, NxHullElementConstructor constructor)
{
    if(static_cast<NxI32>(count-1)<0) return;
    unsigned char* element=static_cast<unsigned char*>(array);
    for(NxU32 i=0;i<count;++i,element+=size) constructor(element);
}
void* nxHullPolygonConstruct(void* polygon)
{
    HullPolygon* p=static_cast<HullPolygon*>(polygon);
    p->mNbVerts=0;p->mVRefs=0;p->mERefs=0;return polygon;
}
void* nxIceIdentityConstruct(void* object) { return object; }
void* nxEdgeDescConstruct(void* desc)
{
    EdgeDesc* e=static_cast<EdgeDesc*>(desc);e->Flags=0;e->Count=0;e->Offset=0;return desc;
}
float nxHullTriangleArea(const NxU16* triangle,const IceMaths::Point* vertices)
{
    if(!vertices) return 0.0f;
    const IceMaths::Point& p=vertices[triangle[0]];
    const IceMaths::Point& q=vertices[triangle[1]];
    const IceMaths::Point& r=vertices[triangle[2]];
    const double ax=double(p.x)-r.x,ay=double(p.y)-r.y,az=double(p.z)-r.z;
    const float bx=float(double(p.x)-q.x),by=float(double(p.y)-q.y);
    const double bz=double(p.z)-q.z;
    const float x=float(double(by)*az-bz*ay),y=float(bz*ax-double(bx)*az);
    const double z=double(bx)*ay-double(by)*ax;
    return float(std::sqrt((z*z+double(y)*y)+double(x)*x)*0.5);
}
void nxHullTriangleCenter(const NxU16* triangle,const IceMaths::Point* vertices,IceMaths::Point* center)
{
    if(!vertices) return;
    const IceMaths::Point& p=vertices[triangle[0]];
    const IceMaths::Point& q=vertices[triangle[1]];
    const IceMaths::Point& r=vertices[triangle[2]];
    const float x=float((double(q.x)+p.x)+r.x),z=float(double(q.z)+p.z);
    center->x=float(double(x)*kIceHullThird);
    center->y=float(((double(q.y)+p.y)+r.y)*kIceHullThird);
    center->z=float((double(z)+r.z)*kIceHullThird);
}
bool nxHullComputeCentroid(const ConvexHull* hull,IceMaths::Point* center)
{
    if(!hull->mNbVerts||!hull->mVerts) return false;
    center->Set(0,0,0);float total=0;
    for(NxU32 i=0;i<hull->mNbFaces;++i){
        const NxU16* triangle=hull->mFaces+i*3;
        const float area=nxHullTriangleArea(triangle,hull->mVerts);
        IceMaths::Point c;nxHullTriangleCenter(triangle,hull->mVerts,&c);
        const float z=float(double(c.z)*area);
        center->x=float(double(c.x)*area+center->x);
        center->y=float(double(c.y)*area+center->y);
        center->z=float(double(z)+center->z);total=float(double(area)+total);
    }
    const double inverse=1.0/total;
    center->x=float(inverse*center->x);center->y=float(inverse*center->y);center->z=float(inverse*center->z);return true;
}
bool nxHullPolygonPlane(IceMaths::Plane* plane,NxU32 count,const NxU32* refs,const IceMaths::Point* vertices)
{
    if(!count||!refs||!vertices) return false;
    const NxU32 step=count/3;NxU32 best=0;float bestArea=-FLT_MAX;
    for(NxU32 i=0;i<count;++i){const NxU32 j=(i+step)%count,k=(j+step)%count;
        const IceMaths::Triangle triangle(vertices[refs[i]],vertices[refs[j]],vertices[refs[k]]);
        const float area=triangle.Area();if(area>bestArea){bestArea=area;best=i;}}
    const NxU32 j=(best+step)%count,k=(j+step)%count;
    plane->Set(vertices[refs[best]],vertices[refs[j]],vertices[refs[k]]);return true;
}
static double nxHullDotZYX(const IceMaths::Point& a,const IceMaths::Point& b)
{ return (double(a.z)*b.z+double(a.y)*b.y)+double(a.x)*b.x; }
static double nxHullDotZXY(const IceMaths::Point& a,const IceMaths::Point& b)
{ return (double(a.z)*b.z+double(a.x)*b.x)+double(a.y)*b.y; }
static IceMaths::Point nxHullLocalAxis(const IceMaths::Point& axis,const float* pose)
{
    if(!pose) return axis;
    return IceMaths::Point(float((double(pose[0])*axis.x+double(pose[1])*axis.y)+double(pose[2])*axis.z),
        float((double(pose[4])*axis.x+double(pose[5])*axis.y)+double(pose[6])*axis.z),
        float((double(pose[8])*axis.x+double(pose[9])*axis.y)+double(pose[10])*axis.z));
}
bool nxHullComputePolygons(ConvexHull* hull)
{
    hull->mNbPolygons=0;
    if(hull->mPolygonVRefs){nxIceFree(hull->mPolygonVRefs);hull->mPolygonVRefs=0;}
    if(hull->mPolygons){nxIceDeleteArray(hull->mPolygons);hull->mPolygons=0;}
    IceCore::Container data;if(!nxHullExtractPolygons(&hull->mNbPolygons,&data,hull)) return false;
    hull->mPolygons=static_cast<HullPolygon*>(nxIceNewArray(hull->mNbPolygons,sizeof(HullPolygon),NX_MEMORY_PERSISTENT));
    if(!hull->mPolygons) return false;
    nxIceVectorConstruct(hull->mPolygons,sizeof(HullPolygon),hull->mNbPolygons,nxHullPolygonConstruct);
    //001472's centroid is local scratch at esp+0x28, never hull+0x18.
    IceMaths::Point centroid;nxHullComputeCentroid(hull,&centroid);
    const NxU32 total=data.GetNbEntries()-hull->mNbPolygons;
    hull->mPolygonVRefs=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_PERSISTENT));
    if(!hull->mPolygonVRefs) return false;
    const NxU32* source=data.GetEntries();NxU32* output=hull->mPolygonVRefs;
    for(NxU32 i=0;i<hull->mNbPolygons;++i){HullPolygon& p=hull->mPolygons[i];p.mNbVerts=*source++;p.mVRefs=output;
        std::memcpy(output,source,p.mNbVerts*4);source+=p.mNbVerts;
        nxHullPolygonPlane(&p.mPlane,p.mNbVerts,output,hull->mVerts);
        const double side=(double(centroid.y)*p.mPlane.n.y+double(centroid.z)*p.mPlane.n.z)+double(centroid.x)*p.mPlane.n.x+p.mPlane.d;
        if(side>0){nxIceReverseArray(p.mNbVerts,output);p.mPlane.n.x=-p.mPlane.n.x;p.mPlane.n.y=-p.mPlane.n.y;p.mPlane.n.z=-p.mPlane.n.z;p.mPlane.d=-p.mPlane.d;}
        output+=p.mNbVerts;
    }
    for(NxU32 i=0;i<hull->mNbPolygons;++i){HullPolygon& p=hull->mPolygons[i];p.mMin=FLT_MAX;p.mMax=-FLT_MAX;
        for(NxU32 j=0;j<hull->mNbVerts;++j){const double projection=nxHullDotZYX(hull->mVerts[j],p.mPlane.n);
            if(projection<p.mMin)p.mMin=float(projection);if(projection>p.mMax)p.mMax=float(projection);}}
    return true;
}
// The support scans preserve the listing's first seed, four-at-a-time term
// grouping, and narrow only a winning candidate before retaining its score.
static NxU32 nxHullBestPolygon(ConvexHull* hull,const IceMaths::Point& axis,double& score)
{
    if(!hull->mNbPolygons)nxHullComputePolygons(hull);
    NxU32 best=0;score=nxHullDotZYX(axis,hull->mPolygons[0].mPlane.n);NxU32 i=1;
    if(hull->mNbPolygons-1>=4)for(;i<hull->mNbPolygons-3;i+=4)for(NxU32 j=0;j<4;++j){
        const double value=j?nxHullDotZYX(axis,hull->mPolygons[i+j].mPlane.n):nxHullDotZXY(axis,hull->mPolygons[i].mPlane.n);
        if(value>score){best=i+j;score=float(value);}}
    for(;i<hull->mNbPolygons;++i){const double value=nxHullDotZXY(axis,hull->mPolygons[i].mPlane.n);if(value>score){best=i;score=float(value);}}
    return best;
}
NxU32 nxHullSupportPolygon(ConvexHull* hull,const IceMaths::Point* axis,const float* pose)
{ double score;return nxHullBestPolygon(hull,nxHullLocalAxis(*axis,pose),score); }
bool nxHullComputeEdges(ConvexHull* hull)
{
    if(!hull->mNbPolygons)nxHullComputePolygons(hull);
    NxU32 total=0;for(NxU32 p=0;p<hull->mNbPolygons;++p){if(!hull->mPolygons)nxHullComputePolygons(hull);total+=hull->mPolygons[p].mNbVerts;}
    // The original failure arms leave earlier allocations in place. This gate
    // does not change historically unchecked allocation/empty-edge behavior.
    NxU32* high=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!high)return false;
    NxU32* low=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!low)return false;
    NxU32* polygon=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!polygon)return false;
    NxU32* position=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!position)return false;
    NxU32 index=0;for(NxU32 p=0;p<hull->mNbPolygons;++p){if(!hull->mPolygons)nxHullComputePolygons(hull);const HullPolygon& poly=hull->mPolygons[p];
        for(NxU32 j=0;j<poly.mNbVerts;++j,++index){NxU32 a=poly.mVRefs[j],b=poly.mVRefs[(j+1)%poly.mNbVerts];if(a>b){const NxU32 t=a;a=b;b=t;}
            low[index]=a;high[index]=b;polygon[index]=p;position[index]=j;}}
    IceCore::RadixSort sorter;const NxU32* ranks=sorter.Sort(high,total,IceCore::RADIX_SIGNED).Sort(low,total,IceCore::RADIX_SIGNED).GetRanks();
    hull->mNbEdges=0;HullEdge* oversized=static_cast<HullEdge*>(nxIceNewArray(total,sizeof(HullEdge),NX_MEMORY_PERSISTENT));if(!oversized)return false;
    nxIceVectorConstruct(oversized,sizeof(HullEdge),total,nxIceIdentityConstruct);
    NxU32* sortedPosition=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!sortedPosition)return false;
    NxU32* sortedPolygon=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!sortedPolygon)return false;
    NxU32* sortedEdge=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_TEMP));if(!sortedEdge)return false;
    NxU32 lastLow=0xffffffff,lastHigh=0xffffffff;
    for(NxU32 i=0;i<total;++i){const NxU32 r=ranks[i];if(low[r]!=lastLow||high[r]!=lastHigh){lastLow=low[r];lastHigh=high[r];
            oversized[hull->mNbEdges].mRef0=lastLow;oversized[hull->mNbEdges].mRef1=lastHigh;++hull->mNbEdges;}
        sortedPosition[i]=position[r];sortedPolygon[i]=polygon[r];sortedEdge[i]=hull->mNbEdges-1;}
    if(hull->mEdges){nxIceDeleteArray(hull->mEdges);hull->mEdges=0;}
    hull->mEdges=static_cast<HullEdge*>(nxIceNewArray(hull->mNbEdges,sizeof(HullEdge),NX_MEMORY_PERSISTENT));if(!hull->mEdges)return false;
    nxIceVectorConstruct(hull->mEdges,sizeof(HullEdge),hull->mNbEdges,nxIceIdentityConstruct);
    std::memcpy(hull->mEdges,oversized,hull->mNbEdges*sizeof(HullEdge));nxIceDeleteArray(oversized);
    ranks=sorter.Sort(sortedPosition,total,IceCore::RADIX_SIGNED).Sort(sortedPolygon,total,IceCore::RADIX_SIGNED).GetRanks();
    if(hull->mPolygonERefs){nxIceFree(hull->mPolygonERefs);hull->mPolygonERefs=0;}
    hull->mPolygonERefs=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_PERSISTENT));
    for(NxU32 i=0;i<total;++i)hull->mPolygonERefs[i]=sortedEdge[ranks[i]];
    NxU32* run=hull->mPolygonERefs;for(NxU32 p=0;p<hull->mNbPolygons;++p){hull->mPolygons[p].mERefs=run;run+=hull->mPolygons[p].mNbVerts;}
    nxIceFree(position);nxIceFree(polygon);nxIceFree(low);nxIceFree(high);
    if(hull->mEdgeToPolygons){nxIceFree(hull->mEdgeToPolygons);hull->mEdgeToPolygons=0;}
    hull->mEdgeToPolygons=static_cast<EdgeDesc*>(nxIceAlloc(hull->mNbEdges*sizeof(EdgeDesc),NX_MEMORY_PERSISTENT));if(!hull->mEdgeToPolygons)return false;
    nxIceVectorConstruct(hull->mEdgeToPolygons,sizeof(EdgeDesc),hull->mNbEdges,nxEdgeDescConstruct);
    for(NxU32 i=0;i<total;++i)++hull->mEdgeToPolygons[hull->mPolygonERefs[i]].Count;
    hull->mEdgeToPolygons[0].Offset=0;for(NxU32 i=1;i<hull->mNbEdges;++i)hull->mEdgeToPolygons[i].Offset=hull->mEdgeToPolygons[i-1].Offset+hull->mEdgeToPolygons[i-1].Count;
    if(hull->mEdgePolygons){nxIceFree(hull->mEdgePolygons);hull->mEdgePolygons=0;}
    hull->mEdgePolygons=static_cast<NxU32*>(nxIceAlloc(total*4,NX_MEMORY_PERSISTENT));if(!hull->mEdgePolygons)return false;
    for(NxU32 i=0;i<total;++i){EdgeDesc& e=hull->mEdgeToPolygons[hull->mPolygonERefs[i]];hull->mEdgePolygons[e.Offset++]=sortedPolygon[ranks[i]];}
    hull->mEdgeToPolygons[0].Offset=0;for(NxU32 i=1;i<hull->mNbEdges;++i)hull->mEdgeToPolygons[i].Offset=hull->mEdgeToPolygons[i-1].Offset+hull->mEdgeToPolygons[i-1].Count;
    nxIceFree(sortedEdge);nxIceFree(sortedPolygon);nxIceFree(sortedPosition);
    if(!hull->mEdges)nxHullComputeEdges(hull);if(!hull->mEdgeToPolygons)nxHullComputeEdges(hull);if(!hull->mEdgePolygons)nxHullComputeEdges(hull);
    hull->mEdgeNormals=static_cast<IceMaths::Point*>(nxIceAlloc(hull->mNbEdges*sizeof(IceMaths::Point),NX_MEMORY_PERSISTENT));
    for(NxU32 i=0;i<hull->mNbEdges;++i){const NxU32 offset=hull->mEdgeToPolygons[i].Offset;
        const IceMaths::Point& a=hull->mPolygons[hull->mEdgePolygons[offset]].mPlane.n;
        const IceMaths::Point& b=hull->mPolygons[hull->mEdgePolygons[offset+1]].mPlane.n;
        IceMaths::Point n(float(double(b.x)+a.x),float(double(a.y)+b.y),float(double(a.z)+b.z));
        const double z=double(a.z)+b.z;
        const double square=(z*n.z+double(n.y)*n.y)+double(n.x)*n.x;
        if(square!=0){const double inverse=1.0/std::sqrt(square);n.x=float(double(n.x)*inverse);n.y=float(double(n.y)*inverse);n.z=float(double(n.z)*inverse);}
        hull->mEdgeNormals[i]=n;
    }
    return true;
}
bool nxHullComputeEdgeAxes(ConvexHull* hull)
{
    if(hull->mEdgeAxes){hull->mEdgeAxes->~Container();std::free(hull->mEdgeAxes);hull->mEdgeAxes=0;}
    IceCore::Container axes;if(!hull->mNbPolygons)nxHullComputePolygons(hull);
    for(NxU32 p=0;p<hull->mNbPolygons;++p){if(!hull->mPolygons)nxHullComputePolygons(hull);const HullPolygon& polygon=hull->mPolygons[p];
        for(NxU32 i=0;i<polygon.mNbVerts;++i){NxU32 a=polygon.mVRefs[i],b=polygon.mVRefs[(i+1)%polygon.mNbVerts];if(a>b){const NxU32 t=a;a=b;b=t;}
            const IceMaths::Point& va=hull->mVerts[a];const IceMaths::Point& vb=hull->mVerts[b];
            const double x=double(va.x)-vb.x,y=double(va.y)-vb.y,z=double(va.z)-vb.z;const float nz=float(z);
            const double square=(z*nz+y*y)+x*x;IceMaths::Point axis;
            if(square!=0&&!std::isnan(square)){const double inverse=1.0/std::sqrt(square);axis.Set(float(x*inverse),float(y*inverse),float(double(nz)*inverse));}
            else axis.Set(float(x),float(y),nz);
            nxIceAddUniqueAxis(&axes,&axis);
        }}
    void* memory=::operator new(sizeof(IceCore::Container),std::nothrow);
    hull->mEdgeAxes=memory?new(memory) IceCore::Container:0;
    if(hull->mEdgeAxes)hull->mEdgeAxes->Add(axes.GetEntries(),(axes.GetNbEntries()/3)*3);
    return true;
}
NxU32 nxHullSupportFace(ConvexHull* hull,const IceMaths::Point* axis,const float* pose,NxU32* kind)
{
    const IceMaths::Point local=nxHullLocalAxis(*axis,pose);double best;
    const NxU32 polygon=nxHullBestPolygon(hull,local,best);
    // Call boundaries spill the best score to binary32 as in001516.
    if(!hull->mNbEdges){best=float(best);nxHullComputeEdges(hull);}if(!hull->mEdgeNormals){best=float(best);nxHullComputeEdges(hull);}
    NxU32 edge=0xffffffff,i=0;
    if(hull->mNbEdges>=4)for(;i<hull->mNbEdges-3;i+=4)for(NxU32 j=0;j<4;++j){const IceMaths::Point& n=hull->mEdgeNormals[i+j];
        const double value=(double(local.x)*n.x+double(local.z)*n.z)+double(local.y)*n.y;if(value>best){best=float(value);edge=i+j;}}
    for(;i<hull->mNbEdges;++i){const double value=nxHullDotZXY(local,hull->mEdgeNormals[i]);if(value>best){best=float(value);edge=i;}}
    if(edge==0xffffffff){if(kind)*kind=0;return polygon;}
    if(kind)*kind=1;if(!hull->mEdges)nxHullComputeEdges(hull);if(!hull->mEdgeToPolygons)nxHullComputeEdges(hull);if(!hull->mEdgePolygons)nxHullComputeEdges(hull);
    const NxU32 offset=hull->mEdgeToPolygons[edge].Offset,a=hull->mEdgePolygons[offset],b=hull->mEdgePolygons[offset+1];
    // Second score is st0 at fcompp; the equal branch returns the second face.
    return nxHullDotZYX(local,hull->mPolygons[a].mPlane.n)>nxHullDotZYX(local,hull->mPolygons[b].mPlane.n)?a:b;
}
bool nxHullClimbSupportVertex(NxU32* index,const IceMaths::Point* axis,const IceMaths::Point* vertices,
    const Valencies* graph,NxU32 stamp,NxU32* visited)
{
    if(!vertices||!graph||!visited||!graph->mValencies||!graph->mOffsets||!graph->mAdjVerts)return false;
    NxU32 current=*index;visited[current]=stamp;
    float best=float((double(vertices[current].y)*axis->y+double(vertices[current].z)*axis->z)+double(axis->x)*vertices[current].x);
    for(;;){const NxU32 start=current;*index=current;const NxU32* neighbours=graph->mAdjVerts+graph->mOffsets[current];
        const NxU32 count=graph->mValencies[current];
        for(NxU32 i=0;i<count;++i){const NxU32 n=neighbours[i];if(visited[n]==stamp)continue;visited[n]=stamp;
            const double value=(double(vertices[n].y)*axis->y+double(vertices[n].z)*axis->z)+double(axis->x)*vertices[n].x;
            if(value>best){best=float(value);current=n;}}
        if(current==start)return true;
    }
}
