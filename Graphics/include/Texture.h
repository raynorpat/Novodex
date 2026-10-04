#ifndef _SG_TEXTURE_H__
#define _SG_TEXTURE_H__
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include <string>

namespace SceneGraph
	{

class Texture
/*-------------------------------*\
| this is basically an image preprocessed
| to be suitable for rendering.
| wheather its bump, etc is spacified
| not here but in material.
\*-------------------------------*/
	{
	public:
	Texture();
	virtual ~Texture();

	void load(const char * fileName, bool filter = true);
	void activate();
	void create(bool filter = true);
	// Upload tightly packed, bottom-left RGBA pixels in the current GL context.
	void uploadRGBA(unsigned width, unsigned height, const unsigned char * pixels, bool filter = true);
	void destroy();
	inline unsigned int getDispListNum() {return DispListNum;}

	std::string name;
	unsigned int DispListNum;

	unsigned int width;
	unsigned int height;

	float originalAspect;
	private:
	bool pendingFilter;
	};
	};
#endif //__TEXTURE_H__
