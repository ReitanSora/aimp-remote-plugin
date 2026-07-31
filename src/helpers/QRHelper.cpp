#include "pch.h"

#include "QRHelper.h"

IAIMPImage2* GenerateQRCodeImage(const std::wstring& ip, IAIMPCore* core)
{
    std::string ipA;
    for (wchar_t c : ip)
        ipA += (char)c;
    std::string urlUtf8 = "fluke://preferences?ip=" + ipA;

    QrCode qr = QrCode::encodeText(urlUtf8.c_str(), QrCode::Ecc::MEDIUM);
    const int qrSize = qr.getSize();
    const int border = 1;
    const int scale = 12;
    const int imgSize = (qrSize + border * 2) * scale;

    HDC hdc = CreateCompatibleDC(NULL);
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = imgSize;
    bmi.bmiHeader.biHeight = -imgSize;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pixels, NULL, 0);
    HGDIOBJ oldBmp = SelectObject(hdc, hBmp);
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    RECT bg = { 0, 0, imgSize, imgSize };

    FillRect(hdc, &bg, whiteBrush);
    DeleteObject(whiteBrush);

    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));

    for (int y = 0; y < qrSize; y++)
    {
        for (int x = 0; x < qrSize; x++)
        {
            if (qr.getModule(x, y))
            {
                RECT r = { (x + border) * scale, (y + border) * scale, (x + border + 1) * scale, (y + border + 1) * scale };
                FillRect(hdc, &r, blackBrush);
            }
        }
    }

    DeleteObject(blackBrush);
    SelectObject(hdc, oldBmp);

    IAIMPImage2* aimpImg = nullptr;
    if (SUCCEEDED(core->CreateObject(IID_IAIMPImage2, (void**)&aimpImg)))
    {
        if (FAILED(aimpImg->LoadFromBitmap(hBmp)))
        {
            aimpImg->Release();
            aimpImg = nullptr;
        }
    }

    DeleteObject(hBmp);
    DeleteDC(hdc);

    return aimpImg;
}