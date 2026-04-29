#define CURL_STATICLIB
#include "json.hpp"
#include "auth.h"
#include "curl/curl.h"
#include <wininet.h>  // Para pegar o IP (WinINet API)
#include <Windows.h>
#include <iostream>
#include <string>
#include <thread>  // Adicionando para o uso de threads
#include <sddl.h>

#pragma comment(lib, "wininet.lib")
#pragma comment (lib, "source/auth/curl/libcurl_a.lib")
#pragma comment(lib, "advapi32.lib")

using json = nlohmann::json;
using namespace std;

// URL da API
const std::string Auth::apiUrl = "https://api.zeroauth.cc";

// Função para obter o IP público
std::string GetIPInfo() {
    std::string ip = "Unknown";
    HINTERNET hInternet = InternetOpenW(L"GetIP", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hConnect = InternetOpenUrlA(hInternet, "http://ip-api.com/json/", NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect) {
            char buffer[4096];
            DWORD bytesRead = 0;
            std::string fullResponse;
            do {
                if (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead)) {
                    buffer[bytesRead] = '\0';
                    fullResponse += std::string(buffer);
                }
            } while (bytesRead > 0);
            if (!fullResponse.empty()) {
                ip = fullResponse;
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    return ip;
}

// Função para obter HWID usando API nativa do Windows (SID do usuário)
static std::string get_hwid() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return "none";
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    
    std::string sidString = "none";
    if (dwSize > 0) {
        PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
        if (pTokenUser) {
            if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
                LPSTR pStringSid = NULL;
                if (ConvertSidToStringSidA(pTokenUser->User.Sid, &pStringSid)) {
                    sidString = pStringSid;
                    LocalFree(pStringSid);
                }
            }
            free(pTokenUser);
        }
    }

    CloseHandle(hToken);
    return sidString;
}

// Função para exibir MessageBox com o título e mensagem fornecida
void ShowMessageBoxError(const std::string& title, const std::string& message) {
    int len = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    wchar_t* wide_message = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide_message, len);

    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
    wchar_t* wide_title = new wchar_t[titleLen];
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wide_title, titleLen);

    MessageBoxW(NULL, wide_message, wide_title, MB_OK | MB_ICONERROR);
    delete[] wide_message;
    delete[] wide_title;
}

void ShowMessageBox(const std::string& title, const std::string& message) {
    int len = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    wchar_t* wide_message = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide_message, len);

    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
    wchar_t* wide_title = new wchar_t[titleLen];
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wide_title, titleLen);

    MessageBoxW(NULL, wide_message, wide_title, MB_OK | MB_ICONINFORMATION);
    delete[] wide_message;
    delete[] wide_title;
}

// Função de callback para cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Função de erro do sistema
void error(const std::string& message) {
    // Escapar a mensagem para uso no CMD (substituir aspas e outros caracteres problemáticos)
    std::string escapedMessage = message;

    // Substituir aspas duplas por simples para evitar problemas no comando
    size_t pos = 0;
    while ((pos = escapedMessage.find('"', pos)) != std::string::npos) {
        escapedMessage.replace(pos, 1, "'");
        pos += 1;
    }

    // Substituir quebras de linha por espaços
    pos = 0;
    while ((pos = escapedMessage.find('\n', pos)) != std::string::npos) {
        escapedMessage.replace(pos, 1, " ");
        pos += 1;
    }

    while ((pos = escapedMessage.find('\r', pos)) != std::string::npos) {
        escapedMessage.replace(pos, 1, " ");
        pos += 1;
    }

    // Abrir janela de comando com o erro (igual ao C#)
    // Usar chcp 65001 para configurar UTF-8 no CMD antes de exibir a mensagem
    std::string cmd = "cmd.exe /c start cmd /C \"chcp 65001 >nul && color b && title Error && echo " + escapedMessage + " && timeout /t 5\"";
    system(cmd.c_str());

    ExitProcess(0);
}

// Função para escapar strings JSON
std::string Auth::EscapeJsonString(const std::string& value) {
    if (value.empty())
        return "";

    std::string escaped;
    for (char c : value) {
        switch (c) {
        case '\\': escaped += "\\\\"; break;
        case '\"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        default: escaped += c; break;
        }
    }
    return escaped;
}

// Função para parse de mensagens de erro
std::string Auth::ParseErrorMessage(const std::string& responseBody, const std::string& defaultMessage) {
    try {
        json jsonDoc = json::parse(responseBody);
        if (jsonDoc.contains("message") && jsonDoc["message"].is_string()) {
            return jsonDoc["message"].get<std::string>();
        }
    }
    catch (...) {
        if (responseBody.length() > 200) {
            return responseBody.substr(0, 200) + "...";
        }
        return responseBody;
    }
    return defaultMessage;
}

// Função para realizar requisição GET
bool Auth::SendGetRequest(const std::string& url, std::string& response, long& statusCode) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return res == CURLE_OK;
}

// Função para realizar requisição HTTP POST com cURL
bool Auth::SendRequest(const std::string& url, const std::string& jsonData, std::string& response, long& statusCode) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return res == CURLE_OK;
}

// Função estática para enviar log para API
void Auth::SendLogToAPI(const std::string& usernameOrKey, const std::string& hwid, const std::string& ipInfo,
    bool isKeyLogin, const std::string& appid, const std::string& appDatabase) {
    try {
        char computerUsername[256];
        size_t len = 0;
        getenv_s(&len, computerUsername, sizeof(computerUsername), "USERNAME");
        std::string compUsername = (len > 0) ? std::string(computerUsername) : "";

        std::string escapedUsernameOrKey = EscapeJsonString(usernameOrKey);
        std::string escapedHwid = EscapeJsonString(hwid);
        std::string escapedIpInfo = EscapeJsonString(ipInfo);
        std::string escapedCompUsername = EscapeJsonString(compUsername);
        std::string escapedAppid = EscapeJsonString(appid);
        std::string escapedAppDatabase = EscapeJsonString(appDatabase);

        std::string jsonPayload = "{\"usernameOrKey\":\"" + escapedUsernameOrKey +
            "\", \"hwid\":\"" + escapedHwid +
            "\", \"ipInfo\":\"" + escapedIpInfo +
            "\", \"isKeyLogin\":" + (isKeyLogin ? "true" : "false") +
            ", \"computerUsername\":\"" + escapedCompUsername +
            "\", \"appid\":\"" + escapedAppid +
            "\", \"appDatabase\":\"" + escapedAppDatabase + "\"}";

        std::string response;
        long statusCode = 0;
        bool success = SendRequest(apiUrl + "/api/auth-api/log-login", jsonPayload, response, statusCode);

        if (!success || statusCode < 200 || statusCode >= 300) {
            std::string errorMsg = "Erro ao enviar log para API. Status: " + std::to_string(statusCode);
            if (!response.empty()) {
                errorMsg += ", Resposta: " + response;
            }
            // Não chamar error() aqui para não encerrar o programa durante o log
        }
    }
    catch (const std::exception& ex) {
        // Silenciar erros de log para não interromper o fluxo principal
    }
    catch (...) {
        // Silenciar erros de log
    }
}

// Função estática para obter HWID
std::string Auth::GetHWID() {
    return get_hwid();
}

// Função estática para obter IP Info (usando a função global)
std::string Auth::GetIPInfo() {
    return ::GetIPInfo();
}

// Construtor da classe Auth
Auth::Auth(const std::string& Application, const std::string& OwnerID)
    : application(Application), ownerID(OwnerID), isInitialized(false) {
}

// Método para verificar inicialização
void Auth::EnsureInitialized() {
    if (!isInitialized) {
        error("ZeroAuth não foi inicializado. Chame o método Init() primeiro antes de usar qualquer método da classe.");
    }
}

// Método Init()
bool Auth::Init() {
    try {
        // Verificar health
        std::string healthUrl = apiUrl + "/api/auth-api/health?appDatabase=";
        // Escapar ownerID para URL
        CURL* curl = curl_easy_init();
        if (curl) {
            char* escaped = curl_easy_escape(curl, ownerID.c_str(), 0);
            healthUrl += escaped;
            curl_free(escaped);
            curl_easy_cleanup(curl);
        }

        std::string healthResponse;
        long statusCode = 0;
        bool healthSuccess = SendGetRequest(healthUrl, healthResponse, statusCode);

        // Se não conseguiu conectar, a API está offline
        if (!healthSuccess) {
            error("ZeroAuth offline. Não foi possível conectar à API. Verifique sua conexão com a internet.");
            return false;
        }

        // Se conseguiu conectar mas retornou erro, a API está online mas com problema
        if (statusCode < 200 || statusCode >= 300) {
            // 400 = Bad Request (appDatabase não fornecido ou inválido)
            if (statusCode == 400) {
                std::string errorMsg = "Erro: appDatabase inválido ou não fornecido. Status: " + std::to_string(statusCode);
                if (!healthResponse.empty()) {
                    errorMsg += ". " + ParseErrorMessage(healthResponse, "");
                }
                error(errorMsg);
                return false;
            }
            // 404 = Not Found (endpoint não existe, mas API está online)
            else if (statusCode == 404) {
                std::cout << "Aviso: Endpoint de health não encontrado (404), mas API está online. Continuando..." << std::endl;
            }
            // 500 = Erro interno do servidor
            else if (statusCode == 500) {
                std::string errorMsg = "Erro interno no servidor ZeroAuth. Status: " + std::to_string(statusCode);
                if (!healthResponse.empty()) {
                    errorMsg += ". " + ParseErrorMessage(healthResponse, "");
                }
                // Não bloqueia, continua para verificar AppID
                std::cout << "Aviso: " << errorMsg << ". Continuando..." << std::endl;
            }
            // Outros erros HTTP
            else {
                std::string errorMsg = "Erro ao verificar saúde da API. Status: " + std::to_string(statusCode);
                if (!healthResponse.empty()) {
                    errorMsg += ". Resposta: " + ParseErrorMessage(healthResponse, "");
                }
                error(errorMsg);
                return false;
            }
        }

        // Verificar status do AppID
        std::string escapedApp = EscapeJsonString(application);
        std::string escapedOwner = EscapeJsonString(ownerID);
        std::string checkData = "{\"appid\":\"" + escapedApp + "\", \"appDatabase\":\"" + escapedOwner + "\"}";

        std::string statusResponse;
        statusCode = 0;
        bool statusSuccess = SendRequest(apiUrl + "/api/auth-api/check-app-status", checkData, statusResponse, statusCode);

        if (statusSuccess && statusCode >= 200 && statusCode < 300) {
            if (statusResponse.find("AppID está ativo.") != std::string::npos) {
                std::cout << "AppID está ativo." << std::endl;
                isInitialized = true;
                return true;
            }
        }

        std::string errorMessage = "Erro ao verificar status do AppID.";
        if (!statusResponse.empty()) {
            errorMessage = ParseErrorMessage(statusResponse, errorMessage);
        }
        error(errorMessage);
        return false;
    }
    catch (const std::exception& ex) {
        error("Erro de conexão ao inicializar ZeroAuth: " + std::string(ex.what()) + ". Verifique sua conexão com a internet.");
        return false;
    }
    catch (...) {
        error("Timeout ao inicializar ZeroAuth. A requisição demorou muito para responder.");
        return false;
    }
}

// Método GetExpiration()
std::string Auth::GetExpiration(const std::string& keyOrUsername, const std::string& format, bool isKey) {
    EnsureInitialized();

    try {
        std::string escapedKeyOrUsername = EscapeJsonString(keyOrUsername);
        std::string escapedFormat = EscapeJsonString(format);
        std::string escapedApp = EscapeJsonString(application);
        std::string escapedOwner = EscapeJsonString(ownerID);

        std::string fieldName = isKey ? "key" : "username";
        std::string postData = "{\"" + fieldName + "\":\"" + escapedKeyOrUsername +
            "\", \"appid\":\"" + escapedApp +
            "\", \"appDatabase\":\"" + escapedOwner +
            "\", \"format\":\"" + escapedFormat + "\"}";

        std::string response;
        long statusCode = 0;
        bool success = SendRequest(apiUrl + "/api/auth-api/get-expiration", postData, response, statusCode);

        if (success && statusCode >= 200 && statusCode < 300) {
            try {
                json jsonDoc = json::parse(response);
                if (jsonDoc.contains("message") && jsonDoc["message"].is_string()) {
                    return jsonDoc["message"].get<std::string>();
                }
            }
            catch (...) {
                return "Erro: Resposta JSON inválida da API.";
            }
            return "Erro: Resposta inválida da API.";
        }
        else {
            return ParseErrorMessage(response, "Erro ao obter a expiração.");
        }
    }
    catch (const std::exception& ex) {
        return "Erro de conexão: " + std::string(ex.what()) + ". Verifique sua conexão com a internet.";
    }
    catch (...) {
        return "Erro: Timeout ao obter expiração. A requisição demorou muito para responder.";
    }
}

// Método CheckApiAvailability()
bool Auth::CheckApiAvailability() {
    try {
        std::string healthUrl = apiUrl + "/api/auth-api/health?appDatabase=";
        CURL* curl = curl_easy_init();
        if (curl) {
            char* escaped = curl_easy_escape(curl, ownerID.c_str(), 0);
            healthUrl += escaped;
            curl_free(escaped);
            curl_easy_cleanup(curl);
        }

        std::string response;
        long statusCode = 0;
        bool success = SendGetRequest(healthUrl, response, statusCode);
        return success && statusCode >= 200 && statusCode < 300;
    }
    catch (...) {
        return false;
    }
}

// Método LoginWithKey()
AuthResult Auth::LoginWithKey(const std::string& key) {
    EnsureInitialized();

    // Validar se a key está vazia
    if (key.empty() || key.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira uma key válida para fazer login." };
    }

    if (!CheckApiAvailability()) {
        error("Erro: ZeroAuth não está disponível no momento. Verifique sua conexão.");
        return AuthResult{ 503, "Erro: API não disponível." };
    }

    try {
        std::string hwid = GetHWID();
        std::string escapedKey = EscapeJsonString(key);
        std::string escapedHwid = EscapeJsonString(hwid);
        std::string escapedApp = EscapeJsonString(application);
        std::string escapedOwner = EscapeJsonString(ownerID);

        std::string postData = "{\"key\":\"" + escapedKey +
            "\", \"hwid\":\"" + escapedHwid +
            "\", \"appid\":\"" + escapedApp +
            "\", \"appDatabase\":\"" + escapedOwner + "\"}";

        std::string response;
        long statusCode = 0;
        bool success = SendRequest(apiUrl + "/api/auth-api/login", postData, response, statusCode);

        if (success && statusCode >= 200 && statusCode < 300) {
            std::string ip = GetIPInfo();
            std::thread logThread(SendLogToAPI, key, hwid, ip, true, application, ownerID);
            logThread.detach();
            return AuthResult{ (int)statusCode, "Login com a key bem-sucedido!" };
        }
        else {
            return AuthResult{ (int)statusCode, ParseErrorMessage(response, "Erro desconhecido ao fazer login com a key.") };
        }
    }
    catch (const std::exception& ex) {
        return AuthResult{ 0, "Erro de conexão: " + std::string(ex.what()) + ". Verifique sua conexão com a internet." };
    }
    catch (...) {
        return AuthResult{ 0, "Erro: Timeout ao fazer login. A requisição demorou muito para responder." };
    }
}

// Método LoginWithUser()
AuthResult Auth::LoginWithUser(const std::string& username, const std::string& password) {
    EnsureInitialized();

    // Validar se username ou password estão vazios
    if (username.empty() || username.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira um nome de usuário válido para fazer login." };
    }
    if (password.empty() || password.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira uma senha válida para fazer login." };
    }

    if (!CheckApiAvailability()) {
        error("Erro: ZeroAuth não está disponível no momento. Verifique sua conexão.");
        return AuthResult{ 503, "Erro: API não disponível." };
    }

    try {
        std::string hwid = GetHWID();
        std::string escapedUsername = EscapeJsonString(username);
        std::string escapedPassword = EscapeJsonString(password);
        std::string escapedHwid = EscapeJsonString(hwid);
        std::string escapedApp = EscapeJsonString(application);
        std::string escapedOwner = EscapeJsonString(ownerID);

        std::string postData = "{\"username\":\"" + escapedUsername +
            "\", \"password\":\"" + escapedPassword +
            "\", \"hwid\":\"" + escapedHwid +
            "\", \"appid\":\"" + escapedApp +
            "\", \"appDatabase\":\"" + escapedOwner + "\"}";

        std::string response;
        long statusCode = 0;
        bool success = SendRequest(apiUrl + "/api/auth-api/user-login", postData, response, statusCode);

        if (success && statusCode >= 200 && statusCode < 300) {
            std::string ip = GetIPInfo();
            std::thread logThread(SendLogToAPI, username, hwid, ip, false, application, ownerID);
            logThread.detach();
            return AuthResult{ (int)statusCode, "Login com usuário bem-sucedido!" };
        }
        else {
            return AuthResult{ (int)statusCode, ParseErrorMessage(response, "Erro desconhecido ao fazer login com usuário.") };
        }
    }
    catch (const std::exception& ex) {
        return AuthResult{ 0, "Erro de conexão: " + std::string(ex.what()) + ". Verifique sua conexão com a internet." };
    }
    catch (...) {
        return AuthResult{ 0, "Erro: Timeout ao fazer login. A requisição demorou muito para responder." };
    }
}

// Método RegisterUserWithKey()
AuthResult Auth::RegisterUserWithKey(const std::string& username, const std::string& password, const std::string& key) {
    EnsureInitialized();

    // Validar se os campos estão vazios
    if (username.empty() || username.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira um nome de usuário válido para registrar." };
    }
    if (password.empty() || password.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira uma senha válida para registrar." };
    }
    if (key.empty() || key.find_first_not_of(" \t\n\r") == std::string::npos) {
        return AuthResult{ 400, "Por favor, insira uma key válida para registrar." };
    }

    if (!CheckApiAvailability()) {
        error("Erro: ZeroAuth não está disponível no momento. Verifique sua conexão.");
        return AuthResult{ 503, "Erro: API não disponível." };
    }

    try {
        std::string hwid = GetHWID();
        std::string escapedUsername = EscapeJsonString(username);
        std::string escapedPassword = EscapeJsonString(password);
        std::string escapedKey = EscapeJsonString(key);
        std::string escapedHwid = EscapeJsonString(hwid);
        std::string escapedApp = EscapeJsonString(application);
        std::string escapedOwner = EscapeJsonString(ownerID);

        std::string postData = "{\"username\":\"" + escapedUsername +
            "\", \"password\":\"" + escapedPassword +
            "\", \"key\":\"" + escapedKey +
            "\", \"hwid\":\"" + escapedHwid +
            "\", \"appid\":\"" + escapedApp +
            "\", \"appDatabase\":\"" + escapedOwner + "\"}";

        std::string response;
        long statusCode = 0;
        bool success = SendRequest(apiUrl + "/api/auth-api/register", postData, response, statusCode);

        if (success && statusCode >= 200 && statusCode < 300) {
            return AuthResult{ (int)statusCode, "Usuário registrado com sucesso!" };
        }
        else {
            return AuthResult{ (int)statusCode, ParseErrorMessage(response, "Erro desconhecido ao registrar usuário.") };
        }
    }
    catch (const std::exception& ex) {
        return AuthResult{ 0, "Erro de conexão: " + std::string(ex.what()) + ". Verifique sua conexão com a internet." };
    }
    catch (...) {
        return AuthResult{ 0, "Erro: Timeout ao registrar usuário. A requisição demorou muito para responder." };
    }
}

// ===== FUNÇÕES ANTIGAS PARA COMPATIBILIDADE =====

void SendLogToAPI_Old(const std::string& usernameOrKey, const std::string& hwid, const std::string& ipInfo,
    bool isKeyLogin, const std::string& appid, const std::string& appDatabase) {
    CURL* curl;
    CURLcode res;
    std::string response;

    char computerUsername[256];
    size_t len = 0;
    getenv_s(&len, computerUsername, sizeof(computerUsername), "USERNAME");

    json logData = {
        {"usernameOrKey", usernameOrKey},
        {"hwid", hwid},
        {"ipInfo", ipInfo},
        {"isKeyLogin", isKeyLogin},
        {"computerUsername", std::string(computerUsername)},
        {"appid", appid},
        {"appDatabase", appDatabase}
    };

    std::string jsonPayload = logData.dump();

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.zeroauth.cc/api/auth-api/log-login");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Erro ao enviar log para API: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

bool SendRequest_Old(const std::string& url, const json& data, std::string& response, long& statusCode) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        std::string json_data = data.dump();

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return true;
}

bool LoginWithKey(const std::string& key, const std::string& appID, const std::string& appDataBase, std::string& response) {
    std::string hwid = get_hwid();
    std::string ipInfo = GetIPInfo();

    json loginData = {
        {"key", key},
        {"hwid", hwid},
        {"appid", appID},
        {"appDatabase", appDataBase}
    };

    long statusCode = 0;
    bool success = SendRequest_Old("https://api.zeroauth.cc/api/auth-api/login", loginData, response, statusCode);

    if (success && statusCode >= 200 && statusCode < 300) {
        std::thread logThread(SendLogToAPI_Old, key, hwid, ipInfo, true, appID, appDataBase);
        logThread.detach();
    }

    return success && statusCode >= 200 && statusCode < 300;
}

bool LoginWithUser(const std::string& username, const std::string& password, const std::string& appID, const std::string& appDataBase, std::string& response) {
    std::string hwid = get_hwid();
    std::string ipInfo = GetIPInfo();

    json userLoginData = {
        {"username", username},
        {"password", password},
        {"hwid", hwid},
        {"appid", appID},
        {"appDatabase", appDataBase}
    };

    long statusCode = 0;
    bool success = SendRequest_Old("https://api.zeroauth.cc/api/auth-api/user-login", userLoginData, response, statusCode);

    if (success && statusCode >= 200 && statusCode < 300) {
        std::thread logThread(SendLogToAPI_Old, username, hwid, ipInfo, false, appID, appDataBase);
        logThread.detach();
    }

    return success && statusCode >= 200 && statusCode < 300;
}

bool RegisterUser(const std::string& username, const std::string& password, const std::string& key, const std::string& appID, const std::string& appDataBase, std::string& response) {
    json registerData = {
        {"username", username},
        {"password", password},
        {"key", key},
        {"appid", appID},
        {"appDatabase", appDataBase}
    };
    long statusCode = 0;
    return SendRequest_Old("https://api.zeroauth.cc/api/auth-api/register", registerData, response, statusCode);
}

bool CheckAppStatus(const std::string& appID, const std::string& appDataBase, std::string& response) {
    json checkData = {
        {"appid", appID},
        {"appDatabase", appDataBase}
    };
    long statusCode = 0;
    return SendRequest_Old("https://api.zeroauth.cc/api/auth-api/check-app-status", checkData, response, statusCode);
}
