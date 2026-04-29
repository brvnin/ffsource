#ifndef AUTH_H
#define AUTH_H

#include <string>

// Struct para resultado de autenticação
struct AuthResult {
    int StatusCode;
    std::string Message;
    bool Success() const { return StatusCode >= 200 && StatusCode < 300; }
};

// Classe Auth (equivalente à ZeroAUTH)
class Auth {
private:
    std::string application;
    std::string ownerID;
    bool isInitialized;
    static const std::string apiUrl;

    void EnsureInitialized();
    static std::string GetHWID();
    static std::string GetIPInfo();
    static void SendLogToAPI(const std::string& usernameOrKey, const std::string& hwid, 
                             const std::string& ipInfo, bool isKeyLogin, const std::string& appid, 
                             const std::string& appDatabase);
    static std::string EscapeJsonString(const std::string& value);
    static std::string ParseErrorMessage(const std::string& responseBody, const std::string& defaultMessage);
    static bool SendRequest(const std::string& url, const std::string& jsonData, std::string& response, long& statusCode);
    static bool SendGetRequest(const std::string& url, std::string& response, long& statusCode);

public:
    Auth(const std::string& Application, const std::string& OwnerID);
    bool Init();
    std::string GetExpiration(const std::string& keyOrUsername, const std::string& format, bool isKey = true);
    bool CheckApiAvailability();
    AuthResult LoginWithKey(const std::string& key);
    AuthResult LoginWithUser(const std::string& username, const std::string& password);
    AuthResult RegisterUserWithKey(const std::string& username, const std::string& password, const std::string& key);
};

// Função para exibir MessageBox com o título e mensagem fornecida
void ShowMessageBoxError(const std::string& title, const std::string& message);
void ShowMessageBox(const std::string& title, const std::string& message);

// Funções estáticas antigas para compatibilidade (deprecated)
bool CheckAppStatus(const std::string& appID, const std::string& appDataBase, std::string& response);
bool LoginWithKey(const std::string& key, const std::string& appID, const std::string& appDataBase, std::string& response);
bool LoginWithUser(const std::string& username, const std::string& password, const std::string& appID, const std::string& appDataBase, std::string& response);
bool RegisterUser(const std::string& username, const std::string& password, const std::string& key, const std::string& appID, const std::string& appDataBase, std::string& response);

// Função de erro do sistema
void error(const std::string& message);

#endif
