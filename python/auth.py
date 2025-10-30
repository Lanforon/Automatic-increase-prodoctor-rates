from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
import platform, os, json, time
import threading


def setup_driver():
    try:
        if platform.system() == "Windows":
            os.environ['WDM_PLATFORM'] = 'win64'
            driver_path = ChromeDriverManager(driver_version="latest").install()
        else:
            driver_path = ChromeDriverManager().install()
        
        print(f"✅ ChromeDriver: {driver_path}")
        
        if not os.path.exists(driver_path):
            raise FileNotFoundError(f"ChromeDriver не найден: {driver_path}")
        
        chrome_options = Options()
        chrome_options.add_argument("--disable-blink-features=AutomationControlled")
        chrome_options.add_argument("--disable-extensions")
        chrome_options.add_argument("--no-sandbox")
        chrome_options.add_argument("--disable-dev-shm-usage")
        chrome_options.add_argument("--disable-gpu")
        chrome_options.add_argument("--window-size=1920,1080")
        chrome_options.add_argument("--start-maximized")
        chrome_options.add_experimental_option("excludeSwitches", ["enable-automation"])
        chrome_options.add_experimental_option('useAutomationExtension', False)
        
        service = Service(driver_path)
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.execute_script("Object.defineProperty(navigator, 'webdriver', {get: () => undefined})")
        
        print("✅ ChromeDriver запущен")
        return driver
        
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        return setup_driver_manual()

def setup_driver_manual():
    try:
        driver_path = ChromeDriverManager(version="latest", os_type="win64").install()
        chrome_options = Options()
        chrome_options.add_argument("--disable-blink-features=AutomationControlled")
        chrome_options.add_argument("--no-sandbox")
        chrome_options.add_argument("--disable-dev-shm-usage")
        service = Service(driver_path)
        driver = webdriver.Chrome(service=service, options=chrome_options)
        print("✅ ChromeDriver запущен вручную")
        return driver
    except Exception as e:
        print(f"❌ Ошибка ручной настройки: {e}")
        return setup_driver_fallback()

def setup_driver_fallback():
    try:
        chrome_options = Options()
        chrome_options.add_argument("--disable-blink-features=AutomationControlled")
        driver_path = ChromeDriverManager().install()
        service = Service(driver_path)
        driver = webdriver.Chrome(service=service, options=chrome_options)
        print("✅ ChromeDriver запущен через fallback")
        return driver
    except Exception as e:
        print(f"❌ Все методы не удались: {e}")
        return None
    
def load_credentials(filename='config.json'):
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            config = json.load(f)
        email = config.get('email')
        password = config.get('password')
        if not email or not password:
            raise ValueError("JSON должен содержать email и password")
        return email, password
    except FileNotFoundError:
        print(f"❌ Файл {filename} не найден")
        return None, None
    except json.JSONDecodeError:
        print(f"❌ Ошибка JSON в файле {filename}")
        return None, None
    except Exception as e:
        print(f"❌ Ошибка чтения: {e}")
        return None, None

def prodoctorov_login(driver, email, password):
    try:
        wait = WebDriverWait(driver, 20)
        driver.get("https://prodoctorov.ru/profile/login/?next=https%3A%2F%2Fprodoctorov.ru%2F")
        
        email_field = wait.until(EC.presence_of_element_located((By.NAME, "username")))
        password_field = wait.until(EC.presence_of_element_located((By.NAME, "password")))
        
        email_field.clear()
        email_field.send_keys(email)
        password_field.clear()
        password_field.send_keys(password)

        login_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//button[@type='submit']")))
        login_button.click()
        
        print("🎉 Успешный вход в Личный кабинет!")
        return True
        
    except Exception as e:
        print(f"❌ Ошибка входа: {e}")
        return False

def browser_session(session_name, config_file, redirect_function):
    print(f"🚀 Запуск сессии: {session_name}")
    email, password = load_credentials(config_file)
    if not email or not password:
        print(f"❌ Не удалось загрузить учетные данные для {session_name}")
        return
    driver = setup_driver()
    if not driver:
        print(f"❌ Не удалось запустить браузер для {session_name}")
        return
    try:
        if prodoctorov_login(driver, email, password):
            redirect_function(driver)
            input(f"⏸️ {session_name} активна. Нажмите Enter для завершения...")
    finally:
        driver.quit()
        print(f"✅ {session_name} завершена")

def redirect_session_1(driver):
    from redirect import redirect_session
    redirect_session(driver, change_filial=False)

def redirect_session_2(driver):
    from redirect import redirect_session
    redirect_session(driver, change_filial=True)

def run_multiple_browsers():
    print("🚀 Запуск нескольких сессий браузера...")
    
    session_filial1 = threading.Thread(target=browser_session, args=("Сессия 1", "config.json", redirect_session_1))
    session_filial2 = threading.Thread(target=browser_session, args=("Сессия 2", "config.json", redirect_session_2))
    
    session_filial1.start()
    time.sleep(1)
    session_filial2.start()
    session_filial1.join()
    session_filial2.join()

if __name__ == "__main__":
    run_multiple_browsers()