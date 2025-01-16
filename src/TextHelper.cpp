#include "TextHelper.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

namespace th08
{
#pragma optimize("s", on)
TextHelper::TextHelper()
{
    m_format = (D3DFORMAT)-1;
    m_width = 0;
    m_height = 0;
    m_hdc = 0;
    m_gdiObj2 = 0;
    m_gdiObj = 0;
    m_buffer = NULL;
}

TextHelper::~TextHelper()
{
    ReleaseBuffer();
}

bool TextHelper::ReleaseBuffer()
{
    if (m_hdc)
    {
        SelectObject(m_hdc, m_gdiObj);
        DeleteDC(m_hdc);
        DeleteObject(m_gdiObj2);
        m_format = (D3DFORMAT)-1;
        m_width = 0;
        m_height = 0;
        m_hdc = 0;
        m_gdiObj2 = 0;
        m_gdiObj = 0;
        m_buffer = NULL;
        return true;
    }
    else
    {
        return false;
    }
}

bool TextHelper::AllocateBufferWithFallback(i32 width, i32 height, D3DFORMAT format)
{
    if (TryAllocateBuffer(width, height, format))
    {
        return true;
    }

    if (format == D3DFMT_A1R5G5B5 || format == D3DFMT_A4R4G4B4)
    {
        return TryAllocateBuffer(width, height, D3DFMT_A8R8G8B8);
    }
    if (format == D3DFMT_R5G6B5)
    {
        return TryAllocateBuffer(width, height, D3DFMT_X8R8G8B8);
    }
    return false;
}
#pragma optimize("", on)
}; // namespace th08
