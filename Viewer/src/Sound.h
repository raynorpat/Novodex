/* NovodeX viewer sound. XAudio2 replaces the obsolete OpenAL/ALUT dependency. */
#ifndef __SOUND_H__
#define __SOUND_H__
#include "Nxf.h"
#include "NxArray.h"
#include "NxVec3.h"
#ifdef INCLUDE_SOUND
#include <memory>
#include <vector>

// Keep the legacy public scalar names without depending on OpenAL headers.
typedef char ALbyte;
typedef unsigned char ALubyte;
typedef unsigned short ALushort;
typedef unsigned ALuint;
typedef int ALint;
typedef int ALsizei;
typedef float ALfloat;

namespace Sound {
bool displayALError(ALbyte * text, ALint errorcode);
struct WAVE_Struct {
    ALubyte riff[4]; ALsizei riffSize; ALubyte wave[4]; ALubyte fmt[4];
    ALuint fmtSize; ALushort Format; ALushort Channels; ALuint SamplesPerSec;
    ALuint BytesPerSec; ALushort BlockAlign; ALushort BitsPerSample;
    ALubyte data[4]; ALuint dataSize;
};

struct WaveData {
    unsigned formatTag, channels, sampleRate, byteRate, blockAlign, bitsPerSample;
    std::vector<unsigned char> samples;
};

class Buffer {
public:
    Buffer();
    ~Buffer();
    bool loadWAV(const char * file);
    void free();
    const WaveData * waveData() const { return data.get(); }
    char fileName[256];
    ALuint id;
    bool loaded;
private:
    std::shared_ptr<const WaveData> data;
    friend class Source;
};

class Listener {
public:
    Listener();
    void update();
    float gain;
    NxVec3 position, velocity;
    ALfloat listenerOri[6];
};

class Manager {
public:
    static bool open();
    static void close();
    static Listener * getListener() { return &listener; }
    static int addBuffer(const char * fileName);
    static Buffer * getBuffer(unsigned i) { return i < buffers.size() ? &buffers[i] : 0; }
    static bool isRunning();
    static const char * lastError();
private:
    static Listener listener;
    static std::vector<Buffer> buffers;
    static bool running;
};

class Source {
public:
    Source();
    ~Source();
    void updateBuffer();
    void queueBuffer(unsigned queueBufferIndex);
    void updateAll();
    void replay();
    void play();
    void stop();
    unsigned queuedBuffers() const;
    unsigned long long samplesPlayed() const;
    NxVec3 position, velocity;
    NxReal gain, gainDefault, pitch, pitchDefault;
    unsigned bufferIndex;
    bool loop, relative, playing;
private:
    struct Backend;
    std::unique_ptr<Backend> backend;
    void releaseVoice();
    bool prepareVoice();
    void refreshPlaying();
    void managerClosed();
    Source(const Source &) = delete;
    Source & operator=(const Source &) = delete;
    friend class Manager;
};
}
#endif
#endif
