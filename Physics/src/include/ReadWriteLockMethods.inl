// phys_fn_002362 (0x0005b700). The critical section is entered and not left --
// unlock is what leaves it -- and the interlocked exchange at 0x0005b716 claims
// the flag only if it was clear. Returns a literal true at 0x0005b727.
bool ReadWriteLock::lock()
	{
	ReadWriteLockData* data = static_cast<ReadWriteLockData*>(mData);
	EnterCriticalSection(&data->mCriticalSection);
	InterlockedCompareExchange(&data->mOwned, 1, 0);
	data->mOwnerThreadId = GetCurrentThreadId();
	return true;
	}

// phys_fn_002364 (0x0005b730). 0x0005b745 claims the flag; if it was already
// claimed, 0x0005b755 compares the recorded owner against this thread and only
// then falls through to the acquire at 0x0005b760. So it is reentrant on the
// owning thread and fails, at 0x0005b75c, only for another thread.
bool ReadWriteLock::tryLock()
	{
	ReadWriteLockData* data = static_cast<ReadWriteLockData*>(mData);
	if(InterlockedCompareExchange(&data->mOwned, 1, 0) != 0
		&& data->mOwnerThreadId != GetCurrentThreadId())
		return false;

	EnterCriticalSection(&data->mCriticalSection);
	InterlockedCompareExchange(&data->mOwned, 1, 0);
	data->mOwnerThreadId = GetCurrentThreadId();
	return true;
	}

// phys_fn_002366 (0x0005b790). Releases the flag and leaves the critical
// section, in that order, and checks nothing: unlocking a lock this thread does
// not hold is not refused here.
bool ReadWriteLock::unlock()
	{
	ReadWriteLockData* data = static_cast<ReadWriteLockData*>(mData);
	InterlockedCompareExchange(&data->mOwned, 0, 1);
	LeaveCriticalSection(&data->mCriticalSection);
	return true;
	}
