#ifndef AUTH_H
#define AUTH_H

#include <memory>
#include <string>
#include <webdriverxx/webdriverxx.h>

using namespace webdriverxx;
using namespace std;

struct Config {
    string email;
    string password;
};

Config loadCredentials(const string& filename = "config.json");
shared_ptr<WebDriver> setupDriver();
bool prodoctorovLogin(shared_ptr<WebDriver> driver, const string& email, const string& password);
void redirectSession1(shared_ptr<WebDriver> driver);
void redirectSession2(shared_ptr<WebDriver> driver);
void browserSession(const string& sessionName, const string& configFile, 
                   void (*redirectFunction)(shared_ptr<WebDriver>));
void runMultipleBrowsers();

#endif