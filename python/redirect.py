from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException, StaleElementReferenceException
import time

def wait_and_click(driver, by, selector, timeout=20, retries=3):
    for attempt in range(retries):
        try:
            element = WebDriverWait(driver, timeout).until(
                EC.element_to_be_clickable((by, selector))
            )
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", element)
            time.sleep(0.5)
            
            element.click()
            return True
            
        except StaleElementReferenceException:
            print(f"🔄 Элемент устарел, попытка {attempt + 1}/{retries}")
            time.sleep(1)
        except TimeoutException:
            print(f"⏰ Таймаут ожидания элемента, попытка {attempt + 1}/{retries}")
            if attempt == retries - 1:
                raise
            time.sleep(2)
    
    return False

def redirect_session(driver, change_filial):
    print("🚀 Начинаем редирект по страницам")
    
    try:
        wait_and_click(driver, By.XPATH, "//a[@class='b-button b-button_small b-button_light' and contains(., 'Личный кабинет')]")
        
        WebDriverWait(driver, 20).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, "a[data-qa='placement']"))
        )

        wait_and_click(driver, By.CSS_SELECTOR, "a[data-qa='placement']")
        
        if change_filial:
            WebDriverWait(driver, 20).until(
                EC.presence_of_element_located((By.CSS_SELECTOR, "select.change_profile_lpu"))
            )
            select_element = driver.find_element(By.CSS_SELECTOR, "select.change_profile_lpu")
            select_element.click()
            
            WebDriverWait(driver, 20).until(
                EC.presence_of_element_located((By.XPATH, "//option[@value='101530']"))
            )
            option_element = driver.find_element(By.XPATH, "//option[@value='101530']")
            option_element.click()
        
        WebDriverWait(driver, 20).until(
            EC.presence_of_element_located((By.XPATH, "//a[text()='Врачи' and @href='/money/placement/lpu/speciality/']"))
        )
        wait_and_click(driver, By.XPATH, "//a[text()='Врачи' and @href='/money/placement/lpu/speciality/']")
        print("🎉 Все переходы успешно выполнены!")
        
    except Exception as e:
        print(f"❌ Ошибка при выполнении редиректа: {e}")
        try:
            wait_and_click(driver, By.XPATH, "//a[contains(@href, '/profile/') and contains(., 'Личный кабинет')]")
            wait_and_click(driver, By.XPATH, "//a[contains(@class, 'v-list-item') and .//div[contains(text(), 'Спецразмещение')]]")
            
            if change_filial:
                wait_and_click(driver, By.CSS_SELECTOR, "select.change_profile_lpu")
                wait_and_click(driver, By.XPATH, "//option[@value='101530']")
                
            wait_and_click(driver, By.CSS_SELECTOR, "a[href='/money/placement/lpu/speciality/']")
            print("🎉 Переходы выполнены через альтернативные селекторы!")
            
        except Exception as alt_e:
            print(f"❌ Критическая ошибка: {alt_e}")

    if(change_filial):
        from monitor import main
        main(driver, "Сессия 2")
    else:
        from monitor import main
        main(driver, "Сессия 1")


def main(driver):
    success = redirect_session(driver)

if __name__ == "__main__":
    print("INFO: Этот скрипт должен запускаться из auth.py")