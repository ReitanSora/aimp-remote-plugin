#include "pch.h"
#include "Plugin.h"

/**
 * @brief Convert a wide-character (UTF-16) string to a UTF-8 encoded std::string.
 *
 * This utility wraps the Windows API {@code WideCharToMultiByte} to convert
 * a NUL-terminated wide string (WCHAR const*) into a UTF-8 std::string.
 *
 * Notes:
 * - If {@code wstr} is null or points to an empty string, an empty string is
 *   returned.
 * - The returned std::string does NOT include a trailing NUL character.
 * - The function is safe for use on the AIMP main thread and worker threads,
 *   but callers must still respect COM thread-affinity rules for any COM
 *   objects they use alongside this function.
 *
 * @param wstr Pointer to a NUL-terminated wide-character string (UTF-16).
 * @return std::string UTF-8 encoded string (empty on null/empty input).
 */
std::string MyPlugin::WideToUTF8(const WCHAR* wstr)
{
    if (!wstr || wstr[0] == L'\0')
        return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
    if (!strTo.empty() && strTo.back() == '\0')
        strTo.pop_back();
    return strTo;
}

/**
 * @brief Convert a UTF-8 encoded std::string to a wide-character (UTF-16) std::wstring.
 *
 * This utility wraps the Windows API `MultiByteToWideChar` to convert a UTF-8 byte
 * sequence held in a `std::string` into a `std::wstring`.
 *
 * Behavior and notes:
 * - If `str` is empty, an empty `std::wstring` is returned.
 * - The conversion uses `str.size()` (not a NUL-terminated c-string). This means
 *   embedded NUL bytes in `str` are preserved up to `str.size()`.
 * - The returned `std::wstring` represents the converted characters and does not
 *   include an extra terminating NUL in its logical length (though `c_str()` will
 *   provide a terminating NUL as usual).
 * - Uses `CP_UTF8` as the source code page.
 * - The function itself is thread-safe (no global state). Callers must still
 *   follow COM apartment/threading rules when using COM objects elsewhere.
 *
 * @param str UTF-8 encoded input string
 * @return std::wstring UTF-16 wide string converted from the input (empty on empty input)
 */
std::wstring MyPlugin::Utf8ToWide(const std::string& str)
{
    if (str.empty())
        return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

/**
 * @brief Retrieve a textual property from an IAIMPFileInfo as a UTF-8 std::string.
 *
 * This helper queries the provided `IAIMPFileInfo` for a property identified by
 * `propID` and requests the value as an `IAIMPString` COM object. If the call
 * succeeds the returned `IAIMPString` is converted to UTF-8 using `WideToUTF8`
 * and the COM object is released. If the property cannot be retrieved or the
 * resulting string is empty, the provided `defaultValue` is returned.
 *
 * @param info Pointer to an `IAIMPFileInfo` instance to query. The caller is
 *             responsible for providing a valid pointer (no null check is
 *             performed inside).
 * @param propID Integer identifier of the property to request.
 * @param defaultValue Fallback UTF-8 string returned when the property is
 *                     missing, empty, or retrieval fails.
 * @return std::string The property value converted to UTF-8, or `defaultValue`
 *                    when not available.
 *
 * @remarks
 * - The function relies on the COM-style `GetValueAsObject` method and checks
 *   success with `SUCCEEDED(...)`.
 * - The acquired `IAIMPString*` is released via `Release()` to avoid leaks.
 * - `WideToUTF8` is expected to accept the wide string returned by
 *   `IAIMPString::GetData()` and produce a UTF-8 `std::string`.
 * - This function does not perform thread-synchronization; ensure callers do
 *   not use the same `IAIMPFileInfo` concurrently in unsafe ways.
 */
std::string MyPlugin::GetPropertyText(IAIMPPropertyList* info, int propID, const std::string& defaultValue)
{
    IAIMPString* aString = nullptr;
    std::string result = "";
    if (SUCCEEDED(info->GetValueAsObject(propID, IID_IAIMPString, (void**)&aString)))
    {
        result = WideToUTF8(aString->GetData());
        aString->Release();
    }
    return result.empty() ? defaultValue : result;
}

/**
 * @brief Create an `IAIMPString` containing the UTF-8 text provided.
 *
 * This helper creates an `IAIMPString` COM object via the plugin core and
 * initializes its content using the UTF-8 input `utf8Text`.
 *
 * Behavior:
 * - Calls `_core->CreateObject(IID_IAIMPString, (void**)ppAIMPString)` to
 *   instantiate an `IAIMPString`. If this call fails the function returns
 *   `E_FAIL` and no object is returned.
 * - Converts the UTF-8 `std::string` to a `std::wstring` using
 *   `Utf8ToWide()` and calls `IAIMPString::SetData` to set the wide text and
 *   length (in characters).
 *
 * Threading & COM:
 * - The function uses the `_core` COM interface. Callers must respect COM
 *   apartment/threading rules required by AIMP core services (typically the
 *   AIMP main thread or other threads allowed by AIMP).
 *
 * Ownership / Lifetime:
 * - On success (CreateObject succeeded), `*ppAIMPString` receives a reference
 *   to a newly created `IAIMPString`. The caller is responsible for calling
 *   `Release()` on the returned `IAIMPString*` when no longer needed.
 * - If `CreateObject` fails, no object is returned and nothing needs to be
 *   released.
 * - Note: this function returns the HRESULT from `SetData`. If `CreateObject`
 *   succeeds but `SetData` fails, the created `IAIMPString` is still returned
 *   to the caller and must be released by the caller to avoid a leak.
 *
 * Parameters:
 * - `utf8Text` : UTF-8 encoded text to store in the `IAIMPString`.
 * - `ppAIMPString` : Out parameter that will receive the `IAIMPString*`.
 *
 * Return:
 * - `S_OK` if the string object was created and `SetData` succeeded.
 * - `E_FAIL` if `_core->CreateObject` failed.
 * - Other HRESULT error codes propagated from `SetData` when creation
 *   succeeds but initialization fails.
 *
 * Example:
 * IAIMPString *str = nullptr;
 * if (SUCCEEDED(CreateAIMPString("Hello", &str))) {
 *     // use str ...
 *     str->Release();
 * }
 */
HRESULT MyPlugin::CreateAIMPString(const std::string& utf8Text, IAIMPString** ppAIMPString)
{
    if (FAILED(_core->CreateObject(IID_IAIMPString, (void**)ppAIMPString)))
        return E_FAIL;
    std::wstring wstr = Utf8ToWide(utf8Text);
    return (*ppAIMPString)->SetData((WCHAR*)wstr.c_str(), static_cast<int>(wstr.length()));
}