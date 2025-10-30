#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include <cmath>

#include <webdriverxx/webdriverxx.h>
#include "monitor.h"

using namespace webdriverxx;
using namespace std;
using namespace chrono;

DailyConfig loadDailyConfig(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Файл " + filename + " не найден");
    }

    string jsonContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    DailyConfig config;
    
    regex endBetsRegex("\"end_bets\"\\s*:\\s*\"([^\"]+)\"");
    regex increaseRegex("\"increase\"\\s*:\\s*(\\d+)");
    regex inASecondRegex("\"in_a_second\"\\s*:\\s*(\\d+)");
    regex reloadRegex("\"reload\"\\s*:\\s*(\\d+)");
    
    smatch matches;
    
    if (regex_search(jsonContent, matches, endBetsRegex) && matches.size() > 1) {
        config.end_bets = matches[1];
    } else {
        throw runtime_error("Не найден end_bets в конфигурации");
    }
    
    if (regex_search(jsonContent, matches, increaseRegex) && matches.size() > 1) {
        config.increase = stoi(matches[1]);
    } else {
        throw runtime_error("Не найден increase в конфигурации");
    }
    
    if (regex_search(jsonContent, matches, inASecondRegex) && matches.size() > 1) {
        config.in_a_second = stoi(matches[1]);
    } else {
        throw runtime_error("Не найден in_a_second в конфигурации");
    }
    
    if (regex_search(jsonContent, matches, reloadRegex) && matches.size() > 1) {
        config.reload = stoi(matches[1]);
    } else {
        throw runtime_error("Не найден reload в конфигурации");
    }
    
    return config;
}

void waitUntilTime(const system_clock::time_point& target_time, const string& session_name, const string& action_name) {
    while (true) {
        auto now = system_clock::now();
        auto time_left = duration_cast<seconds>(target_time - now).count();
        
        if (time_left <= 0) {
            break;
        }
        
        string status;
        if (time_left > 3600) {
            int hours = time_left / 3600;
            int minutes = (time_left % 3600) / 60;
            status = "⏳ [" + session_name + "] До " + action_name + ": " + to_string(hours) + "ч " + to_string(minutes) + "м";
        } else if (time_left > 60) {
            int minutes = time_left / 60;
            int seconds = time_left % 60;
            status = "⏳ [" + session_name + "] До " + action_name + ": " + to_string(minutes) + "м " + to_string(seconds) + "с";
        } else {
            status = "⏳ [" + session_name + "] До " + action_name + ": " + to_string(time_left) + "с";
        }
        
        cout << status << string(10, ' ') << '\r' << flush;
        
        int sleep_time;
        if (time_left > 300) {
            sleep_time = 30;
        } else if (time_left > 60) {
            sleep_time = 10;
        } else if (time_left > 10) {
            sleep_time = 1;
        } else {
            sleep_time = 0.5;
        }
        
        this_thread::sleep_for(seconds(sleep_time));
    }
    
    cout << "🔔 [" + session_name + "] Будильник для " + action_name + "!" + string(30, ' ') << endl;
}

void refreshPageAndGetData(shared_ptr<WebDriver> driver, const string& session_name) {
    try {
        cout << "🔃 [" << session_name << "] Обновляем страницу..." << endl;
        driver->Refresh();
        
        this_thread::sleep_for(seconds(2));
        
        try {
            driver->FindElement(ByCss("table.bid-form__table"));
            cout << "✅ [" << session_name << "] Страница успешно обновлена" << endl;
        } catch (const exception&) {
            throw runtime_error("Таблица не найдена после обновления");
        }
        
        this_thread::sleep_for(seconds(2));
        
    } catch (const exception& e) {
        cout << "❌ [" << session_name << "] Ошибка при обновлении страницы: " << e.what() << endl;
    }
}

vector<Doctor> findListDoctors(shared_ptr<WebDriver> driver, const string& session_name) {
    const int max_retries = 3;
    
    for (int retry = 0; retry < max_retries; ++retry) {
        try {
            cout << "🔍 [" << session_name << "] Поиск списка врачей (попытка " << (retry + 1) << "/" << max_retries << ")..." << endl;
            
            this_thread::sleep_for(seconds(2));
            
            auto doctors = driver->FindElements(ByCss("tr.bid-form__table_full"));
            cout << "✅ [" << session_name << "] Найдено врачей: " << doctors.size() << endl;
            
            vector<Doctor> doctor_data;
            
            for (const auto& doctor : doctors) {
                try {
                    Doctor doc;
                    doc.id = doctor.GetAttribute("id");
                    
                    try {
                        auto name_element = doctor.FindElement(ByCss("td:nth-child(1)"));
                        doc.name = name_element.GetText();
                        regex newline_regex("\n");
                        doc.name = regex_replace(doc.name, newline_regex, " ");
                    } catch (const exception&) {
                        doc.name = "Неизвестно";
                    }
                    
                    try {
                        auto specialty_element = doctor.FindElement(ByCss("td:nth-child(2)"));
                        doc.specialty = specialty_element.GetText();
                    } catch (const exception&) {
                        doc.specialty = "Неизвестно";
                    }
                    
                    try {
                        auto bid_input = doctor.FindElement(ByCss("input.bid-form__inp"));
                        doc.current_bid = bid_input.GetAttribute("value");
                        doc.recommended_bid = bid_input.GetAttribute("data-recommend");
                    } catch (const exception&) {
                        doc.current_bid = "0";
                        doc.recommended_bid = "0";
                    }
                    
                    try {
                        auto toggle_inner = doctor.FindElement(ByCss(".toggle-inner"));
                        auto style = toggle_inner.GetAttribute("style");
                        
                        regex margin_regex("margin-left:\\s*([-\\d]+)px");
                        smatch matches;
                        
                        if (regex_search(style, matches, margin_regex) && matches.size() > 1) {
                            int margin_value = stoi(matches[1]);
                            doc.enabled = (margin_value == 0);
                            doc.status = doc.enabled ? "включен" : "выключен";
                        } else {
                            doc.enabled = true;
                            doc.status = "неопределен";
                        }
                    } catch (const exception&) {
                        doc.enabled = true;
                        doc.status = "неопределен";
                    }
                    
                    doctor_data.push_back(doc);
                    
                    cout << "   👤 " << doc.name << " - " << doc.specialty << endl;
                    cout << "      💰 Ставка: " << doc.current_bid << "р. (рекомендовано: " << doc.recommended_bid << "р.)" << endl;
                    cout << "      🔘 Статус: " << doc.status << endl;
                    
                } catch (const exception& e) {
                    cout << "   ❌ Ошибка чтения данных врача: " << e.what() << endl;
                    continue;
                }
            }
            
            return doctor_data;
            
        } catch (const exception& e) {
            cout << "❌ [" << session_name << "] Ошибка поиска таблицы врачей: " << e.what() << endl;
            if (retry < max_retries - 1) {
                this_thread::sleep_for(seconds(2));
                continue;
            }
        }
    }
    
    return vector<Doctor>();
}

void saveAllChanges(shared_ptr<WebDriver> driver, const string& session_name) {
    const int max_retries = 3;
    
    for (int retry = 0; retry < max_retries; ++retry) {
        try {
            cout << "💾 [" << session_name << "] Сохранение изменений (попытка " << (retry + 1) << "/" << max_retries << ")..." << endl;
            
            auto save_buttons = driver->FindElements(ByCss("button.bid-form__btn-doc-save"));
            int success_count = 0;
            
            for (const auto& button : save_buttons) {
                try {
                    button.Click();
                    this_thread::sleep_for(milliseconds(300));
                    success_count++;
                } catch (const exception& e) {
                    cout << "⚠️ [" << session_name << "] Ошибка при сохранении: " << e.what() << endl;
                    continue;
                }
            }
            
            cout << "✅ [" << session_name << "] Успешно сохранено " << success_count << "/" << save_buttons.size() << " ставок" << endl;
            break;
            
        } catch (const exception& e) {
            cout << "❌ [" << session_name << "] Ошибка при массовом сохранении: " << e.what() << endl;
            if (retry < max_retries - 1) {
                this_thread::sleep_for(seconds(2));
                continue;
            }
        }
    }
}

void updateAllBids(shared_ptr<WebDriver> driver, const string& session_name, int increase) {
    const int max_retries = 3;
    int retry_count = 0;
    
    while (retry_count < max_retries) {
        try {
            cout << "🔍 [" << session_name << "] Поиск врачей для обновления (попытка " << (retry_count + 1) << "/" << max_retries << ")..." << endl;
            
            auto doctors = findListDoctors(driver, session_name);
            int updated_count = 0;
            
            for (const auto& doctor : doctors) {
                if (!doctor.enabled) {
                    cout << "⏭️ [" << session_name << "] " << doctor.name << " пропущен (выключен)" << endl;
                    continue;
                }
                
                try {
                    int current_bid = stoi(doctor.current_bid);
                    int recommended_bid = stoi(doctor.recommended_bid);
                    int new_bid = recommended_bid + increase;
                    
                    auto bid_input = driver->FindElement(ByCss("#number-" + doctor.id));
                    bid_input.Clear();
                    bid_input.SendKeys(to_string(new_bid));
                    
                    cout << "✅ [" << session_name << "] " << doctor.name << ": " << current_bid << "р. → " << new_bid << "р. (статус: " << doctor.status << ")" << endl;
                    updated_count++;
                    
                } catch (const exception& e) {
                    cout << "❌ [" << session_name << "] Ошибка обновления ставки " << doctor.name << ": " << e.what() << endl;
                    continue;
                }
            }
            
            if (updated_count > 0) {
                saveAllChanges(driver, session_name);
                cout << "💾 [" << session_name << "] Сохранено " << updated_count << " ставок включенных врачей" << endl;
                break;
            } else {
                cout << "ℹ️ [" << session_name << "] Нет включенных врачей для обновления" << endl;
                break;
            }
            
        } catch (const exception& e) {
            cout << "❌ [" << session_name << "] Ошибка при обновлении ставок: " << e.what() << endl;
            retry_count++;
            if (retry_count < max_retries) {
                this_thread::sleep_for(seconds(2));
                continue;
            } else {
                cout << "❌ [" << session_name << "] Не удалось обновить ставки после " << max_retries << " попыток" << endl;
                break;
            }
        }
    }
}

void scheduleBidUpdates(shared_ptr<WebDriver> driver, const string& session_name) {
    int day_counter = 0;
    
    while (true) {
        day_counter++;
        cout << "\n" << string(60, '=') << endl;
        cout << "📅 [" << session_name << "] Цикл #" << day_counter << " - ";
        
        auto now = system_clock::now();
        time_t now_time = system_clock::to_time_t(now);
        cout << put_time(localtime(&now_time), "%d.%m.%Y %H:%M:%S") << endl;
        cout << string(60, '=') << endl;
        
        try {
            auto daily_config = loadDailyConfig("config.json");
            
            tm end_bets_tm = {};
            istringstream ss(daily_config.end_bets);
            ss >> get_time(&end_bets_tm, "%H:%M");
            
            auto today = system_clock::to_time_t(now);
            tm* today_tm = localtime(&today);
            
            end_bets_tm.tm_year = today_tm->tm_year;
            end_bets_tm.tm_mon = today_tm->tm_mon;
            end_bets_tm.tm_mday = today_tm->tm_mday;
            
            auto end_bets_time = system_clock::from_time_t(mktime(&end_bets_tm));
            auto update_time = end_bets_time - seconds(daily_config.in_a_second);
            auto reload_time = end_bets_time - seconds(daily_config.reload);
            
            if (now >= end_bets_time) {
                end_bets_time += hours(24);
                update_time = end_bets_time - seconds(daily_config.in_a_second);
                reload_time = end_bets_time - seconds(daily_config.reload);
                cout << "⏭️ [" << session_name << "] Время на сегодня прошло, планируем на завтра" << endl;
            }
            
            cout << "📅 [" << session_name << "] Расписание:" << endl;
            cout << "   🔄 Обновление страницы: " << put_time(localtime(&reload_time), "%H:%M:%S") << endl;
            cout << "   🚀 Обновление ставок: " << put_time(localtime(&update_time), "%H:%M:%S") << endl;
            cout << "   ⏹️ Окончание торгов: " << put_time(localtime(&end_bets_time), "%H:%M:%S") << endl;
            
            if (now < reload_time) {
                waitUntilTime(reload_time, session_name, "обновления страницы");
                cout << "🔄 [" << session_name << "] Будильник сработал - обновляем страницу" << endl;
                refreshPageAndGetData(driver, session_name);
            } else {
                cout << "⏩ [" << session_name << "] Время обновления страницы прошло" << endl;
            }
            
            if (now < update_time) {
                waitUntilTime(update_time, session_name, "обновления ставок");
                cout << "🚀 [" << session_name << "] Будильник сработал - обновляем ставки" << endl;
                updateAllBids(driver, session_name, daily_config.increase);
            } else {
                cout << "⏩ [" << session_name << "] Время обновления ставок прошло" << endl;
            }
            
            auto next_day = end_bets_time + hours(24);
            cout << "💤 [" << session_name << "] Ожидание следующего дня..." << endl;
            waitUntilTime(next_day, session_name, "следующего цикла");
            
        } catch (const exception& e) {
            cout << "❌ [" << session_name << "] Критическая ошибка в цикле: " << e.what() << endl;
            cout << "🕒 [" << session_name << "] Повтор через 5 минут..." << endl;
            this_thread::sleep_for(minutes(5));
        }
    }
}

void mainMonitor(shared_ptr<WebDriver> driver, const string& session_name) {
    cout << "🎯 [" << session_name << "] Запуск непрерывного мониторинга" << endl;
    cout << "💡 Для остановки нажмите Ctrl+C" << endl;
    
    try {
        scheduleBidUpdates(driver, session_name);
    } catch (const exception& e) {
        cout << "❌ [" << session_name << "] Непредвиденная ошибка: " << e.what() << endl;
    }
}

void standaloneMain() {
    cout << "INFO: Этот модуль должен запускаться из auth.cpp" << endl;
}