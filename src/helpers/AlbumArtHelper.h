#include "../core/Plugin.h"

void __stdcall OnAlbumArtReceive(IAIMPImage* Image, IAIMPImageContainer* Container, void* UserData);

std::string get_mime_type(const std::vector<unsigned char>& data);