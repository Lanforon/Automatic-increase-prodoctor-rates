#ifndef MONITOR_H
#define MONITOR_H

#include <memory>
#include <string>
#include <webdriverxx/webdriverxx.h>

using namespace webdriverxx;
using namespace std;

struct Doctor {
    string id;
    string name;
    string specialty;
    string current_bid;
    string recommended_bid;
    string status;
    bool enabled;
};

struct DailyConfig {
    string end_bets;
    int increase;
    int in_a_second;
    int reload;
};

void scheduleBidUpdates(shared_ptr<WebDriver> driver, const string& session_name);
void mainMonitor(shared_ptr<WebDriver> driver, const string& session_name = "Сессия 1");

void waitUntilTime(const chrono::system_clock::time_point& target_time, const string& session_name, const string& action_name);
vector<Doctor> findListDoctors(shared_ptr<WebDriver> driver, const string& session_name);
void refreshPageAndGetData(shared_ptr<WebDriver> driver, const string& session_name);
void updateAllBids(shared_ptr<WebDriver> driver, const string& session_name, int increase);
void saveAllChanges(shared_ptr<WebDriver> driver, const string& session_name);
DailyConfig loadDailyConfig(const string& filename = "config.json");

#endif