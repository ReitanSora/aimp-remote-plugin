#include "pch.h"

#include "AlbumArtHelper.h"

void __stdcall OnAlbumArtReceive(IAIMPImage* Image, IAIMPImageContainer* Container, void* UserData)
{
    auto* imageBuffer = static_cast<std::vector<unsigned char> *>(UserData);
    if (Container)
    {
        DWORD size = Container->GetDataSize();
        byte* dataPtr = Container->GetData();
        if (dataPtr && size > 0)
        {
            imageBuffer->assign(dataPtr, dataPtr + size);
        }
    }
}

std::string get_mime_type(const std::vector<unsigned char>& data)
{
    if (data.size() < 4)
        return "image/jpeg";

    // JPEG: FF D8 FF
    if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
    {
        return "image/jpeg";
    }
    // PNG: 89 50 4E 47
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47)
    {
        return "image/png";
    }
    // GIF: 47 49 46 38
    if (data[0] == 0x47 && data[1] == 0x49 && data[2] == 0x46 && data[3] == 0x38)
    {
        return "image/gif";
    }
    // WebP: RIFF .... WEBP
    if (data.size() > 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
    {
        return "image/webp";
    }

    return "image/jpeg"; // Fallback
}