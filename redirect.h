#ifndef REDIRECT_H
#define REDIRECT_H

#include <memory>
#include <webdriverxx/webdriverxx.h>

using namespace webdriverxx;
using namespace std;

void redirectSession(shared_ptr<WebDriver> driver, bool changeFilial);

void redirectSession1(shared_ptr<WebDriver> driver);
void redirectSession2(shared_ptr<WebDriver> driver);

bool waitAndClick(shared_ptr<WebDriver> driver, const By& by, const string& selector, 
                  int timeout = 20, int retries = 3);
bool waitForElement(shared_ptr<WebDriver> driver, const By& by, int timeout = 20);

#endif