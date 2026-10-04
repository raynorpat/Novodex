#ifndef ACTRECORDER_H
#define ACTRECORDER_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/**
A class for user input event recording.
*/
class ActRecorder
	{
	enum RecorderSettings { RS_OFF = 0, RS_RECORD = 1, RS_PLAYBACK  = 2};
	NxI32 recorderSetting;
	FILE * recordFile;
	NxU32 nextPlaybackHeader[2];

	void set(NxI32 rs, unsigned frameNumber);

	public:
	/**
	record - record or playback
	*/
	ActRecorder(bool record, unsigned frameNumber);	
	~ActRecorder();

	/**
	notifications of the events that can be recorded:
	*/
	void recordPick(NxVec3 & start, NxVec3 & end, bool pick, unsigned frameNumber);	//pick or drag.
	void recordUnpick(unsigned frameNumber);

	/**
	playback of an event.  Frame number is the current frame in the simulation.
	*/
	void playback(unsigned frameNumber);

	/**
	status - returns a RecorderSettings.
	*/
	NxI32 getStatus();
	};
#endif