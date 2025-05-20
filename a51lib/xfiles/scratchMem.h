#pragma once

#include <cassert>

#define ALIGN_16(n) ((((size_t)(n)) + 15) & (-16))
#define ALIGN_64(n) ((((size_t)(n)) + 63) & (-64))
#define SMEM_ALIGN ALIGN_16

void     smem_ChangeSize(int NBytes);
uint8_t* smem_BufferAlloc(int NBytes); // Can return nullptr
uint8_t* smem_StackAlloc(int NBytes);  // Can return nullptr
void     smem_StackPushMarker();
void     smem_StackPopToMarker();
int      smem_GetMaxUsed();
int      smem_GetBufferSize();
void     smem_SetThreadId(int ThreadId);

void smem_Init(int NBytes);
void smem_Kill();
void smem_Toggle();
void smem_ResetAfterException();

#define smem_BufferAlloc(S) smem_rfunc_BufferAlloc((S))
#define smem_StackAlloc(S) smem_rfunc_StackAlloc((S))
#define smem_StackPushMarker() smem_rfunc_StackPushMarker()
#define smem_StackPopToMarker() smem_rfunc_StackPopToMarker()
uint8_t* smem_rfunc_BufferAlloc(int NBytes);
uint8_t* smem_rfunc_StackAlloc(int NBytes);
void     smem_rfunc_StackPushMarker();
void     smem_rfunc_StackPopToMarker();

// These functions can be used to make sure you have a valid scratch allocation
int      smem_GetActiveID();
uint8_t* smem_GetActiveStartAddr();
uint8_t* smem_GetActiveEndAddr();
void     smem_Validate();

#define MAX_MARKERS 16

extern bool     g_SMemInitialized;
extern uint8_t* g_pSMemBufferTop;
extern uint8_t* g_pSMemStackTop;
extern uint8_t* g_pSMemMarker[MAX_MARKERS];
extern int      g_SMemNextMarker;
extern int      g_SMemThreadId;

inline uint8_t* smem_rfunc_BufferAlloc(int NBytes)
{
    //assert( (g_SMemThreadId == -1) || (g_SMemThreadId == x_GetThreadID()) );
    assert(g_SMemInitialized);
    assert(NBytes >= 0);

    // Allocate memory for the request.
    uint8_t* pAllocation = g_pSMemBufferTop;
    g_pSMemBufferTop += SMEM_ALIGN(NBytes);

    return (pAllocation);
}

//==============================================================================

inline uint8_t* smem_rfunc_StackAlloc(int NBytes)
{
    //assert( (g_SMemThreadId == -1) || (g_SMemThreadId == x_GetThreadID()) );
    assert(g_SMemInitialized);
    assert(NBytes >= 0);

    // Allocate memory for the request.
    g_pSMemStackTop -= SMEM_ALIGN(NBytes);
    uint8_t* pAllocation = g_pSMemStackTop;

    // Make sure we haven't exhausted this section.
    //
    // NOTE - If you got an assert failure here, then you have consumed all
    //        available scratch memory.  Increase the amount of memory dedicated
    //        to the scratch memory system.
    //
    if (g_pSMemBufferTop > g_pSMemStackTop) {
        //x_DebugMsg("smem_StackAlloc Failed!!!\n");
        assert(false);
    }

    // Return the allocated memory.
    return (pAllocation);
}

//==============================================================================

inline void smem_rfunc_StackPushMarker()
{
    //assert( (g_SMemThreadId == -1) || (g_SMemThreadId == x_GetThreadID()) );
    assert(g_SMemInitialized);
    assert(g_SMemNextMarker < MAX_MARKERS); // Too many markers!

    g_pSMemMarker[g_SMemNextMarker] = g_pSMemStackTop;
    g_SMemNextMarker++;
}

//==============================================================================

inline void smem_rfunc_StackPopToMarker()
{
    //assert( (g_SMemThreadId == -1) || (g_SMemThreadId == x_GetThreadID()) );
    assert(g_SMemInitialized);
    assert(g_SMemNextMarker > 0); // Too many pops!

    g_SMemNextMarker--;
    g_pSMemStackTop = g_pSMemMarker[g_SMemNextMarker];
}
