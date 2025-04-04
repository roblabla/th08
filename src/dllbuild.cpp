// DLLBUILD support
//
// The DLLBUILD is a special build of th08 meant to be injected into th08 1.00d,
// replacing the official functions with our own reimplementation. The primary
// purpose here is to validate that our reimplementations work even when they
// are not bit-accurate, but can also serve as a mechanism to enable modding
// before the reimplementation work is complete.
//
// The way it works is rather simple: We use the Detours library to hijack all
// the functions we have reimplemented from the original binary. Similarly, for
// the functions we want to call but have not yet reimplemented, we hijack the
// stub functions to forward them to the original binary.
//
// The stubs are generated automatically by a script in
// scripts/generate_stubs.py, based on the information found in
// config/stubbed.csv. This generates a stubbed.cpp file that will be
// automatically compiled and linked.
//
// To ensure the data is shared, we integrate the DIFFBUILD mechanism to store
// data at fixed locations in memory. A downside of this approach is that the
// static initialization functions of those globals will run from the main
// binary instead of our reimplementation. This is fine though, as those
// functions generally just call the constructor, which we detour.
//
// Touhou6 uses a static CRT, which means in a normal scenario, the main
// executable and DLLs don't share a heap, leading to problems if one side
// allocates a pointer, returns it to the other side which then attemps to free
// it. To avoid this issue, we additionally detour a handful of memory
// allocation functions from the CRT (malloc, calloc, realloc, free, msize,
// operator_new) to ensure only a single implementation is used at all times.
//
// Meanwhile, the list of functions to detour is auto-generated by another
// script, scripts/generate_detours.py, based on the information found in
// config/implemented.csv.

#include <d3d8.h>
#include <d3dx8.h>

#include "detours.h"
#include <fstream>
#include <string.h>

struct Detouring
{
    size_t addressInOriginalBinary;
    char *nameInDllReplacement;
    BOOL stub;

    void *addrToReplace;
    void *replaceWith;
};

#include "detouring.cpp"

// For now, always use D3D_WRAPPER. In the future, we may want to swap it out
// with another "host" dll.
#define D3D_WRAPPER 1
#ifdef D3D_WRAPPER
typedef IDirect3D8 *(WINAPI *Direct3DCreate8Proc)(UINT);
extern "C" IDirect3D8 *__stdcall Direct3DCreate8(UINT sdk_version)
{
    char path[MAX_PATH + 1];
    GetSystemDirectoryA(path, MAX_PATH);
    strncat(path, "\\d3d8.dll", MAX_PATH - strlen(path));
    HMODULE d3d8dll = LoadLibraryA(path);
    if (d3d8dll == NULL)
    {
        return NULL;
    }
    Direct3DCreate8Proc realproc = (Direct3DCreate8Proc)GetProcAddress(d3d8dll, "Direct3DCreate8");
    if (!realproc)
    {
        return NULL;
    }

    return realproc(sdk_version);
}
#endif

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DetourRestoreAfterWith();

        if (DetourTransactionBegin() != 0)
        {
            // TODO: Show an error.
            return TRUE;
        }
        if (DetourUpdateThread(GetCurrentThread()) != 0)
        {
            // TODO: Show an error
            DetourTransactionAbort();
            return TRUE;
        }
        for (size_t i = 0; i < sizeof(detours) / sizeof(detours[0]); i++)
        {
            if (!detours[i].stub)
            {
                detours[i].addrToReplace = (void *)detours[i].addressInOriginalBinary;
                detours[i].replaceWith = (void *)GetProcAddress(hinst, detours[i].nameInDllReplacement);
            }
            else
            {
                detours[i].addrToReplace = (void *)GetProcAddress(hinst, detours[i].nameInDllReplacement);
                detours[i].replaceWith = (void *)detours[i].addressInOriginalBinary;
            }
            if (DetourAttach(&detours[i].addrToReplace, detours[i].replaceWith) != 0)
            {
                // TODO: Show an error
                DetourTransactionAbort();
                return TRUE;
            }
        }
        if (DetourTransactionCommit() != 0)
        {
            // TODO: Show an error
            return TRUE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        for (size_t i = 0; i < sizeof(detours) / sizeof(detours[0]); i++)
        {
            DetourDetach(&detours[i].addrToReplace, detours[i].replaceWith);
        }
        DetourTransactionCommit();
    }
    return TRUE;
}
