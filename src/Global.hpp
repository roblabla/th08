#pragma once

#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "pbg/PbgArchive.hpp"
#include <windows.h>

namespace th08
{
namespace FileSystem
{
LPBYTE Decrypt(LPBYTE inData, i32 size, u8 xorValue, u8 xorValueInc, i32 chunkSize, i32 maxBytes);
LPBYTE TryDecryptFromTable(LPBYTE inData, LPINT unused, i32 size);
LPBYTE Encrypt(LPBYTE inData, i32 size, u8 xorValue, u8 xorValueInc, i32 chunkSize, i32 maxBytes);
LPBYTE OpenFile(LPSTR path, i32 *fileSize, BOOL isExternalResource);
BOOL CheckIfFileAlreadyExists(LPCSTR path);
int WriteDataToFile(LPCSTR path, LPVOID data, size_t size);
}; // namespace FileSystem

class GameErrorContext
{
  public:
    GameErrorContext();
    ~GameErrorContext();

    void ResetContext()
    {
        m_BufferEnd = m_Buffer;
        m_BufferEnd[0] = '\0';
    }

    void Flush()
    {
        if (m_BufferEnd != m_Buffer)
        {
            Log("---------------------------------------------------------- \n");

            if (m_ShowMessageBox)
            {
                MessageBoxA(NULL, m_Buffer, "", MB_ICONSTOP);
            }

            FileSystem::WriteDataToFile("./log.txt", m_Buffer, strlen(m_Buffer));
        }
    }

    const char *Log(const char *fmt, ...);
    const char *Fatal(const char *fmt, ...);

  private:
    char m_Buffer[0x2000];
    char *m_BufferEnd;
    i8 m_ShowMessageBox;
};

class Rng
{
  public:
    u16 GetRandomU16();
    u32 GetRandomU32();
    f32 GetRandomF32();
    f32 GetRandomF32Signed();

    void ResetGenerationCount()
    {
        m_GenerationCount = 0;
    }

    void SetSeed(u16 newSeed)
    {
        m_Seed = newSeed;
    }

    u16 GetSeed()
    {
        return m_Seed;
    }

    u16 GetRandomU16InRange(u16 range)
    {
        return range != 0 ? GetRandomU16() % range : 0;
    }

    u32 GetRandomU32InRange(u32 range)
    {
        return range != 0 ? GetRandomU32() % range : 0;
    }

    f32 GetRandomF32InRange(f32 range)
    {
        return GetRandomF32() * range;
    }

    f32 GetRandomF32SignedInRange(f32 range)
    {
        return GetRandomF32Signed() * range;
    }

  private:
    u16 m_Seed, m_SeedBackup;
    u32 m_GenerationCount;
};

class ZunMemory
{
  public:
    ZunMemory();
    ~ZunMemory();

    void *Alloc(size_t size, const char *debugText)
    {
        return malloc(size);
    }

    void Free(void *ptr)
    {
        free(ptr);
    }

  private:
    LPVOID m_Unk0[0x1000];
    BOOL m_Unk4000;
};

DIFFABLE_EXTERN(Rng, g_Rng);
DIFFABLE_EXTERN(GameErrorContext, g_GameErrorContext);
DIFFABLE_EXTERN(PbgArchive, g_PbgArchive);
DIFFABLE_EXTERN(ZunMemory, g_ZunMemory);
}; // namespace th08
