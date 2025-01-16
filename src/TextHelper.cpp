#include "TextHelper.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

namespace th08
{
DIFFABLE_STATIC_ARRAY_ASSIGN(FormatInfo, 7, g_FormatInfoArray) = {
    {D3DFMT_X8R8G8B8, 32, 0x00000000, 0x00FF0000, 0x0000FF00, 0x000000FF},
    {D3DFMT_A8R8G8B8, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF},
    {D3DFMT_X1R5G5B5, 16, 0x00000000, 0x00007C00, 0x000003E0, 0x0000001F},
    {D3DFMT_R5G6B5, 16, 0x00000000, 0x0000F800, 0x000007E0, 0x0000001F},
    {D3DFMT_A1R5G5B5, 16, 0x0000F000, 0x00007C00, 0x000003E0, 0x0000001F},
    {D3DFMT_A4R4G4B4, 16, 0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F},
    {(D3DFORMAT)-1, 0, 0, 0, 0, 0},
};

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

struct THBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[17];
};

#pragma function(memset)
#pragma var_order(imageWidthInBytes, deviceContext, originalBitmapObj, padding, bitmapInfo, formatInfo, bitmapObj,     \
                  bitmapData)
bool TextHelper::TryAllocateBuffer(i32 width, i32 height, D3DFORMAT format)
{
    HGDIOBJ originalBitmapObj;
    u8 *bitmapData;
    HBITMAP bitmapObj;
    FormatInfo *formatInfo;
    THBITMAPINFO bitmapInfo;
    u32 padding;
    HDC deviceContext;
    i32 imageWidthInBytes;

    ReleaseBuffer();
    memset(&bitmapInfo, 0, sizeof(THBITMAPINFO));
    formatInfo = GetFormatInfo(format);
    if (formatInfo == NULL)
    {
        return false;
    }
    imageWidthInBytes = ((((width * formatInfo->bitCount) / 8) + 3) / 4) * 4;
    bitmapInfo.bmiHeader.biSize = sizeof(THBITMAPINFO);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -(height + 1);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = formatInfo->bitCount;
    bitmapInfo.bmiHeader.biSizeImage = height * imageWidthInBytes;
    if (format != D3DFMT_X1R5G5B5 && format != D3DFMT_X8R8G8B8)
    {
        bitmapInfo.bmiHeader.biCompression = 3;
        ((u32 *)bitmapInfo.bmiColors)[0] = formatInfo->redMask;
        ((u32 *)bitmapInfo.bmiColors)[1] = formatInfo->greenMask;
        ((u32 *)bitmapInfo.bmiColors)[2] = formatInfo->blueMask;
        ((u32 *)bitmapInfo.bmiColors)[3] = formatInfo->alphaMask;
    }
    bitmapObj = CreateDIBSection(NULL, (BITMAPINFO *)&bitmapInfo, 0, (void **)&bitmapData, NULL, 0);
    if (bitmapObj == NULL)
    {
        return false;
    }
    memset(bitmapData, 0, bitmapInfo.bmiHeader.biSizeImage);
    deviceContext = CreateCompatibleDC(NULL);
    originalBitmapObj = SelectObject(deviceContext, bitmapObj);
    m_hdc = deviceContext;
    m_gdiObj2 = bitmapObj;
    m_buffer = bitmapData;
    m_imageSizeInBytes = bitmapInfo.bmiHeader.biSizeImage;
    m_gdiObj = originalBitmapObj;
    m_width = width;
    m_height = height;
    m_format = format;
    m_imageWidthInBytes = imageWidthInBytes;
    return true;
}

FormatInfo *TextHelper::GetFormatInfo(D3DFORMAT format)
{
    i32 local_8;

    for (local_8 = 0; g_FormatInfoArray[local_8].format != -1 && g_FormatInfoArray[local_8].format != format; local_8++)
    {
    }
    if (format == -1)
    {
        return NULL;
    }
    return &g_FormatInfoArray[local_8];
}

struct A1R5G5B5
{
    u16 blue : 5;
    u16 green : 5;
    u16 red : 5;
    u16 alpha : 1;
};

#pragma var_order(bufferRegion, idx, doubleArea, tempColor, bufferCursor, bufferStart)
bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight, BOOL param_5)
{
    i32 doubleArea;
    u8 *bufferRegion;
    i32 idx;
    A1R5G5B5 *bufferCursor;

    i32 tempColor;

    doubleArea = spriteWidth * fontHeight * 2;
    bufferRegion = &GetBuffer()[y * spriteWidth * 2];
    switch (m_format)
    {
    case D3DFMT_A8R8G8B8:
        for (idx = 3; idx < doubleArea; idx += 4)
        {
            bufferRegion[idx] = bufferRegion[idx] ^ 0xff;
        }
        break;
    case D3DFMT_A1R5G5B5:
        for (bufferCursor = (A1R5G5B5 *)bufferRegion, idx = 0; idx < doubleArea; idx += 2, bufferCursor += 1)
        {
            bufferCursor->alpha ^= 1;
            if (bufferCursor->alpha)
            {
                if (!param_5)
                {
                    if (bufferCursor->red >= bufferCursor->blue)
                    {
                        tempColor = bufferCursor->red - bufferCursor->red * idx * 2 / doubleArea / 3;
                        bufferCursor->red = tempColor >= 0x20 ? 0x1f : tempColor;

                        tempColor = bufferCursor->green - bufferCursor->green * idx * 2 / doubleArea / 3;
                        bufferCursor->green = tempColor >= 0x20 ? 0x1f : tempColor;
                    }
                    else
                    {
                        tempColor = bufferCursor->blue - bufferCursor->blue * idx / doubleArea / 2;
                        bufferCursor->blue = tempColor >= 0x20 ? 0x1f : tempColor;

                        tempColor = bufferCursor->green - bufferCursor->green * idx / doubleArea / 2;
                        bufferCursor->green = tempColor >= 0x20 ? 0x1f : tempColor;
                    }
                }
                else
                {
                    if (bufferCursor->red >= bufferCursor->blue)
                    {
                        tempColor = bufferCursor->red - bufferCursor->red * idx / doubleArea / 4;
                        bufferCursor->red = tempColor >= 0x20 ? 0x1f : tempColor;

                        tempColor = bufferCursor->green - bufferCursor->green * idx / doubleArea / 4;
                        bufferCursor->green = tempColor >= 0x20 ? 0x1f : tempColor;
                    }
                    else
                    {
                        tempColor = bufferCursor->blue - bufferCursor->blue * idx / doubleArea / 4;
                        bufferCursor->blue = tempColor >= 0x20 ? 0x1f : tempColor;

                        tempColor = bufferCursor->green - bufferCursor->green * idx / doubleArea / 4;
                        bufferCursor->green = tempColor >= 0x20 ? 0x1f : tempColor;
                    }
                }
            }
            else
            {
                bufferCursor->red = 0;
                bufferCursor->green = 0;
                bufferCursor->blue = 0;
            }
        }
        break;
    case D3DFMT_A4R4G4B4:
        for (idx = 1; idx < doubleArea; idx = idx + 2)
        {
            bufferRegion[idx] = bufferRegion[idx] ^ 0xf0;
        }
        break;
    default:
        return false;
    }
    return true;
}

u8 *TextHelper::GetBuffer()
{
    return m_buffer;
}

u32 TextHelper::GetImageWidthInBytes()
{
    return m_imageWidthInBytes;
}

i32 TextHelper::GetHeight()
{
    return m_height;
}
#pragma optimize("", on)
}; // namespace th08
