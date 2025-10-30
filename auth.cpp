#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <memory>

#include <webdriverxx/webdriverxx.h>
#include "redirect.h"

using namespace webdriverxx;
using namespace std;

// Конфигурация
struct Config {
    string email;
    string password;
};

Config loadCredentials(const string& filename = "config.json") {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Файл " + filename + " не найден");
    }

    string jsonContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    size_t emailPos = jsonContent.find("\"email\"");
    size_t passwordPos = jsonContent.find("\"password\"");
    
    if (emailPos == string::npos || passwordPos == string::npos) {
        throw runtime_error("JSON должен содержать email и password");
    }

    Config config;
    
    size_t emailStart = jsonContent.find(":", emailPos) + 1;
    size_t emailEnd = jsonContent.find(",", emailStart);
    if (emailEnd == string::npos) emailEnd = jsonContent.find("}", emailStart);
    config.email = jsonContent.substr(emailStart, emailEnd - emailStart);
    config.email = config.email.substr(config.email.find("\"") + 1);
    config.email = config.email.substr(0, config.email.find_last_of("\""));
    
    size_t passwordStart = jsonContent.find(":", passwordPos) + 1;
    size_t passwordEnd = jsonContent.find(",", passwordStart);
    if (passwordEnd == string::npos) passwordEnd = jsonContent.find("}", passwordStart);
    config.password = jsonContent.substr(passwordStart, passwordEnd - passwordStart);
    config.password = config.password.substr(config.password.find("\"") + 1);
    config.password = config.password.substr(0, config.password.find_last_of("\""));

    if (config.email.empty() || config.password.empty()) {
        throw runtime_error("Email или password пустые в конфигурационном файле");
    }

    return config;
}

shared_ptr<WebDriver> setupDriver() {
    try {
        Capabilities capabilities;
        capabilities.SetVersion(""); 
        capabilities.SetBrowserName(browser::Chrome);
        
        Json::Value chromeOptions;
        chromeOptions["args"] = Json::arrayValue;
        chromeOptions["args"].append("--disable-blink-features=AutomationControlled");
        chromeOptions["args"].append("--disable-extensions");
        chromeOptions["args"].append("--no-sandbox");
        chromeOptions["args"].append("--disable-dev-shm-usage");
        chromeOptions["args"].append("--disable-gpu");
        chromeOptions["args"].append("--window-size=1920,1080");
        chromeOptions["args"].append("--start-maximized");
        
        Json::Value excludeSwitches;
        excludeSwitches["excludeSwitches"] = Json::arrayValue;
        excludeSwitches["excludeSwitches"].append("enable-automation");
        chromeOptions["excludeSwitches"] = excludeSwitches["excludeSwitches"];
        
        capabilities.Set("goog:chromeOptions", chromeOptions);
        
        auto driver = make_shared<WebDriver>(webdriverxx::StartWith(capabilities));
        
        driver->ExecuteScript("Object.defineProperty(navigator, 'webdriver', {get: () => undefined})");
        
        cout << "✅ ChromeDriver запущен" << endl;
        return driver;
        
    } catch (const exception& e) {
        cerr << "❌ Ошибка настройки драйвера: " << e.what() << endl;
        return nullptr;
    }
}

bool prodoctorovLogin(shared_ptr<WebDriver> driver, const string& email, const string& password) {
    try {
        driver->Get("https://prodoctorov.ru/profile/login/?next=https%3A%2F%2Fprodoctorov.ru%2F");
        
        this_thread::sleep_for(chrono::seconds(2));
        
        auto emailField = driver->FindElement(ByName("username"));
        emailField.Clear();
        emailField.SendKeys(email);
        
        auto passwordField = driver->FindElement(ByName("password"));
        passwordField.Clear();
        passwordField.SendKeys(password);
        
        auto loginButton = driver->FindElement(ByXPath("//button[@type='submit']"));
        loginButton.Click();
        
        this_thread::sleep_for(chrono::seconds(3));
        
        cout << "🎉 Успешный вход в Личный кабинет!" << endl;
        return true;
        
    } catch (const exception& e) {
        cerr << "❌ Ошибка входа: " << e.what() << endl;
        return false;
    }
}

// Функции редиректа - исправленные вызовы
void redirectSession1(shared_ptr<WebDriver> driver) {
    cout << "🚀 Выполняется редирект сессии 1" << endl;
    redirectSession(driver, false);  // Исправлено: redirectSession вместо redirect_session
}

void redirectSession2(shared_ptr<WebDriver> driver) {
    cout << "🚀 Выполняется редирект сессии 2" << endl;
    redirectSession(driver, true);   // Исправлено: redirectSession вместо redirect_session
}

void browserSession(const string& sessionName, const string& configFile, 
                   void (*redirectFunction)(shared_ptr<WebDriver>)) {
    cout << "🚀 Запуск сессии: " << sessionName << endl;
    
    try {
        Config config = loadCredentials(configFile);
        
        auto driver = setupDriver();
        if (!driver) {
            cerr << "❌ Не удалось запустить браузер для " << sessionName << endl;
            return;
        }
        
        try {
            if (prodoctorovLogin(driver, config.email, config.password)) {
                redirectFunction(driver);
                cout << "⏸️ " << sessionName << " активна. Нажмите Enter для завершения...";
                cin.get();
            }
        } catch (...) {
            driver->Quit();
            throw;
        }
        
        driver->Quit();
        cout << "✅ " << sessionName << " завершена" << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Ошибка в сессии " << sessionName << ": " << e.what() << endl;
    }
}

void runMultipleBrowsers() {
    cout << "🚀 Запуск нескольких сессий браузера..." << endl;
    
    thread session1(browserSession, "Сессия 1", "config.json", redirectSession1);
    this_thread::sleep_for(chrono::seconds(1));
    thread session2(browserSession, "Сессия 2", "config.json", redirectSession2);
    
    session1.join();
    session2.join();
}

int main() {
    try {
        runMultipleBrowsers();
    } catch (const exception& e) {
        cerr << "❌ Критическая ошибка: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}