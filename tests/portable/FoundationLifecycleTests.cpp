#include "FoundationSDK.h"
#include "NxUserOutputStream.h"
#include "NxUserDebugRenderer.h"
#include "NxDebugRenderable.h"
#include "NxProfiler.h"
#include "NxMath.h"
#include "FixtureSupport.h"
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <csignal>
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

static unsigned failures;
static std::vector<unsigned> observations;
static_assert(sizeof(NxFoundation::FoundationSDK) == 56, "the genuine Win32 Foundation receiver");

static void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", message);
        ++failures;
    }
}

static void exact(unsigned value)
{
    observations.push_back(value);
}

static void bits(float value)
{
    unsigned word;
    std::memcpy(&word, &value, sizeof(word));
    exact(word);
}

class GuardAllocator : public NxUserAllocator
{
  public:
    std::map<void *, size_t> blocks;
    unsigned allocations = 0;
    unsigned releases = 0;

    void *mallocDEBUG(size_t size, const char *, int) override
    {
        return malloc(size);
    }
    void *malloc(size_t size) override
    {
        unsigned char *block = static_cast<unsigned char *>(std::malloc(size + 32));
        if (!block)
            std::abort();
        std::memset(block, 0x6a, 16);
        std::memset(block + 16, 0xcd, size);
        std::memset(block + 16 + size, 0x7b, 16);
        blocks[block + 16] = size;
        ++allocations;
        return block + 16;
    }
    void *realloc(void *pointer, size_t size) override
    {
        if (!pointer)
            return malloc(size);
        const auto found = blocks.find(pointer);
        check(found != blocks.end(), "realloc owns the actual block");
        if (found == blocks.end())
            std::abort();
        void *next = malloc(size);
        std::memcpy(next, pointer, size < found->second ? size : found->second);
        free(pointer);
        return next;
    }
    void canaries() const
    {
        for (const auto &block : blocks)
        {
            const unsigned char *bytes = static_cast<const unsigned char *>(block.first) - 16;
            for (unsigned i = 0; i < 16; ++i)
                check(bytes[i] == 0x6a && bytes[16 + block.second + i] == 0x7b, "actual allocator canaries");
        }
    }
    void free(void *pointer) override
    {
        if (!pointer)
            return;
        const auto found = blocks.find(pointer);
        check(found != blocks.end(), "free owns the actual block");
        if (found == blocks.end())
            std::abort();
        canaries();
        blocks.erase(found);
        ++releases;
        std::free(static_cast<unsigned char *>(pointer) - 16);
    }
};

class ErrorStream : public NxUserOutputStream
{
  public:
    NxAssertResponse response = NX_AR_CONTINUE;
    unsigned errors = 0, assertions = 0, prints = 0;
    NxErrorCode code = NXE_NO_ERROR;
    std::string message, file;
    int line = 0;
    void reportError(NxErrorCode value, const char *text, const char *path, int row) override
    {
        ++errors;
        code = value;
        message = text;
        file = path;
        line = row;
    }
    NxAssertResponse reportAssertViolation(const char *text, const char *path, int row) override
    {
        ++assertions;
        message = text;
        file = path;
        line = row;
        return response;
    }
    void print(const char *text) override
    {
        ++prints;
        message = text;
    }
};

class Observer : public NxFoundation::Observable
{
  public:
    unsigned calls = 0, eventId = 0;
    NxFoundation::Observable *source = nullptr;
    void event(NxU32 value, NxFoundation::Observable &sender) override
    {
        ++calls;
        eventId = value;
        source = &sender;
    }
};

class Renderer : public NxUserDebugRenderer
{
  public:
    mutable unsigned calls = 0;
    void renderData(const NxDebugRenderable &data) const override
    {
        ++calls;
        exact(data.getNbPoints());
        exact(data.getNbLines());
        exact(data.getNbTriangles());
    }
};

#ifdef _DEBUG
static void assertIgnoreSite()
{
    NX_ASSERT(false);
}
static void assertContinueSite()
{
    NX_ASSERT(false);
}

static void assertionMacroDomain()
{
    GuardAllocator allocator;
    ErrorStream stream;
    NxFoundationSDK *sdk = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &stream, &allocator);
    stream.response = NX_AR_IGNORE;
    assertIgnoreSite();
    assertIgnoreSite();
    check(stream.assertions == 1, "actual assertion macro persists ignore per call site");
    stream.response = NX_AR_CONTINUE;
    assertContinueSite();
    assertContinueSite();
    check(stream.assertions == 3, "actual assertion macro continues and reports again");
    check(stream.message == "false" && stream.line > 0,
          "actual assertion macro forwards expression and location");
    sdk->release();
    check(allocator.blocks.empty(), "debug assertion owner released");
    nxFoundationSDKAllocator = nullptr;
}
#endif

static void errorDomain(NxFoundationSDK &sdk, ErrorStream &stream)
{
    using NxFoundation::FoundationSDK;
    check(!FoundationSDK::error(NXE_INVALID_PARAMETER, "input.cpp", 47, nullptr, "value=%d name=%s", 17,
                                "mesh"),
          "error return remains false");
    check(stream.message == "value=17 name=mesh" && stream.file == "input.cpp" && stream.line == 47 &&
              stream.code == NXE_INVALID_PARAMETER,
          "formatted error payload carries every argument");
    exact(stream.errors);
    FoundationSDK::error(NXE_INVALID_OPERATION, "second.cpp", 48, nullptr, "%s", "second");
    exact(sdk.getFirstError());
    exact(sdk.getFirstError());
    exact(sdk.getLastError());
    exact(sdk.getLastError());
    check(!FoundationSDK::error(NXE_DB_PRINT, "print.cpp", 49, nullptr, "%s:%u", "cell", 3u),
          "print return remains false");
    check(stream.message == "cell:3" && stream.prints == 1, "print routes actual formatted payload");
    exact(sdk.getFirstError());
    exact(sdk.getLastError());
    for (NxAssertResponse response : {NX_AR_CONTINUE, NX_AR_IGNORE, NX_AR_BREAKPOINT})
    {
        bool ignore = false;
        stream.response = response;
        const bool shouldBreak =
            FoundationSDK::dbAssert(NXE_ASSERTION, "assert.cpp", 50, &ignore, "vertex count");
        check(shouldBreak == (response == NX_AR_BREAKPOINT), "assert response controls break");
        check(ignore == (response == NX_AR_IGNORE), "only ignore response sets flag");
        check(stream.message == "vertex count" && stream.file == "assert.cpp" && stream.line == 50,
              "assertion payload is passed directly");
        exact(shouldBreak);
        exact(ignore);
        exact(sdk.getFirstError());
        exact(sdk.getLastError());
    }
    stream.response = NX_AR_BREAKPOINT;
    bool ignore = false;
    exact(FoundationSDK::error(NXE_ASSERTION, "assert.cpp", 51, &ignore, "literal assertion"));
    exact(ignore);
    exact(sdk.getFirstError());
    exact(sdk.getLastError());
    sdk.setErrorStream(nullptr);
    exact(sdk.getErrorStream() == nullptr);
    FoundationSDK::error(NXE_OUT_OF_MEMORY, "memory.cpp", 52, nullptr, "allocation");
    exact(sdk.getFirstError());
    exact(sdk.getLastError());
    sdk.setErrorStream(&stream);
    // _vsnprintf's complete, terminated short-message domain ends at159.
    const std::string longestSupported(159, 'x');
    FoundationSDK::error(NXE_INTERNAL_ERROR, "short.cpp", 53, nullptr, "%s", longestSupported.c_str());
    check(stream.message == longestSupported, "159-character defined short formatting");
    exact(sdk.getFirstError());
    exact(sdk.getLastError());
    exact(stream.errors);
    exact(stream.assertions);
    exact(stream.prints);
}

static void debugOwnership(NxFoundationSDK &sdk)
{
    NxDebugRenderable *first = sdk.createDebugRenderable();
    NxDebugRenderable *second = sdk.createDebugRenderable();
    check(first && second && first != second, "genuine debug objects are independently owned");
    const NxVec3 a(0.1f, -2.0f, 3.25f), b(1.5f, 0.0f, -0.0f), c(-1.0f, 2.0f, 0.5f);
    first->addPoint(a, 0x12345678);
    first->addLine(a, b, 0x87654321);
    first->addTriangle(a, b, c, 0xaabbccdd);
    exact(first->getNbPoints());
    exact(first->getNbLines());
    exact(first->getNbTriangles());
    const NxDebugPoint &point = first->getPoints()[0];
    bits(point.p.x);
    bits(point.p.y);
    bits(point.p.z);
    exact(point.color);
    const NxDebugLine &line = first->getLines()[0];
    bits(line.p0.x);
    bits(line.p0.y);
    bits(line.p0.z);
    bits(line.p1.x);
    bits(line.p1.y);
    bits(line.p1.z);
    exact(line.color);
    const NxDebugTriangle &triangle = first->getTriangles()[0];
    bits(triangle.p0.x);
    bits(triangle.p0.y);
    bits(triangle.p0.z);
    bits(triangle.p1.x);
    bits(triangle.p1.y);
    bits(triangle.p1.z);
    bits(triangle.p2.x);
    bits(triangle.p2.y);
    bits(triangle.p2.z);
    exact(triangle.color);
    Renderer renderer;
    sdk.renderDebugData(renderer);
    exact(renderer.calls);
    first->clear();
    exact(first->getNbPoints());
    exact(first->getNbLines());
    exact(first->getNbTriangles());
    sdk.releaseDebugRenderable(first);
    exact(first == nullptr);
    sdk.releaseDebugRenderable(first);
    sdk.releaseDebugRenderable(second);
    exact(second == nullptr);
    // The original SDK destructor deletes the nonvirtual public debug base.
    // Retained debug objects are unsupported with this guarded allocator:
    // releaseDebugRenderable must destroy their concrete owner first.
    NxProfilingZone *zone = sdk.createProfilingZone("mesh lifecycle");
    check(zone != nullptr, "genuine profiling zone");
    check(std::strcmp(static_cast<NxProfiler::DefineZone *>(zone)->getName(), "mesh lifecycle") == 0,
          "profiling zone retains borrowed name");
    zone->enter();
    zone->leave();
    zone->release();
}

static void lifecycle()
{
    using NxFoundation::FoundationSDK;
    GuardAllocator allocator, alternate;
    ErrorStream firstStream, secondStream;
    exact(FoundationSDK::dbAssert(NXE_ASSERTION, "absent.cpp", 1, nullptr, "absent"));
    check(NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION ^ 0x100, &firstStream, &allocator) == nullptr,
          "version mismatch rejects before creation");
    exact(allocator.allocations);
    NxFoundationSDK *sdk = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &firstStream, &allocator);
    check(sdk != nullptr && &sdk->getAllocator() == &allocator, "actual custom allocator selection");
    check(allocator.allocations == 1 && allocator.blocks.size() == 1 &&
              allocator.blocks.begin()->second == 56,
          "actual SDK allocation size and count");
    check(NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION ^ 1, &secondStream, &alternate) == nullptr &&
              sdk->getErrorStream() == &firstStream && &sdk->getAllocator() == &allocator,
          "version mismatch leaves live singleton bindings untouched");
    exact(sdk->getErrorStream() == &firstStream);
    exact(&FoundationSDK::getInstance() == static_cast<FoundationSDK *>(sdk));
    exact(sdk->getFirstError());
    exact(sdk->getLastError());
    NxFoundationSDK *again = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &secondStream, &alternate);
    exact(again == sdk);
    exact(&sdk->getAllocator() == &allocator);
    exact(sdk->getErrorStream() == &secondStream);
    exact(alternate.allocations);
    errorDomain(*sdk, secondStream);
    debugOwnership(*sdk);
    {
        Observer observer;
        FoundationSDK &actual = static_cast<FoundationSDK &>(*sdk);
        actual.addObserver(observer);
        exact(actual.getNumObservers());
        actual.notifyObservers(37);
        exact(observer.calls);
        exact(observer.eventId);
        exact(observer.source == &actual);
        sdk->release();
        exact(&FoundationSDK::getInstance() == &actual);
        again = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &firstStream, &alternate);
        exact(again == sdk);
        actual.removeObserver(observer);
        exact(actual.getNumObservers());
        exact(&FoundationSDK::getInstance() == &actual);
        actual.addObserver(observer);
        sdk->release();
        // Last observer removal invokes real FoundationSDK::event and deletes it.
        actual.removeObserver(observer);
    }
    allocator.canaries();
    check(allocator.blocks.empty(), "SDK, observer arrays and debug payloads released");
    check(nxFoundationSDKAllocator == &allocator, "released SDK retains original allocator binding");
    exact(allocator.allocations == allocator.releases);
    exact(alternate.blocks.empty());
    exact(FoundationSDK::dbAssert(NXE_ASSERTION, "absent.cpp", 2, nullptr, "absent"));
    sdk = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, nullptr, nullptr);
    exact(&sdk->getAllocator() == &FoundationSDK::defaultSDKAllocator);
    exact(sdk->getErrorStream() == nullptr);
    sdk->release();
    check(nxFoundationSDKAllocator == &FoundationSDK::defaultSDKAllocator,
          "default allocator binding also remains after release");
    nxFoundationSDKAllocator = nullptr;
    // Public numerical headers are exercised without the old parser seam.
    bits(NxMath::sqrt(4.0f));
    exact(NxMath::sqrt(9.0) == 3.0);
    float cosine = 7, sine = 8;
    NxSinCos(cosine, sine, 0.0f);
    bits(cosine);
    bits(sine);
}

static LONG CALLBACK trapHandler(EXCEPTION_POINTERS *exception)
{
    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
        ExitProcess(73);
    return EXCEPTION_CONTINUE_SEARCH;
}

#if !NX_PHYSICS_USE_X87
static LONG CALLBACK resumeTrap(EXCEPTION_POINTERS *exception)
{
    if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;
    // The Windows x86 context points at the intrinsic's one-byte INT3. A
    // debugger-style continuation must advance it rather than retrap forever.
    const DWORD address = reinterpret_cast<DWORD>(exception->ExceptionRecord->ExceptionAddress);
    if (*reinterpret_cast<const unsigned char *>(address) != 0xcc)
        ExitProcess(75);
    if (exception->ContextRecord->Eip == address)
        ++exception->ContextRecord->Eip;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void abortHandler(int)
{
    ExitProcess(74);
}
#endif

static void childOutcome(const char *executable, const char *argument, DWORD expected)
{
    std::string command = std::string("\"") + executable + "\" " + argument;
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    check(CreateProcessA(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                         &startup, &process) != FALSE,
          "start actual missing-instance child");
    if (!process.hProcess)
        return;
    const DWORD waited = WaitForSingleObject(process.hProcess, 10000);
    check(waited == WAIT_OBJECT_0, "missing-instance trap terminates");
    if (waited != WAIT_OBJECT_0)
        TerminateProcess(process.hProcess, 76);
    DWORD exitCode = 0;
    check(GetExitCodeProcess(process.hProcess, &exitCode) != FALSE && exitCode == expected,
          "real child failure outcome");
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

static void put(FILE *file, unsigned value)
{
    const unsigned char bytes[] = {static_cast<unsigned char>(value), static_cast<unsigned char>(value >> 8),
                                   static_cast<unsigned char>(value >> 16),
                                   static_cast<unsigned char>(value >> 24)};
    std::fwrite(bytes, 1, 4, file);
}

static unsigned get(const unsigned char *bytes)
{
    return unsigned(bytes[0]) | (unsigned(bytes[1]) << 8) | (unsigned(bytes[2]) << 16) |
           (unsigned(bytes[3]) << 24);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (std::strcmp(argv[1], "--missing-instance") == 0)
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        AddVectoredExceptionHandler(1, trapHandler);
        NxFoundation::FoundationSDK::getInstance();
        return 4;
    }
#ifdef _DEBUG
    if (std::strcmp(argv[1], "--assert-break") == 0)
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        AddVectoredExceptionHandler(1, trapHandler);
        GuardAllocator allocator;
        ErrorStream stream;
        stream.response = NX_AR_BREAKPOINT;
        NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &stream, &allocator);
        NX_ASSERT(false);
        return 4;
    }
#endif
#if !NX_PHYSICS_USE_X87
    if (std::strcmp(argv[1], "--resumed-missing-instance") == 0)
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        std::signal(SIGABRT, abortHandler);
        AddVectoredExceptionHandler(1, resumeTrap);
        NxFoundation::FoundationSDK::getInstance();
        return 4;
    }
#endif
#if NX_PHYSICS_USE_X87
    check((_controlfp(0, 0) & (_MCW_PC | _MCW_RC)) == (_PC_53 | _RC_NEAR),
          "reconstructed reference uses nearest53 control word");
#endif
    lifecycle();
#ifdef _DEBUG
    assertionMacroDomain();
    childOutcome(argv[0], "--assert-break", 73);
#endif
    childOutcome(argv[0], "--missing-instance", 73);
    exact(73);
#if !NX_PHYSICS_USE_X87
    childOutcome(argv[0], "--resumed-missing-instance", 74);
#endif
#if NX_PHYSICS_USE_X87
    FILE *file = std::fopen(argv[1], "wb");
    if (!file)
        return 5;
    std::fwrite("NXPF", 1, 4, file);
    put(file, 1);
    put(file, unsigned(observations.size()) * 20);
    put(file, 20);
    for (unsigned i = 0; i < observations.size(); ++i)
    {
        put(file, 0);
        put(file, 0);
        put(file, i);
        put(file, 0);
        put(file, observations[i]);
    }
    std::fclose(file);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    check(nxReadFixture(argv[1], bytes, error), error.c_str());
    check(bytes.size() == observations.size() * 20, "actual Foundation observation count");
    if (bytes.size() == observations.size() * 20)
        for (unsigned i = 0; i < observations.size(); ++i)
        {
            const unsigned char *row = &bytes[i * 20];
            check(get(row) == 0 && get(row + 4) == 0 && get(row + 8) == i && get(row + 12) == 0,
                  "Foundation fixture identity");
            check(get(row + 16) == observations[i], "exact Foundation state/ownership/payload word");
        }
#endif
    std::printf("foundation_lifecycle observations=%zu failures=%u\n", observations.size(), failures);
    return failures ? 1 : 0;
}
