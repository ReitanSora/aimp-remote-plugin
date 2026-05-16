#pragma once

#include "../../sdk/aimp/5.40/apiObjects.h"
#include "../../sdk/aimp/5.40/apiCore.h"
#include "../third_party/qrcodegen/qrcodegen.hpp"

using qrcodegen::QrCode;

IAIMPImage2* GenerateQRCodeImage(const std::wstring& ip, IAIMPCore* core);