#ifndef NX_FOUNDATION_BREAK_H
#define NX_FOUNDATION_BREAK_H

#include <cstdlib>
#if defined(_WIN32) && defined(_MSC_VER)
#include <intrin.h>
#endif

// Assertion breaks retain Windows' breakpoint exception. Other hosts terminate
// explicitly until their platform debugger contract is implemented.
inline void nxFoundationBreak()
{
#if defined(_WIN32) && defined(_MSC_VER)
    __debugbreak();
#else
    std::abort();
#endif
}

// Resuming a missing-singleton breakpoint must not produce a null reference.
inline void nxFoundationMissingInstance()
{
    nxFoundationBreak();
    std::abort();
}

#endif
