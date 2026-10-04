#include "Sound.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <fstream>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static void u16(std::vector<unsigned char> & bytes, unsigned value)
{ bytes.push_back(static_cast<unsigned char>(value)); bytes.push_back(static_cast<unsigned char>(value >> 8)); }
static void u32(std::vector<unsigned char> & bytes, unsigned value)
{ u16(bytes, value); u16(bytes, value >> 16); }
static void text(std::vector<unsigned char> & bytes, const char * value)
{ bytes.insert(bytes.end(), value, value + 4); }
static void chunk(std::vector<unsigned char> & bytes, const char * id, const std::vector<unsigned char> & data)
{ text(bytes, id); u32(bytes, static_cast<unsigned>(data.size())); bytes.insert(bytes.end(), data.begin(), data.end()); if (data.size() & 1) bytes.push_back(0); }
static std::vector<unsigned char> wav(unsigned tag, unsigned channels, unsigned bits, const std::vector<unsigned char> & samples, bool extensible = false)
{
    std::vector<unsigned char> bytes, format;
    text(bytes, "RIFF"); u32(bytes, 0); text(bytes, "WAVE");
    chunk(bytes, "JUNK", std::vector<unsigned char>(3, 99)); // Odd chunks require padding.
    u16(format, extensible ? 0xfffe : tag); u16(format, channels); u32(format, 8000);
    const unsigned align = channels * bits / 8;
    u32(format, 8000 * align); u16(format, align); u16(format, bits);
    if (extensible) {
        u16(format, 22); u16(format, bits); u32(format, channels == 1 ? 4 : 3);
        u32(format, tag); u16(format, 0); u16(format, 16);
        const unsigned char guid[] = {128,0,0,170,0,56,155,113};
        format.insert(format.end(), guid, guid + sizeof(guid));
    }
    chunk(bytes, "fmt ", format); chunk(bytes, "data", samples);
    const unsigned size = static_cast<unsigned>(bytes.size() - 8);
    for (unsigned i = 0; i < 4; ++i) bytes[4+i] = static_cast<unsigned char>(size >> (8*i));
    return bytes;
}
static bool write(const char * path, const std::vector<unsigned char> & bytes)
{
    FILE * file = std::fopen(path, "wb");
    if (!file) return false;
    const bool okay = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file); return okay;
}
static int decoding(const char * path)
{
    const unsigned char data[] = {0,128,255,64};
    const std::vector<unsigned char> samples(data, data + sizeof(data));
    CHECK(write(path, wav(1,1,8,samples)));
    Sound::Buffer buffer;
    CHECK(buffer.loadWAV(path));
    CHECK(buffer.loaded);
    const Sound::WaveData * decoded = buffer.waveData();
    CHECK(decoded && decoded->channels == 1 && decoded->bitsPerSample == 8 && decoded->sampleRate == 8000);
    CHECK(decoded && decoded->samples == samples);
    CHECK(!buffer.loadWAV(path)); // A second load must not overwrite an owned clip.
    Sound::Buffer copy = buffer;
    buffer.free();
    CHECK(!buffer.loaded && !buffer.waveData());
    CHECK(copy.loaded && copy.waveData() && copy.waveData()->samples == samples);
    copy.free(); copy.free();
    CHECK(!copy.loadWAV(0));
    CHECK(!copy.loadWAV("missing-sound-fixture.wav"));
    std::string tooLong(300, 'x');
    CHECK(!copy.loadWAV(tooLong.c_str()));
    for (unsigned bits = 8; bits <= 32; bits += 8) {
        CHECK(write(path, wav(1,2,bits,std::vector<unsigned char>(2 * bits / 8 * 4, 0), bits == 24)));
        CHECK(copy.loadWAV(path));
        CHECK(copy.waveData() && copy.waveData()->channels == 2 && copy.waveData()->bitsPerSample == bits);
        copy.free();
    }
    const unsigned char floats[] = {0,0,0,0, 0,0,128,63, 0,0,128,191}; // 0, 1, -1.
    CHECK(write(path, wav(3,1,32,std::vector<unsigned char>(floats,floats+sizeof(floats)), true)));
    CHECK(copy.loadWAV(path));
    CHECK(copy.waveData() && copy.waveData()->formatTag == 3);
    copy.free();
    std::vector<std::vector<unsigned char> > malformed;
    malformed.push_back(std::vector<unsigned char>(11,0));
    std::vector<unsigned char> bad = wav(1,1,8,samples); bad[0]='X'; malformed.push_back(bad);
    bad = wav(1,1,8,samples); bad[4]=255; bad[5]=255; bad[6]=255; bad[7]=255; malformed.push_back(bad);
    bad = wav(1,1,8,samples); bad[28]=255; malformed.push_back(bad); // fmt size exceeds remaining bytes.
    malformed.push_back(wav(2,1,8,samples)); // Unsupported compressed format.
    malformed.push_back(wav(1,0,8,samples));
    malformed.push_back(wav(1,3,8,samples));
    malformed.push_back(wav(1,1,12,samples));
    malformed.push_back(wav(1,2,16,std::vector<unsigned char>(3,0))); // Partial frame.
    malformed.push_back(wav(1,1,8,std::vector<unsigned char>()));
    bad = wav(1,1,8,samples); bad[44]=2; malformed.push_back(bad); // Inconsistent block alignment.
    bad = wav(1,1,8,samples); bad[40]=0; malformed.push_back(bad); // Inconsistent average byte rate.
    bad = wav(1,1,8,samples); bad.pop_back(); malformed.push_back(bad);
    const unsigned char nan[] = {0,0,192,127};
    malformed.push_back(wav(3,1,32,std::vector<unsigned char>(nan,nan+4)));
    for (size_t i = 0; i < malformed.size(); ++i) {
        CHECK(write(path, malformed[i]));
        CHECK(!copy.loadWAV(path));
        CHECK(!copy.loaded && !copy.waveData());
    }
    std::remove(path);
    std::printf("ViewerSoundTests decode: %d failures\n", failures);
    return failures ? 1 : 0;
}

static int backend(const char * path)
{
    Sound::Manager::close(); Sound::Manager::close();
    Sound::Source source; // Must survive creation before the engine and close/reopen.
    source.play(); CHECK(!source.playing);
    CHECK(Sound::Manager::addBuffer(path) == -1);
    if (!Sound::Manager::open()) {
        CHECK(!Sound::Manager::isRunning());
        CHECK(Sound::Manager::lastError()[0] != 0);
        source.replay(); source.stop(); source.updateAll();
        CHECK(!source.playing);
        std::printf("SKIP backend playback: %s\n", Sound::Manager::lastError());
        return failures ? 1 : 77;
    }
    CHECK(Sound::Manager::open()); // Idempotent startup must preserve active voices.
    CHECK(write(path, wav(1,1,16,std::vector<unsigned char>(8000,0)))); // 500ms silence.
    int index = Sound::Manager::addBuffer(path);
    CHECK(index >= 0);
    CHECK(Sound::Manager::addBuffer(path) == index);
    CHECK(Sound::Manager::getBuffer(~0u) == 0);
    source.bufferIndex = static_cast<unsigned>(index);
    source.loop = true;
    source.relative = true;
    source.gain = 0.25f;
    source.pitch = 1.25f;
    source.updateBuffer(); source.updateAll(); source.play();
    CHECK(source.playing);
    Sleep(80);
    const unsigned long long first = source.samplesPlayed();
    CHECK(first > 0);
    source.play(); Sleep(40);
    CHECK(source.samplesPlayed() > first); // play must not restart an already playing clip.
    source.replay(); CHECK(source.playing);
    source.stop(); CHECK(!source.playing);
    source.play(); CHECK(source.playing);
    Sound::Listener * listener = Sound::Manager::getListener();
    listener->position.set(1,2,3); listener->velocity.set(0,0,1); listener->gain = 0;
    listener->update();
    source.relative = false; source.position.set(-4,0,0); source.velocity.set(0,0,-1); source.updateAll();
    source.gain = std::numeric_limits<float>::quiet_NaN();
    source.pitch = -5; source.position.x = std::numeric_limits<float>::infinity(); source.updateAll();
    source.gain = 1; source.pitch = 1; source.position.x = 0;
    source.loop = false; source.updateAll(); // May run before the first audio processing quantum.
    const ULONGLONG deadline = GetTickCount64() + 3000;
    while (source.playing && GetTickCount64() < deadline) { Sleep(20); source.updateAll(); }
    if (source.playing) std::fprintf(stderr,"Loop exit: queued=%u, samples=%llu, error=%s\n",source.queuedBuffers(),source.samplesPlayed(),Sound::Manager::lastError());
    CHECK(!source.playing); // A completed one-shot must become playable again.
    source.play(); CHECK(source.playing); source.stop();
    source.queueBuffer(static_cast<unsigned>(index));
    CHECK(source.queuedBuffers() == 2);
    source.replay(); CHECK(source.playing);
    Sound::Manager::getBuffer(static_cast<unsigned>(index))->free(); // A submitted clip retains its PCM.
    Sleep(50); source.updateAll(); CHECK(source.playing);
    Sound::Manager::close();
    CHECK(!source.playing && !Sound::Manager::isRunning());
    CHECK(Sound::Manager::getBuffer(0) == 0);
    source.replay(); CHECK(!source.playing);
    CHECK(Sound::Manager::open());
    index = Sound::Manager::addBuffer(path); CHECK(index >= 0);
    source.bufferIndex = static_cast<unsigned>(index); source.updateBuffer(); source.play(); CHECK(source.playing);
    const unsigned tags[] = {1,1,1,1,3};
    const unsigned widths[] = {8,16,24,32,32};
    for (unsigned i = 0; i < sizeof(widths)/sizeof(*widths); ++i) {
        source.stop();
        Sound::Buffer * old = Sound::Manager::getBuffer(static_cast<unsigned>(index));
        if (old) old->free();
        const unsigned channels = i & 1 ? 2 : 1;
        const std::vector<unsigned char> frames(channels * widths[i] / 8 * 4000, 0);
        CHECK(write(path,wav(tags[i],channels,widths[i],frames,i==2 || i==4)));
        index = Sound::Manager::addBuffer(path); CHECK(index >= 0);
        source.bufferIndex=static_cast<unsigned>(index); source.loop=true; source.relative=true;
        source.updateBuffer(); source.play();
        if (!source.playing) std::fprintf(stderr,"Playback format %u-bit tag %u: %s\n",widths[i],tags[i],Sound::Manager::lastError());
        CHECK(source.playing);
        Sleep(40);
        CHECK(source.samplesPlayed()>0);
    }
    Sound::Manager::close(); source.stop();
    std::remove(path);
    std::printf("ViewerSoundTests backend: %d failures\n", failures);
    return failures ? 1 : 0;
}

int main(int argc, char ** argv)
{
    if (argc == 3 && std::strcmp(argv[1],"--scene") == 0) {
        const std::string dir=argv[2];
        CHECK(write((dir+"/silence.wav").c_str(),wav(1,1,16,std::vector<unsigned char>(8000,0))));
        std::ofstream pds((dir+"/demo.pds.ods").c_str());
        pds << "PDS { soundSupport {} Acts {\"sound.psc.ods\";} Actors { Box { Collision { Box { dimensions {1;1;1;} } } Sound { engine { wave {\"silence.wav\";} gainDefault {0;} } } } } }";
        std::ofstream psc((dir+"/sound.psc.ods").c_str());
        psc << "Scene { PhysStart {1;} PhysDuration {100;} gravity {0;-9.81;0;} Bodies { Box { name {SoundBox;} position {0;2;0;} mass {1;} } } }";
        CHECK(pds.good() && psc.good());
        return failures?1:0;
    }
    char temp[MAX_PATH] = {}, path[MAX_PATH] = {};
    if (!GetTempPathA(MAX_PATH,temp) || !GetTempFileNameA(temp,"nxs",0,path)) return 2;
    const int result = argc > 1 && std::strcmp(argv[1],"--backend") == 0 ? backend(path) : decoding(path);
    std::remove(path);
    return result;
}
