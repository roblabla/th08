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
#pragma optimize("", on)
}; // namespace th08
