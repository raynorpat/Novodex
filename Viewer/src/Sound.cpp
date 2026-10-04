#include "Sound.h"
#ifdef INCLUDE_SOUND
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>
#include <x3daudio.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace Sound {
namespace {
std::string errorText;
const float maxPitch = 16.0f;

void error(const char * text) { errorText = text; }
bool error(const char * operation, HRESULT result)
{
    char text[160];
    std::snprintf(text, sizeof(text), "%s failed (HRESULT 0x%08lx)", operation, static_cast<unsigned long>(result));
    errorText = text;
    return false;
}

struct EngineCallback : IXAudio2EngineCallback {
    std::atomic<HRESULT> failure;
    EngineCallback() : failure(S_OK) {}
    void STDMETHODCALLTYPE OnProcessingPassStart() override {}
    void STDMETHODCALLTYPE OnProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnCriticalError(HRESULT result) override { failure.store(result); }
};
struct EngineState {
    IXAudio2 * engine;
    IXAudio2MasteringVoice * master;
    X3DAUDIO_HANDLE spatial;
    unsigned channels;
    DWORD comThread;
    EngineCallback callback;
    EngineState() : engine(0), master(0), channels(0), comThread(0) {}
} audio;

std::vector<Source *> & sources()
{
    static std::vector<Source *> live;
    return live;
}

unsigned read16(const std::vector<unsigned char> & bytes, size_t i)
{ return bytes[i] | (static_cast<unsigned>(bytes[i+1]) << 8); }
unsigned read32(const std::vector<unsigned char> & bytes, size_t i)
{ return read16(bytes,i) | (read16(bytes,i+2) << 16); }
bool fourcc(const std::vector<unsigned char> & bytes, size_t i, const char * id)
{ return std::memcmp(bytes.data()+i,id,4) == 0; }

bool readWave(const char * file, WaveData & result)
{
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    if (!input) { error("Cannot open WAV file"); return false; }
    const std::streamoff length = input.tellg();
    if (length < 12 || length > XAUDIO2_MAX_BUFFER_BYTES) { error("Invalid WAV file size"); return false; }
    input.seekg(0);
    std::vector<unsigned char> bytes(static_cast<size_t>(length));
    if (!input.read(reinterpret_cast<char *>(bytes.data()),length)) { error("Truncated WAV file"); return false; }
    if (!fourcc(bytes,0,"RIFF") || !fourcc(bytes,8,"WAVE")) { error("Expected RIFF WAVE file"); return false; }
    const unsigned long long end64 = static_cast<unsigned long long>(read32(bytes,4)) + 8;
    if (end64 < 12 || end64 > bytes.size()) { error("Invalid RIFF length"); return false; }
    const size_t end = static_cast<size_t>(end64);
    size_t format = 0, formatSize = 0, data = 0, dataSize = 0;
    for (size_t offset = 12; offset < end;) {
        if (end-offset < 8) { error("Truncated WAV chunk header"); return false; }
        const unsigned size = read32(bytes,offset+4);
        const unsigned long long next = static_cast<unsigned long long>(offset) + 8 + size + (size & 1);
        if (next > end) { error("WAV chunk exceeds RIFF length"); return false; }
        if (fourcc(bytes,offset,"fmt ")) {
            if (format) { error("Duplicate WAV format chunk"); return false; }
            format = offset+8; formatSize = size;
        } else if (fourcc(bytes,offset,"data")) {
            if (data) { error("Duplicate WAV data chunk"); return false; }
            data = offset+8; dataSize = size;
        }
        offset = static_cast<size_t>(next);
    }
    if (!format || formatSize < 16 || !data || !dataSize) { error("WAV needs format and nonempty data chunks"); return false; }
    unsigned tag = read16(bytes,format);
    result.channels = read16(bytes,format+2);
    result.sampleRate = read32(bytes,format+4);
    result.byteRate = read32(bytes,format+8);
    result.blockAlign = read16(bytes,format+12);
    result.bitsPerSample = read16(bytes,format+14);
    if (tag == WAVE_FORMAT_EXTENSIBLE) {
        if (formatSize < 40 || read16(bytes,format+16) < 22 ||
            static_cast<size_t>(read16(bytes,format+16))+18 > formatSize) {
            error("Invalid extensible WAV format"); return false;
        }
        const unsigned validBits = read16(bytes,format+18);
        const unsigned char suffix[] = {0,0,16,0,128,0,0,170,0,56,155,113};
        if (!validBits || validBits > result.bitsPerSample ||
            std::memcmp(bytes.data()+format+28,suffix,sizeof(suffix))) {
            error("Invalid extensible WAV subformat"); return false;
        }
        tag = read32(bytes,format+24);
        if (tag == WAVE_FORMAT_IEEE_FLOAT && validBits != 32) { error("Invalid float WAV precision"); return false; }
    }
    if ((tag != WAVE_FORMAT_PCM && tag != WAVE_FORMAT_IEEE_FLOAT) ||
        result.channels < 1 || result.channels > 2 ||
        result.sampleRate < XAUDIO2_MIN_SAMPLE_RATE || result.sampleRate > XAUDIO2_MAX_SAMPLE_RATE ||
        (result.bitsPerSample != 8 && result.bitsPerSample != 16 && result.bitsPerSample != 24 && result.bitsPerSample != 32) ||
        (tag == WAVE_FORMAT_IEEE_FLOAT && result.bitsPerSample != 32) ||
        result.blockAlign != result.channels * (result.bitsPerSample / 8) ||
        result.byteRate != static_cast<unsigned long long>(result.sampleRate) * result.blockAlign ||
        dataSize % result.blockAlign != 0) {
        error("Unsupported or inconsistent WAV format"); return false;
    }
    if (tag == WAVE_FORMAT_IEEE_FLOAT) {
        for (size_t i = data; i < data+dataSize; i += sizeof(float)) {
            float sample;
            std::memcpy(&sample,bytes.data()+i,sizeof(sample));
            if (!std::isfinite(sample)) { error("Nonfinite WAV float sample"); return false; }
        }
    }
    result.formatTag = tag;
    result.samples.assign(bytes.begin()+data,bytes.begin()+data+dataSize);
    return true;
}

float finite(float value, float fallback) { return std::isfinite(value) ? value : fallback; }
float volume(float value) { return std::max(0.0f,std::min(finite(value,0),XAUDIO2_MAX_VOLUME_LEVEL)); }
float coordinate(NxReal value) { return static_cast<float>(std::max(-1000000.0,std::min(std::isfinite(value) ? static_cast<double>(value) : 0.0,1000000.0))); }
X3DAUDIO_VECTOR vector(const NxVec3 & v) { X3DAUDIO_VECTOR r = {coordinate(v.x),coordinate(v.y),-coordinate(v.z)}; return r; }
X3DAUDIO_VECTOR unit(X3DAUDIO_VECTOR v, X3DAUDIO_VECTOR fallback)
{
    const float length = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    if (!std::isfinite(length) || length < 0.00001f) return fallback;
    v.x /= length; v.y /= length; v.z /= length; return v;
}
float dot(X3DAUDIO_VECTOR a, X3DAUDIO_VECTOR b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
X3DAUDIO_LISTENER spatialListener(const Listener & listener, bool relative)
{
    X3DAUDIO_LISTENER result = {};
    const X3DAUDIO_VECTOR front = {0,0,1}, top = {0,1,0};
    if (relative) { result.OrientFront = front; result.OrientTop = top; return result; }
    result.Position = vector(listener.position); result.Velocity = vector(listener.velocity);
    X3DAUDIO_VECTOR direction = {listener.listenerOri[0],listener.listenerOri[1],-listener.listenerOri[2]};
    X3DAUDIO_VECTOR up = {listener.listenerOri[3],listener.listenerOri[4],-listener.listenerOri[5]};
    result.OrientFront = unit(direction,front);
    const float projection = dot(up,result.OrientFront);
    up.x -= projection*result.OrientFront.x; up.y -= projection*result.OrientFront.y; up.z -= projection*result.OrientFront.z;
    if (!std::isfinite(dot(up,up)) || dot(up,up) < 0.00001f) {
        up = std::fabs(result.OrientFront.y) < 0.9f ? top : X3DAUDIO_VECTOR{1,0,0};
        const float p = dot(up,result.OrientFront);
        up.x -= p*result.OrientFront.x; up.y -= p*result.OrientFront.y; up.z -= p*result.OrientFront.z;
    }
    result.OrientTop = unit(up,top);
    return result;
}
bool sameFormat(const WaveData & a, const WaveData & b)
{
    return a.formatTag==b.formatTag && a.channels==b.channels && a.sampleRate==b.sampleRate &&
        a.blockAlign==b.blockAlign && a.bitsPerSample==b.bitsPerSample;
}
}

bool displayALError(ALbyte * text, ALint code)
{ return error(text ? text : "Audio",static_cast<HRESULT>(code)); }

Buffer::Buffer() : id(~0u), loaded(false) { fileName[0] = 0; }
Buffer::~Buffer() { free(); }
bool Buffer::loadWAV(const char * file)
{
    if (loaded) return false;
    if (!file || !*file || std::strlen(file)>=sizeof(fileName)) { error("Invalid WAV filename"); return false; }
    try {
        std::shared_ptr<WaveData> decoded(new WaveData);
        if (!readWave(file,*decoded)) return false;
        data = decoded;
        std::strcpy(fileName,file);
        static unsigned nextId = 0;
        id = nextId++;
        loaded = true;
        return true;
    } catch (const std::bad_alloc &) { error("Not enough memory to load WAV"); return false; }
      catch (const std::length_error &) { error("WAV allocation exceeds capacity"); return false; }
}
void Buffer::free() { data.reset(); loaded = false; id = ~0u; fileName[0] = 0; }

Listener::Listener() : gain(1), position(0,0,0), velocity(0,0,0)
{
    listenerOri[0]=0; listenerOri[1]=0; listenerOri[2]=-1;
    listenerOri[3]=0; listenerOri[4]=1; listenerOri[5]=0;
}
void Listener::update()
{
    if (!Manager::isRunning()) return;
    const HRESULT hr = audio.master->SetVolume(volume(gain));
    if (FAILED(hr)) error("Listener gain",hr);
    // Camera-only movement also changes sources whose world poses did not move.
    const std::vector<Source *> & live = sources();
    for (size_t i = 0; i < live.size(); ++i) live[i]->updateAll();
}

Listener Manager::listener;
std::vector<Buffer> Manager::buffers;
bool Manager::running = false;
bool Manager::isRunning()
{
    const HRESULT failure = audio.callback.failure.load();
    if (running && FAILED(failure)) { error("XAudio2 device",failure); running = false; }
    return running;
}
const char * Manager::lastError() { return errorText.c_str(); }
bool Manager::open()
{
    if (isRunning()) return true;
    close(); errorText.clear();
    const HRESULT com = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (SUCCEEDED(com)) audio.comThread = GetCurrentThreadId();
    else if (com != RPC_E_CHANGED_MODE) return error("COM initialization",com);
    HRESULT hr = XAudio2Create(&audio.engine,0,XAUDIO2_DEFAULT_PROCESSOR);
    if (SUCCEEDED(hr)) hr = audio.engine->RegisterForCallbacks(&audio.callback);
    if (SUCCEEDED(hr)) hr = audio.engine->CreateMasteringVoice(&audio.master,
        XAUDIO2_DEFAULT_CHANNELS,XAUDIO2_DEFAULT_SAMPLERATE,XAUDIO2_NO_VIRTUAL_AUDIO_CLIENT);
    DWORD mask = 0;
    if (SUCCEEDED(hr)) hr = audio.master->GetChannelMask(&mask);
    if (SUCCEEDED(hr)) hr = X3DAudioInitialize(mask,X3DAUDIO_SPEED_OF_SOUND,audio.spatial);
    if (FAILED(hr)) { error("Audio initialization",hr); close(); return false; }
    XAUDIO2_VOICE_DETAILS details = {};
    audio.master->GetVoiceDetails(&details);
    audio.channels = details.InputChannels;
    if (!audio.channels || audio.channels>XAUDIO2_MAX_AUDIO_CHANNELS) {
        error("Invalid audio output channel count"); close(); return false;
    }
    running = true;
    listener.update();
    return isRunning();
}
void Manager::close()
{
    running = false;
    const std::vector<Source *> & live = sources();
    for (size_t i = 0; i < live.size(); ++i) live[i]->managerClosed();
    buffers.clear();
    if (audio.master) { audio.master->DestroyVoice(); audio.master=0; }
    if (audio.engine) { audio.engine->UnregisterForCallbacks(&audio.callback); audio.engine->Release(); audio.engine=0; }
    if (audio.comThread == GetCurrentThreadId()) CoUninitialize();
    audio.comThread = 0; audio.channels = 0;
    audio.callback.failure.store(S_OK);
}
int Manager::addBuffer(const char * file)
{
    if (!isRunning() || !file) return -1;
    for (size_t i = 0; i < buffers.size(); ++i)
        if (buffers[i].loaded && std::strcmp(buffers[i].fileName,file)==0) return static_cast<int>(i);
    Buffer buffer;
    if (!buffer.loadWAV(file)) return -1;
    try { buffers.push_back(buffer); }
    catch (const std::bad_alloc &) { error("Not enough memory for sound buffer"); return -1; }
    return static_cast<int>(buffers.size()-1);
}

struct Source::Backend {
    IXAudio2SourceVoice * voice;
    std::vector<std::shared_ptr<const WaveData> > sequence;
    std::shared_ptr<const WaveData> joined;
    std::vector<float> matrix;
    bool started, looping, exitPending;
    Backend() : voice(0), started(false), looping(false), exitPending(false) {}
};
Source::Source() : position(0,0,0), velocity(0,0,0), gain(1), gainDefault(1),
    pitch(1), pitchDefault(1), bufferIndex(~0u), loop(false), relative(false), playing(false), backend(new Backend)
{ sources().push_back(this); }
Source::~Source()
{
    releaseVoice();
    std::vector<Source *> & live = sources();
    live.erase(std::remove(live.begin(),live.end(),this),live.end());
}
void Source::releaseVoice()
{
    // DestroyVoice waits for audio reads to stop before PCM ownership is released.
    if (backend->voice) { backend->voice->DestroyVoice(); backend->voice=0; }
    backend->joined.reset(); backend->started=false; backend->looping=false; backend->exitPending=false; playing=false;
}
void Source::managerClosed()
{ releaseVoice(); backend->sequence.clear(); bufferIndex=~0u; }
void Source::updateBuffer()
{
    releaseVoice(); backend->sequence.clear();
    Buffer * buffer = Manager::getBuffer(bufferIndex);
    if (Manager::isRunning() && buffer && buffer->loaded && buffer->data)
        backend->sequence.push_back(buffer->data);
}
bool Source::prepareVoice()
{
    if (!Manager::isRunning()) return false;
    if (backend->sequence.empty()) updateBuffer();
    if (backend->sequence.empty()) return false;
    const WaveData & first = *backend->sequence[0];
    WAVEFORMATEX format = {};
    format.wFormatTag=static_cast<WORD>(first.formatTag); format.nChannels=static_cast<WORD>(first.channels);
    format.nSamplesPerSec=first.sampleRate; format.nAvgBytesPerSec=first.byteRate;
    format.nBlockAlign=static_cast<WORD>(first.blockAlign); format.wBitsPerSample=static_cast<WORD>(first.bitsPerSample);
    HRESULT hr = audio.engine->CreateSourceVoice(&backend->voice,&format,0,maxPitch);
    if (FAILED(hr)) return error("Create source voice",hr);
    backend->looping = loop;
    backend->exitPending = false;
    const WaveData * clip = &first;
    try {
        backend->matrix.resize(first.channels*audio.channels);
        if (loop && backend->sequence.size()>1) {
            std::shared_ptr<WaveData> joined(new WaveData(first));
            for (size_t i = 1; i < backend->sequence.size(); ++i) {
                const std::vector<unsigned char> & samples = backend->sequence[i]->samples;
                if (joined->samples.size()>XAUDIO2_MAX_BUFFER_BYTES-samples.size()) {
                    error("Loop queue exceeds XAudio2 buffer limit"); releaseVoice(); return false;
                }
                joined->samples.insert(joined->samples.end(),samples.begin(),samples.end());
            }
            backend->joined=joined; clip=joined.get();
        }
        const size_t count = loop ? 1 : backend->sequence.size();
        for (size_t i = 0; i < count; ++i) {
            if (!loop) clip=backend->sequence[i].get();
            XAUDIO2_BUFFER buffer = {};
            buffer.AudioBytes=static_cast<UINT32>(clip->samples.size()); buffer.pAudioData=clip->samples.data();
            if (loop) buffer.LoopCount=XAUDIO2_LOOP_INFINITE;
            if (i+1==count) buffer.Flags=XAUDIO2_END_OF_STREAM;
            hr=backend->voice->SubmitSourceBuffer(&buffer);
            if (FAILED(hr)) { error("Submit sound buffer",hr); releaseVoice(); return false; }
        }
    } catch (const std::bad_alloc &) { error("Not enough memory for sound queue"); releaseVoice(); return false; }
      catch (const std::length_error &) { error("Sound queue exceeds capacity"); releaseVoice(); return false; }
    return true;
}
void Source::queueBuffer(unsigned index)
{
    if (!Manager::isRunning()) return;
    Buffer * buffer = Manager::getBuffer(index);
    if (!buffer || !buffer->loaded || !buffer->data) return;
    if (backend->sequence.size()>=XAUDIO2_MAX_QUEUED_BUFFERS) { error("Sound queue is full"); return; }
    if (!backend->sequence.empty() && !sameFormat(*backend->sequence[0],*buffer->data)) {
        error("Queued sounds must have matching formats"); return;
    }
    refreshPlaying();
    const bool restart = playing && (loop || backend->looping);
    if (!playing) releaseVoice();
    backend->sequence.push_back(buffer->data);
    if (backend->sequence.size()==1) bufferIndex=index;
    if (restart) { releaseVoice(); play(); }
    else if (backend->voice) {
        XAUDIO2_BUFFER submitted = {};
        submitted.AudioBytes=static_cast<UINT32>(buffer->data->samples.size());
        submitted.pAudioData=buffer->data->samples.data(); submitted.Flags=XAUDIO2_END_OF_STREAM;
        const HRESULT hr=backend->voice->SubmitSourceBuffer(&submitted);
        if (FAILED(hr)) error("Queue sound buffer",hr);
    }
}
void Source::refreshPlaying()
{
    if (!Manager::isRunning()) { releaseVoice(); return; }
    if (!backend->voice || !backend->started) { playing=false; return; }
    XAUDIO2_VOICE_STATE state = {};
    backend->voice->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    playing = state.BuffersQueued!=0;
    if (!playing) { backend->started=false; backend->looping=false; backend->exitPending=false; }
}
void Source::updateAll()
{
    refreshPlaying();
    if (!backend->voice || backend->sequence.empty() || !Manager::isRunning()) return;
    if (loop!=backend->looping || (loop && backend->exitPending)) {
        if (!loop) {
            const HRESULT hr=backend->voice->ExitLoop();
            if (FAILED(hr)) error("Exit sound loop",hr);
            // ExitLoop is a no-op before the audio thread enters the loop. Keep
            // requesting it on updates until GetState confirms completion.
            if (SUCCEEDED(hr)) backend->exitPending=true;
        } else {
            const bool resume=playing;
            releaseVoice();
            if (resume) { play(); return; }
        }
    }
    if (!backend->voice) return;
    const WaveData & wave = *backend->sequence[0];
    X3DAUDIO_LISTENER listener = spatialListener(*Manager::getListener(),relative);
    X3DAUDIO_EMITTER emitter = {};
    emitter.Position=vector(position); emitter.Velocity=vector(velocity);
    emitter.OrientFront={0,0,1}; emitter.OrientTop={0,1,0};
    emitter.ChannelCount=wave.channels; emitter.ChannelRadius=1;
    float azimuths[] = {X3DAUDIO_PI*1.5f,X3DAUDIO_PI*0.5f};
    emitter.pChannelAzimuths=wave.channels>1 ? azimuths : 0;
    X3DAUDIO_DISTANCE_CURVE_POINT points[] = {{0,1},{1,1}};
    X3DAUDIO_DISTANCE_CURVE constantVolume = {points,2};
    emitter.pVolumeCurve=&constantVolume; emitter.CurveDistanceScaler=4; emitter.DopplerScaler=1;
    X3DAUDIO_DSP_SETTINGS settings = {};
    settings.SrcChannelCount=wave.channels; settings.DstChannelCount=audio.channels;
    settings.pMatrixCoefficients=backend->matrix.data();
    X3DAudioCalculate(audio.spatial,&listener,&emitter,X3DAUDIO_CALCULATE_MATRIX|X3DAUDIO_CALCULATE_DOPPLER,&settings);
    const float dx=emitter.Position.x-listener.Position.x, dy=emitter.Position.y-listener.Position.y, dz=emitter.Position.z-listener.Position.z;
    const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    const float attenuation=4/std::max(4.0f,distance); // Preserve the old OpenAL reference distance/inverse law.
    HRESULT hr=backend->voice->SetVolume(volume(static_cast<float>(gain))*attenuation);
    if (SUCCEEDED(hr)) hr=backend->voice->SetOutputMatrix(audio.master,wave.channels,audio.channels,backend->matrix.data());
    const float ratio=std::max(XAUDIO2_MIN_FREQ_RATIO,std::min(maxPitch,finite(static_cast<float>(pitch)*settings.DopplerFactor,1)));
    if (SUCCEEDED(hr)) hr=backend->voice->SetFrequencyRatio(ratio);
    if (FAILED(hr)) error("Update sound source",hr);
}
void Source::play()
{
    refreshPlaying();
    if (playing || !Manager::isRunning()) return;
    releaseVoice();
    if (!prepareVoice()) return;
    updateAll();
    if (!backend->voice || !Manager::isRunning()) return;
    const HRESULT hr=backend->voice->Start();
    if (FAILED(hr)) { error("Start sound source",hr); releaseVoice(); return; }
    backend->started=true; playing=true;
}
void Source::replay() { releaseVoice(); play(); }
void Source::stop() { releaseVoice(); }
unsigned Source::queuedBuffers() const
{
    if (!backend->voice) return static_cast<unsigned>(backend->sequence.size());
    XAUDIO2_VOICE_STATE state = {};
    backend->voice->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued;
}
unsigned long long Source::samplesPlayed() const
{
    if (!backend->voice) return 0;
    XAUDIO2_VOICE_STATE state = {};
    backend->voice->GetState(&state);
    return state.SamplesPlayed;
}
}
#endif
