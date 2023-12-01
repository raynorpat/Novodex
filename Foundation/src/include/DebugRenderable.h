#ifndef NX_FOUNDATION_DEBUGRENDERABLE
#define NX_FOUNDATION_DEBUGRENDERABLE
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NxDebugRenderable.h"
#include "NxAllocateable.h"
#include "NxArray.h"


/**
Default implementation of NxDebugRenderable class.
*/
namespace NxFoundation
	{
//typedef NxArraySDK<NxU32> GenericArray;
typedef NxArraySDK<NxDebugPoint> PointArray;
typedef NxArraySDK<NxDebugLine> LineArray;
typedef NxArraySDK<NxDebugTriangle> TrisArray;

class DebugRenderable : public NxAllocateable, public NxDebugRenderable
	{
	public:
	DebugRenderable();
	~DebugRenderable();

	virtual	NxU32 getNbPoints() const;
	virtual	const NxDebugPoint* getPoints() const;
	virtual	NxU32 getNbLines() const;
	virtual	const NxDebugLine* getLines() const;
	virtual	NxU32 getNbTriangles() const;
	virtual	const NxDebugTriangle* getTriangles() const;
	virtual	void addPoint(const NxVec3& p, NxU32 color);
	virtual	void addLine(const NxVec3& p0, const NxVec3& p1, NxU32 color);
	virtual	void addTriangle(const NxVec3& p0, const NxVec3& p1, const NxVec3& p2, NxU32 color);
	virtual	void clear();
	virtual	void addOBB(const NxBox& box, NxU32 color, bool render_frame);
	virtual	void addAABB(const NxBounds3& bounds);
	virtual	void addArrow(const NxVec3 & position, const NxVec3 & direction, NxReal length, NxReal scale, NxU32 color);
	virtual	void addBasis(const NxVec3 & position, const NxMat33 & columns, const NxVec3 & lengths, NxReal scale, NxU32 colors[3]);
	virtual	void addCircle(NxU32 nbSegments, const NxMat34& matrix, NxU32 color, NxF32 radius, bool semicircle);

	private:
	PointArray	pointsArray;
	LineArray	linesArray;
	TrisArray	trianglesArray;
	};

	}
#endif
