/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include"Act.h"
#include"ActRecorder.h"

ActRecorder::ActRecorder(bool record, unsigned frameNumber)
	{
	recorderSetting = 0;
	recordFile = 0;

	set(record ? RS_RECORD : RS_PLAYBACK, frameNumber);
	}

void ActRecorder::set(NxI32 rs, unsigned frameNumber)
	{
	if (rs == RS_RECORD)
		{
		//recorder can only be turned on before 1st tick call
		if (frameNumber != 0 || recorderSetting != RS_OFF)
			return;
		//open a file to save the data to.
		//format:  [DWORD event timestamp][DWORD event type][event specific infos with fixed size]
		recordFile = fopen("c:\\viewerRecorder.bin", "wb");
		if (!recordFile)
			return;
		recorderSetting = rs;
		}
	else if (rs == RS_PLAYBACK)
		{
		//player can only be turned on before 1st tick call
		if (frameNumber != 0 || recorderSetting != RS_OFF)
			return;

		recordFile = fopen("c:\\viewerRecorder.bin", "rb");
		if (!recordFile)
			return;
		recorderSetting = rs;

		//read out the first header:
		if (2 != fread(nextPlaybackHeader,sizeof(NxU32), 2, recordFile))
			{
			//ran out of data, stop playback:
			set(RS_OFF, frameNumber);
			}

		}
	else if (rs == RS_OFF)
		{
		if (!recordFile)
			return;

		fclose(recordFile);
		recordFile = 0;

		recorderSetting = rs;
		}
	}

ActRecorder::~ActRecorder()
	{
	set(RS_OFF, 0);
	}

void ActRecorder::recordPick(NxVec3 & start, NxVec3 & end, bool pick, unsigned frameNumber)	//pick or drag.
	{
	if (recorderSetting != RS_RECORD)
		return;

	//format:  [DWORD event timestamp][DWORD event type][event specific infos with fixed size]
	NxU32 header[2];

	header[0] = frameNumber;
	header[1] = pick;

	fwrite(header,sizeof(NxU32),2,recordFile);
	fwrite(&start,sizeof(NxVec3),1,recordFile);
	fwrite(&end  ,sizeof(NxVec3),1,recordFile);
	}

void ActRecorder::recordUnpick(unsigned frameNumber)
	{
	if (recorderSetting != RS_RECORD)
		return;

	//format:  [DWORD event timestamp][DWORD event type][event specific infos with fixed size]
	NxU32 header[2];

	header[0] = frameNumber;
	header[1] = 2;
	fwrite(header,sizeof(NxU32),2,recordFile);
	}

void ActRecorder::playback(unsigned frameNumber)
	{
	if (recorderSetting != RS_PLAYBACK)
		return;

	//format:  [DWORD event timestamp][DWORD event type][event specific infos with fixed size]
	//is it time to execute the currently read event?
	while (frameNumber == nextPlaybackHeader[0])
		{
		//execute action 
		NxVec3 data[2];

		if (nextPlaybackHeader[1] == 2)
			Act::instance()->unpick();
		else
			{
			if (2 != fread(data,sizeof(NxVec3), 2, recordFile))
				{
				//ran out of data, stop playback:
				set(RS_OFF, frameNumber);
				return;
				}

			if (nextPlaybackHeader[1] == 0)
				{
				//0: drag

				//taken from lineDrag():
				Act::instance()->lineDrag(data[0], data[1]);
				/*
				lineStart = data[0];
				lineEnd = data[1];
				drag = true;
				*/
				}
			else //1: pick
				Act::instance()->linePick(data[0], data[1]);
			}

		//read out the next header:
		if (2 != fread(nextPlaybackHeader,sizeof(NxU32), 2, recordFile))
			{
			//ran out of data, stop playback:
			set(RS_OFF, frameNumber);
			return;
			}
		}
	}

NxI32 ActRecorder::getStatus()
	{
	return recorderSetting;
	}
