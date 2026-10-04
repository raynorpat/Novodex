/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef __TEXTSPOOLER_H__
#define __TEXTSPOOLER_H__

class ODBlock;

class TextSpooler
	{
	public:
	enum TRetVal { trHOLDNEXT, trSHOWNEXT, trDESTROY, trDESTROYALLSHOWN  };
	class TextStatement
		{
		char * text;
		float x, y, size;
		float color[3];
		float timeUntilToDestroy;
		float timeUntilShowNext;
		bool destroyAllShown;
		public:
		TextStatement(ODBlock * tBlock);
		TRetVal tick(float s);				//call only when visible; returns true when finished / can be deleted

		void render();						//call only when visible


		};
	private:

	float startAfter_;					//decrement this until zero in tick, then start.
#define MAXTEXTS 30
	TextStatement * texts[MAXTEXTS];			//oldest shown text
	unsigned nextFreeSlot;				//index after last filled slot.
	unsigned lastShownIndex;			//most recent shown text (child of root)

	public:
	TextSpooler();
	void load(ODBlock * textBlock);
	~TextSpooler();
	void startAfter(float s);			//start with messages after s seconds from this call on.
	void tick(float sec);
	void render();
	};
#endif //__TEXTSPOOLER_H__