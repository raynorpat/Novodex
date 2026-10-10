#ifndef NX_READ_WRITE_LOCK_LIFETIME_H
#define NX_READ_WRITE_LOCK_LIFETIME_H
class ReadWriteLock;
// Genuine Scene link producer and owner teardown; Foundation allocator owns both levels.
ReadWriteLock* nxSceneLockCreate();
void nxSceneLockDestroy(void* link);
#endif
