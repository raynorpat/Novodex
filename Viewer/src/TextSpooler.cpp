/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ViewerPlatform.h"
#include "TextSpooler.h"
#include <ODBlock.h>

extern void Fatal(const char * m);


TextSpooler::TextStatement::TextStatement(ODBlock * tBlock)
	{
	if (!tBlock) Fatal("?");
	tBlock->reset();
	if (tBlock->moreTerminals())
		text = tBlock->nextTerminal();
	else 
		Fatal("Missing text in T block.");

	x = 1;
	y = 1;
	size = 1;
	color[0] = 0.7f;
	color[1] = 0.7f;
	color[2] = 0.7f;
	timeUntilToDestroy = 0;	//infinite duration
	timeUntilShowNext = 0;	//infinite duration
	destroyAllShown = false;
	bool bShown = false;

	float temp;

	tBlock->reset();
	ODBlock * b;
	while(tBlock->moreSubBlocks())
		{
		b = tBlock->nextSubBlock();
		b->reset();
		if (b->moreTerminals())
			{
			sscanf(b->nextTerminal(),"%f", &temp);
			switch (b->ident()[0])
				{
				case 'X':
					x = temp;
					break;
				case 'Y':
					y = temp;
					break;
				case 'S'://size
					size = temp;
					break;
				case 'D'://duration
					timeUntilToDestroy = temp;
					break;
				case 'N'://next start
					timeUntilShowNext = temp;
					break;
				case 'E'://extended
					if (temp == 1)
						destroyAllShown = true;
					break;
				// SS: Todo, add a nice color vector
				case 'R'://red
					color[0] = temp;
					break;
				case 'G'://green
					color[1] = temp;
					break;
				case 'B'://blue
					color[2] = temp;
					break;
				}
			}
		}
	if (timeUntilShowNext == 1)	//magic 
		timeUntilShowNext = timeUntilToDestroy;
	}

TextSpooler::TRetVal TextSpooler::TextStatement::tick(float s)				//returns true when finished / can be deleted
	{
	//todo: do any font animation here...
	if(timeUntilShowNext != 0)		//not infinite duration
		{
		timeUntilShowNext-=s;
		if (timeUntilShowNext <= 0)
			{
			timeUntilShowNext = 0;
			return trSHOWNEXT;
			}
		}
	if(timeUntilToDestroy != 0)		//not infinite duration
		{
		timeUntilToDestroy-=s;
		if (timeUntilToDestroy <= 0)
			if (destroyAllShown)
				return trDESTROYALLSHOWN;
			else
				return trDESTROY;
		}
	return trHOLDNEXT;
	}


void  TextSpooler::TextStatement::render()
	{
	ViewerUi::draw(1,size,x,y,text,color);	//TODO: choose a better font/ add support for styles here.
	}

//=======================================================
//=======================================================


TextSpooler::TextSpooler()
	{
	startAfter_ = 0;
	nextFreeSlot = lastShownIndex = 0; 
	for (int i = 0; i<MAXTEXTS; i++)	
		texts[i] = NULL;
	}


void TextSpooler::load(ODBlock * tBlock)
	{
	tBlock->reset();
	while (tBlock->moreSubBlocks())
		{
		ODBlock * t = tBlock->nextSubBlock();
		if (t->ident()[0] == 'T' && nextFreeSlot < MAXTEXTS)
			{
			texts[nextFreeSlot] = new TextStatement(t);
			nextFreeSlot++;
			}
		}
	}

TextSpooler::~TextSpooler()
	{
	for (int i = 0; i<MAXTEXTS; i++)	
		sdelete(texts[i]);
	}


void TextSpooler::startAfter(float s)
	{
	startAfter_ = s;
	}

void TextSpooler::tick(float sec)
	{
	if (startAfter_ > 0)
		startAfter_ -= sec;
	else
		{
		int j;
		for (int i = 0; i<lastShownIndex; i++)
		if (texts[i])
			{
			switch (texts[i]->tick(sec))
				{
				case trDESTROYALLSHOWN:
					for (j = 0; j<=i; j++)
						sdelete(texts[j]);
					return;
				case trDESTROY:
					sdelete(texts[i]);
					break;
				}
			}
		if (texts[lastShownIndex])
		switch (texts[lastShownIndex]->tick(sec))
			{
			case trDESTROY://destory and show next.
				sdelete  (texts[lastShownIndex]);
				if (lastShownIndex < nextFreeSlot)
					lastShownIndex++;
				break;
			case trDESTROYALLSHOWN: //destroy all and show next
				for (j = 0; j<=lastShownIndex; j++)
					sdelete(texts[j]);
				if (lastShownIndex < nextFreeSlot)
					lastShownIndex++;
				break;
			case trSHOWNEXT:			//show next
				if (lastShownIndex < nextFreeSlot)
					lastShownIndex++;
				break;
			}
		}
	}

void TextSpooler::render()
	{
	ViewerUi::start();
	for (int i = 0; i<=lastShownIndex; i++)	
		if (texts[i])
			texts[i]->render();
	ViewerUi::end();
	}
