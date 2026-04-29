#include "discord_avatar.h"
#include <winhttp.h>
#include <vector>
#include <string>

#pragma comment(lib, "winhttp.lib")

ID3D11ShaderResourceView* LoadTextureFromMemory(ID3D11Device* device, const unsigned char* data, int size);

namespace DiscordAvatar
{
    static bool DownloadBytes(const std::string& url, std::vector<unsigned char>& out)
    {
        out.clear();

        URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
        wchar_t hostName[256] = { 0 };
        wchar_t urlPath[1024] = { 0 };

        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;

        int urlLen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), (int)url.length(), nullptr, 0);
        std::wstring wurl(urlLen + 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, url.c_str(), (int)url.length(), &wurl[0], urlLen);

        if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.length(), 0, &urlComp))
            return false;

        HINTERNET hSession = WinHttpOpen(L"SafetyLoader/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            L"GET",
            urlPath,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);

        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        WinHttpSetTimeouts(hRequest, 6000, 6000, 6000, 15000);

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX);

        if (statusCode < 200 || statusCode >= 300) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        DWORD bytesAvailable = 0;
        std::vector<char> buffer(4096);

        do {
            bytesAvailable = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
                break;

            if (bytesAvailable > 0) {
                if (buffer.size() < bytesAvailable)
                    buffer.resize(bytesAvailable);

                DWORD bytesRead = 0;
                if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                    out.insert(out.end(), buffer.begin(), buffer.begin() + bytesRead);
                }
            }
        } while (bytesAvailable > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return !out.empty();
    }

    ID3D11ShaderResourceView* LoadFromUrl(const char* url, ID3D11Device* device)
    {
        if (!url || !*url || !device) return nullptr;

        std::string urlStr = url;
        std::vector<unsigned char> imageData;

        if (!DownloadBytes(urlStr, imageData))
            return nullptr;

        return LoadTextureFromMemory(device, imageData.data(), (int)imageData.size());
    }

    void FreeTexture(ID3D11ShaderResourceView* texture)
    {
        if (texture) {
            texture->Release();
        }
    }
}