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
#pragma optimize("", on)
}; // namespace th08
