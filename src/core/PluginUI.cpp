#include "pch.h"

#include "Plugin.h"
#include "../helpers/QRHelper.h"
#include "../helpers/NetworkHelper.h"

HRESULT WINAPI MyPlugin::GetName(IAIMPString** S)
{
    if (FAILED(_core->CreateObject(IID_IAIMPString, (void**)S)))
        return E_UNEXPECTED;

    const wchar_t* name = L"Fluke: AIMP Remote";
    (*S)->SetData((PWCHAR)name, static_cast<int>(wcslen(name)));
    return S_OK;
}

HWND WINAPI MyPlugin::CreateFrame(HWND ParentWnd)
{
    if (!_core)
        return NULL;

    IAIMPServiceUI* uiService = nullptr;
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceUI, (void**)&uiService)))
        return NULL;

    IAIMPString* strName = nullptr;
    IAIMPString* strCaption = nullptr;
    _core->CreateObject(IID_IAIMPString, (void**)&strName);
    _core->CreateObject(IID_IAIMPString, (void**)&strCaption);

    strName->SetData((PWCHAR)L"RemoteForm", 10);
    if (FAILED(uiService->CreateForm(ParentWnd, AIMPUI_SERVICE_CREATEFORM_FLAGS_CHILD, strName, nullptr, &_uiForm)))
    {
        strName->Release();
        strCaption->Release();
        uiService->Release();
        return NULL;
    }

    TAIMPUIControlPlacement formPlacement{};
    formPlacement.Alignment = ualClient;
    formPlacement.Bounds.left = 0;
    formPlacement.Bounds.top = 0;
    formPlacement.Bounds.right = 0;
    formPlacement.Bounds.bottom = 0;

    _uiForm->SetPlacement(formPlacement);

    strCaption->SetData((PWCHAR)L"Fluke: AIMP Remote", 18);
    _uiForm->SetValueAsObject(3, strCaption);

    strName->SetData((PWCHAR)L"grpRemote", 9);
    IAIMPUIGroupBox* groupBox = nullptr;

    if (SUCCEEDED(uiService->CreateControl(_uiForm, _uiForm, strName, nullptr, IID_IAIMPUIGroupBox, (void**)&groupBox))) {

        TAIMPUIControlPlacement groupPlacement{};

        groupPlacement.Alignment = ualTop;
        groupBox->SetPlacement(groupPlacement);

        groupBox->SetValueAsInt32(AIMPUI_GROUPBOX_PROPID_AUTOSIZE, 1);

        strCaption->SetData((PWCHAR)L"Sync QR Code", 7);
        groupBox->SetValueAsObject(AIMPUI_GROUPBOX_PROPID_CAPTION, strCaption);

        strName->SetData((PWCHAR)L"lblDescription", 14);
        IAIMPUILabel* descLbl = nullptr;

        if (SUCCEEDED(uiService->CreateControl(_uiForm, groupBox, strName, nullptr, IID_IAIMPUILabel, (void**)&descLbl))) {

            TAIMPUIControlPlacement descPlacement{};
            descPlacement.Alignment = ualTop;
            descLbl->SetPlacement(descPlacement);

            const wchar_t* descText = L"Using the mobile app, scan the QR code to connect to Fluke: AIMP Remote. Both devices must be connected to the same Wi-Fi network.";

            strCaption->SetData((PWCHAR)descText, static_cast<int>(wcslen(descText)));
            descLbl->SetValueAsObject(AIMPUI_LABEL_PROPID_TEXT, strCaption);
            descLbl->SetValueAsInt32(AIMPUI_LABEL_PROPID_WORDWRAP, TRUE);
            descLbl->SetValueAsInt32(AIMPUI_LABEL_PROPID_AUTOSIZE, 1);
            descLbl->SetValueAsInt32(AIMPUI_LABEL_PROPID_TEXTALIGNVERT, AIMPUI_ALIGN_NEAR);
            descLbl->Release();
        }

        IAIMPUIPanel* spacer = nullptr;

        strName->SetData((PWCHAR)L"spacer", 6);

        if (SUCCEEDED(uiService->CreateControl(
            _uiForm,
            groupBox,
            strName,
            nullptr,
            IID_IAIMPUIPanel,
            (void**)&spacer)))
        {
            TAIMPUIControlPlacement spacerPlacement{};

            spacerPlacement.Alignment = ualTop;
            spacerPlacement.Bounds.bottom = 20;

            spacer->SetPlacement(spacerPlacement);
            spacer->SetValueAsInt32(
                AIMPUI_PANEL_PROPID_BORDERS,
                AIMPUI_FLAGS_BORDERS_NONE
            );
            spacer->Release();
        }

        strName->SetData((PWCHAR)L"imgQR", 5);
        IAIMPUIImage* qrImageControl = nullptr;
        if (SUCCEEDED(uiService->CreateControl(_uiForm, groupBox, strName, nullptr, IID_IAIMPUIImage, (void**)&qrImageControl)))
        {

            TAIMPUIControlPlacement placement{};
            placement.Alignment = ualTop;
            placement.Bounds.bottom = 150;

            qrImageControl->SetPlacement(placement);

            std::wstring currentIP = GetLocalIPW();
            std::string ipA;
            for (wchar_t c : currentIP)
                ipA += (char)c;
            std::string urlUtf8 = "fluke://preferences?ip=" + ipA;
            IAIMPImage2* qrImage = GenerateQRCodeImage(urlUtf8, _core);

            if (qrImage)
            {
                qrImageControl->SetValueAsObject(AIMPUI_IMAGE_PROPID_IMAGE, qrImage);
                qrImageControl->SetValueAsInt32(AIMPUI_IMAGE_PROPID_IMAGESTRETCHMODE, 2);
                qrImage->Release();
            }
            qrImageControl->Release();
        }

        groupBox->Release();
    }

    strName->SetData((PWCHAR)L"grpRemote", 9);
    IAIMPUIGroupBox* mobileAppGroupBox = nullptr;

    if (SUCCEEDED(uiService->CreateControl(_uiForm, _uiForm, strName, nullptr, IID_IAIMPUIGroupBox, (void**)&mobileAppGroupBox))) {

        TAIMPUIControlPlacement groupPlacement{};

        groupPlacement.Alignment = ualTop;
        groupPlacement.AlignmentMargins.top = 10;
        mobileAppGroupBox->SetPlacement(groupPlacement);

        mobileAppGroupBox->SetValueAsInt32(AIMPUI_GROUPBOX_PROPID_AUTOSIZE, 1);

        strCaption->SetData((PWCHAR)L"Mobile App", 10);
        mobileAppGroupBox->SetValueAsObject(AIMPUI_GROUPBOX_PROPID_CAPTION, strCaption);

        //////////
        strName->SetData((PWCHAR)L"lblDescription", 14);
        IAIMPUILabel* mobileAppDesc = nullptr;

        if (SUCCEEDED(uiService->CreateControl(_uiForm, mobileAppGroupBox, strName, nullptr, IID_IAIMPUILabel, (void**)&mobileAppDesc))) {

            TAIMPUIControlPlacement descPlacement{};
            descPlacement.Alignment = ualTop;
            mobileAppDesc->SetPlacement(descPlacement);

            const wchar_t* descText = L"To download the official mobile app, scan the QR code below, which will take you to the MEGA folder where it is hosted:";

            strCaption->SetData((PWCHAR)descText, static_cast<int>(wcslen(descText)));
            mobileAppDesc->SetValueAsObject(AIMPUI_LABEL_PROPID_TEXT, strCaption);
            mobileAppDesc->SetValueAsInt32(AIMPUI_LABEL_PROPID_WORDWRAP, TRUE);
            mobileAppDesc->SetValueAsInt32(AIMPUI_LABEL_PROPID_AUTOSIZE, 1);
            mobileAppDesc->SetValueAsInt32(AIMPUI_LABEL_PROPID_TEXTALIGNVERT, AIMPUI_ALIGN_NEAR);
            mobileAppDesc->Release();
        }

        IAIMPUIPanel* spacer2 = nullptr;

        strName->SetData((PWCHAR)L"spacer", 6);

        if (SUCCEEDED(uiService->CreateControl(
            _uiForm,
            mobileAppGroupBox,
            strName,
            nullptr,
            IID_IAIMPUIPanel,
            (void**)&spacer2)))
        {
            TAIMPUIControlPlacement spacerPlacement{};

            spacerPlacement.Alignment = ualTop;
            spacerPlacement.Bounds.bottom = 20;

            spacer2->SetPlacement(spacerPlacement);
            spacer2->SetValueAsInt32(
                AIMPUI_PANEL_PROPID_BORDERS,
                AIMPUI_FLAGS_BORDERS_NONE
            );
            spacer2->Release();
        }

        strName->SetData((PWCHAR)L"imgQR", 5);
        IAIMPUIImage* qrMobileApp = nullptr;
        if (SUCCEEDED(uiService->CreateControl(_uiForm, mobileAppGroupBox, strName, nullptr, IID_IAIMPUIImage, (void**)&qrMobileApp)))
        {

            TAIMPUIControlPlacement placement{};
            placement.Alignment = ualTop;
            placement.Bounds.bottom = 150;

            qrMobileApp->SetPlacement(placement);

            std::string urlUtf8 = "https://mega.nz/folder/oB4XWQxZ#kMF6POp4pGYJhxS9n1CDmw";
            IAIMPImage2* qrImage = GenerateQRCodeImage(urlUtf8, _core);

            if (qrImage)
            {
                qrMobileApp->SetValueAsObject(AIMPUI_IMAGE_PROPID_IMAGE, qrImage);
                qrMobileApp->SetValueAsInt32(AIMPUI_IMAGE_PROPID_IMAGESTRETCHMODE, 2);
                qrImage->Release();
            }
            qrMobileApp->Release();
        }

        mobileAppGroupBox->Release();
    }

    strName->Release();
    strCaption->Release();
    uiService->Release();

    return _uiForm->GetHandle();
}

void WINAPI MyPlugin::DestroyFrame()
{
    if (_ipLabel)
    {
        static_cast<IUnknown*>(_ipLabel)->Release();
        _ipLabel = nullptr;
    }
    if (_uiForm)
    {
        static_cast<IUnknown*>(_uiForm)->Release();
        _uiForm = nullptr;
    }
    if (_uiService != nullptr)
    {
        _uiService->Release();
        _uiService = nullptr;
    }
}

void WINAPI MyPlugin::Notification(int ID)
{
}
