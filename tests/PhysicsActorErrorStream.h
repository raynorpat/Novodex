#ifndef NX_PHYSICS_ACTOR_ERROR_STREAM_H
#define NX_PHYSICS_ACTOR_ERROR_STREAM_H

// The Foundation error stream the actor staged-pair targets hand to
// NxCreatePhysicsSDK, and the write-lock holder their G1 cases use.
//
// The stream is silent until a target enables it, so every line a target
// printed before it passed a stream is unchanged; once enabled it prints each
// report the pair's Foundation delivers (code, line, file and the formatted
// message), which is what makes the actor rows' G1/E1 reports part of the
// compared transcript on both sides.

#include "NxUserOutputStream.h"

#include <stdio.h>

class NxActorErrorStream : public NxUserOutputStream
	{
	public:
	NxActorErrorStream(const char* prefix) : enabled(false), reports(0), mPrefix(prefix) {}

	void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		if(!enabled) return;
		++reports;
		printf("%s report=%d.%x.%s.%s\n", mPrefix, static_cast<int>(code), line,
			file ? file : "(null)", message ? message : "(null)");
		}

	NxAssertResponse reportAssertViolation(const char* message, const char* file, int line)
		{
		if(enabled)
			printf("%s assert=%x.%s.%s\n", mPrefix, line, file ? file : "(null)",
				message ? message : "(null)");
		return NX_AR_CONTINUE;
		}

	void print(const char* message)
		{
		if(enabled) printf("%s print=%s\n", mPrefix, message ? message : "(null)");
		}

	bool enabled;
	unsigned reports;

	private:
	const char* mPrefix;
	};

// The actor's write link ([actor+0xc]) points at the scene's lock block; the
// write try (oracle 002364, 0x5b730) fails when the flag word at +0x18 is set
// and the owner word at +0x1c is not the calling thread. Thread id 0 is never
// a thread's id, so flag 1 with owner 0 makes every write try on this thread
// fail without any thread or critical section being involved. A read lock
// (002362) or an unlock rewrites both words, so hold() is applied right
// before each guarded call and release() puts the saved words back.
class NxActorWriteLockHolder
	{
	public:
	NxActorWriteLockHolder(const void* actor)
		{
		const unsigned char* link = *reinterpret_cast<unsigned char* const*>(
			static_cast<const unsigned char*>(actor) + 0x0c);
		mState = reinterpret_cast<unsigned*>(
			*reinterpret_cast<unsigned char* const*>(link) + 0x18);
		mSaved[0] = mState[0];
		mSaved[1] = mState[1];
		}

	void hold() { mState[0] = 1; mState[1] = 0; }
	void release() { mState[0] = mSaved[0]; mState[1] = mSaved[1]; }
	unsigned flag() const { return mState[0]; }
	unsigned owner() const { return mState[1]; }

	private:
	unsigned* mState;
	unsigned mSaved[2];
	};

#endif
