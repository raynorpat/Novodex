// Ordinary equivalents of001651/001653. Original backend1 bodies remain
// authoritative for argument order, temporary stores and per-element grouping.
#include <cmath>
#include <cstring>

static void nxMeshNormalise(IceMaths::Point& n)
{
    const double square = (double(n.x) * n.x + double(n.y) * n.y)
        + double(n.z) * n.z;
    if(square != 0.0)
    {
        const double inverse = 1.0 / std::sqrt(square);
        n.x = float(inverse * n.x);
        n.y = float(inverse * n.y);
        n.z = float(inverse * n.z);
    }
}

static void nxMeshNormalRefs(const MESHNORMALSCREATE& create, NxU32 face,
    NxU32 refs[3])
{
    for(NxU32 corner = 0; corner < 3; ++corner)
    {
        refs[corner] = create.DFaces ? create.DFaces[face * 3 + corner]
            : create.WFaces ? create.WFaces[face * 3 + corner] : corner;
    }
}

bool nxMeshNormalsCompute(MeshNormals* normals, const MESHNORMALSCREATE* create)
{
    if(!create->Verts)
        return false;
    IceMaths::Point* faces = create->FaceNormals;
    if(!faces)
        faces = static_cast<IceMaths::Point*>(nxIceAlloc(create->NbFaces * 12u,
            NX_MEMORY_PERSISTENT));
    if(!faces)
        return false;
    IceMaths::Point* vertices = create->VertexNormals;
    if(!vertices)
        vertices = static_cast<IceMaths::Point*>(nxIceAlloc(create->NbVerts * 12u,
            NX_MEMORY_PERSISTENT));
    if(!vertices)
        return false;
    // Store ownership only after both allocations succeed, as001651 does.
    if(!create->FaceNormals)
        normals->mFaceNormals = faces;
    if(!create->VertexNormals)
        normals->mVertexNormals = vertices;
    for(NxU32 face = 0; face < create->NbFaces; ++face)
    {
        NxU32 refs[3];
        nxMeshNormalRefs(*create, face, refs);
        const IceMaths::Point& p0 = create->Verts[refs[0]];
        const IceMaths::Point& p1 = create->Verts[refs[1]];
        const IceMaths::Point& p2 = create->Verts[refs[2]];
        const double ax = double(p0.x) - p2.x;
        const double ay = double(p0.y) - p2.y;
        const double az = double(p0.z) - p2.z;
        const double bx = double(p1.x) - p2.x;
        const double by = double(p1.y) - p2.y;
        const double bz = double(p1.z) - p2.z;
        faces[face].x = float(az * by - bz * ay);
        faces[face].y = float(bz * ax - bx * az);
        faces[face].z = float(bx * ay - ax * by);
        nxMeshNormalise(faces[face]);
    }
    std::memset(vertices, 0, create->NbVerts * 12u);
    for(NxU32 face = 0; face < create->NbFaces; ++face)
    {
        NxU32 refs[3];
        nxMeshNormalRefs(*create, face, refs);
        // Stack+20/+24/+28: r0,r2,r1. All three actual002144 consumers
        // receive an already narrowed angle, then keep only x unspilled.
        const NxU32 corners[3] = {refs[0], refs[2], refs[1]};
        const IceMaths::Point& normal = faces[face];
        for(NxU32 corner = 0; corner < 3; ++corner)
        {
            IceMaths::Point& target = vertices[corners[corner]];
            if(create->WeightByAngle)
            {
                const NxReal angle = nxSmoothNormalsAngleAtVertex(corners[corner],
                    corners, reinterpret_cast<const NxVec3*>(create->Verts));
                const double x = double(angle) * normal.x;
                const float y = float(double(angle) * normal.y);
                const float z = float(double(angle) * normal.z);
                target.x = float(x + target.x);
                target.y = float(double(y) + target.y);
                target.z = float(double(z) + target.z);
            }
            else
            {
                target.x = float(double(target.x) + normal.x);
                target.y = float(double(target.y) + normal.y);
                target.z = float(double(target.z) + normal.z);
            }
        }
    }
    for(NxU32 vertex = 0; vertex < create->NbVerts; ++vertex)
        nxMeshNormalise(vertices[vertex]);
    return true;
}

// Each output element has its own historical term order. The inverse uses
// the actual vendor PR routine, whose borrowed input must be rotation/translation.
static const unsigned nxPoseTermOrder[2][16][4] = {
    {
        {1, 2, 3, 0},
        {0, 2, 3, 1},
        {0, 1, 3, 2},
        {0, 1, 2, 3},
        {3, 1, 2, 0},
        {1, 3, 0, 2},
        {2, 3, 0, 1},
        {0, 1, 2, 3},
        {3, 1, 2, 0},
        {1, 3, 0, 2},
        {2, 3, 0, 1},
        {0, 1, 2, 3},
        {3, 1, 2, 0},
        {1, 3, 0, 2},
        {2, 3, 0, 1},
        {0, 1, 2, 3},
    },
    {
        {1, 2, 3, 0},
        {2, 1, 0, 3},
        {1, 0, 2, 3},
        {1, 2, 0, 3},
        {3, 0, 1, 2},
        {3, 2, 0, 1},
        {3, 1, 0, 2},
        {1, 2, 3, 0},
        {3, 0, 1, 2},
        {3, 2, 0, 1},
        {3, 1, 0, 2},
        {1, 2, 3, 0},
        {3, 0, 1, 2},
        {3, 2, 0, 1},
        {3, 1, 0, 2},
        {1, 2, 3, 0},
    },
};

static void nxPoseProduct(IceMaths::Matrix4x4& output,
    const IceMaths::Matrix4x4& pose, const IceMaths::Matrix4x4& inverse,
    unsigned which)
{
    for(unsigned row = 0; row < 4; ++row)
    {
        for(unsigned column = 0; column < 4; ++column)
        {
            const unsigned* order = nxPoseTermOrder[which][row * 4 + column];
            double sum = double(inverse.m[order[0]][column]) * pose.m[row][order[0]];
            for(unsigned term = 1; term < 4; ++term)
                sum += double(inverse.m[order[term]][column]) * pose.m[row][order[term]];
            output.m[row][column] = float(sum);
        }
    }
}

void __cdecl nxIcePosePair(IceMaths::Matrix4x4* relative0,
    IceMaths::Matrix4x4* relative1, const IceMaths::Matrix4x4* pose0,
    const IceMaths::Matrix4x4* pose1)
{
    IceMaths::Matrix4x4 inverse0, inverse1, result;
    if(pose0)
        IceMaths::InvertPRMatrix(inverse0, *pose0);
    else
        std::memcpy(&inverse0, nxIceIdentityPoses() + 9, sizeof(inverse0));
    if(pose1)
        IceMaths::InvertPRMatrix(inverse1, *pose1);
    else
        std::memcpy(&inverse1, nxIceIdentityPoses() + 9, sizeof(inverse1));
    if(relative0)
    {
        if(pose0)
            nxPoseProduct(result, *pose0, inverse1, 0);
        else
            std::memcpy(&result, &inverse1, sizeof(result));
        std::memcpy(relative0, &result, sizeof(result));
    }
    if(relative1)
    {
        if(pose1)
            nxPoseProduct(result, *pose1, inverse0, 1);
        else
            std::memcpy(&result, &inverse0, sizeof(result));
        std::memcpy(relative1, &result, sizeof(result));
    }
}
