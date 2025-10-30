#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <stdexcept>

#include <webdriverxx/webdriverxx.h>
#include "monitor.h"

using namespace webdriverxx;
using namespace std;

bool waitAndClick(shared_ptr<WebDriver> driver, const By& by, const string& selector, 
                  int timeout = 20, int retries = 3) {
    for (int attempt = 0; attempt < retries; ++attempt) {
        try {
            auto startTime = chrono::steady_clock::now();
            while (chrono::steady_clock::now() - startTime < chrono::seconds(timeout)) {
                try {
                    auto element = driver->FindElement(by);
                    if (element.IsEnabled() && element.IsDisplayed()) {
                        driver->ExecuteScript("arguments[0].scrollIntoView({block: 'center'});", element);
                        
                        this_thread::sleep_for(chrono::milliseconds(500));
                        
                        element.Click();
                        return true;
                    }
                } catch (const exception&) {
                }
                this_thread::sleep_for(chrono::milliseconds(500));
            }
            
            cout << "⏰ Таймаут ожидания элемента, попытка " << (attempt + 1) << "/" << retries << endl;
            if (attempt == retries - 1) {
                throw runtime_error("Не удалось кликнуть на элемент после " + to_string(retries) + " попыток");
            }
            this_thread::sleep_for(chrono::seconds(2));
            
        } catch (const exception& e) {
            if (string(e.what()).find("stale element") != string::npos) {
                cout << "🔄 Элемент устарел, попытка " << (attempt + 1) << "/" << retries << endl;
                this_thread::sleep_for(chrono::seconds(1));
            } else {
                cout << "❌ Ошибка при клике: " << e.what() << ", попытка " << (attempt + 1) << "/" << retries << endl;
                if (attempt == retries - 1) {
                    throw;
                }
                this_thread::sleep_for(chrono::seconds(2));
            }
        }
    }
    return false;
}

bool waitForElement(shared_ptr<WebDriver> driver, const By& by, int timeout = 20) {
    auto startTime = chrono::steady_clock::now();
    while (chrono::steady_clock::now() - startTime < chrono::seconds(timeout)) {
        try {
            auto element = driver->FindElement(by);
            if (element.IsDisplayed()) {
                return true;
            }
        } catch (const exception&) {
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    return false;
}

void redirectSession(shared_ptr<WebDriver> driver, bool changeFilial) {
    cout << "🚀 Начинаем редирект по страницам" << endl;
    
    try {
        waitAndClick(driver, ByXPath("//a[@class='b-button b-button_small b-button_light' and contains(., 'Личный кабинет')]"));
        
        if (!waitForElement(driver, ByCss("a[data-qa='placement']"), 20)) {
            throw runtime_error("Элемент размещения не найден");
        }
        
        waitAndClick(driver, ByCss("a[data-qa='placement']"));
        
        if (changeFilial) {
            if (!waitForElement(driver, ByCss("select.change_profile_lpu"), 20)) {
                throw runtime_error("Select для смены филиала не найден");
            }
            
            auto selectElement = driver->FindElement(ByCss("select.change_profile_lpu"));
            selectElement.Click();
            
            if (!waitForElement(driver, ByXPath("//option[@value='101530']"), 20)) {
                throw runtime_error("Опция филиала не найдена");
            }
            
            auto optionElement = driver->FindElement(ByXPath("//option[@value='101530']"));
            optionElement.Click();
        }
        
        // Ожидаем и кликаем на "Врачи"
        if (!waitForElement(driver, ByXPath("//a[text()='Врачи' and @href='/money/placement/lpu/speciality/']"), 20)) {
            throw runtime_error("Ссылка 'Врачи' не найдена");
        }
        
        waitAndClick(driver, ByXPath("//a[text()='Врачи' and @href='/money/placement/lpu/speciality/']"));
        cout << "🎉 Все переходы успешно выполнены!" << endl;
        
    } catch (const exception& e) {
        cout << "❌ Ошибка при выполнении редиректа: " << e.what() << endl;
        
        try {
            // Альтернативные селекторы
            cout << "🔄 Пробуем альтернативные селекторы..." << endl;
            
            waitAndClick(driver, ByXPath("//a[contains(@href, '/profile/') and contains(., 'Личный кабинет')]"));
            waitAndClick(driver, ByXPath("//a[contains(@class, 'v-list-item') and .//div[contains(text(), 'Спецразмещение')]]"));
            
            if (changeFilial) {
                waitAndClick(driver, ByCss("select.change_profile_lpu"));
                waitAndClick(driver, ByXPath("//option[@value='101530']"));
            }
            
            waitAndClick(driver, ByCss("a[href='/money/placement/lpu/speciality/']"));
            cout << "🎉 Переходы выполнены через альтернативные селекторы!" << endl;
            
        } catch (const exception& alt_e) {
            cout << "❌ Критическая ошибка: " << alt_e.what() << endl;
            throw;
        }
    }

    if (changeFilial) {
        monitorMain(driver, "Сессия 2");
    } else {
        monitorMain(driver, "Сессия 1");
    }
    
}

void mainFunction(shared_ptr<WebDriver> driver) {
    cout << "INFO: Этот модуль должен запускаться из auth.cpp" << endl;
}

void redirectSession1(shared_ptr<WebDriver> driver) {
    redirectSession(driver, false);
}

void redirectSession2(shared_ptr<WebDriver> driver) {
    redirectSession(driver, true);
}