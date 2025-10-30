import subprocess
import sys
import time

def main():
    print("🚀 Запуск двух сессий браузера...")
    
    # Запускаем первую сессию
    proc1 = subprocess.Popen([sys.executable, "auth.py", "--session=1"])
    time.sleep(5)
    
    # Запускаем вторую сессию
    proc2 = subprocess.Popen([sys.executable, "auth.py", "--session=2"])
    
    # Ждем завершения
    proc1.wait()
    proc2.wait()
    
    print("✅ Все сессии завершены")

if __name__ == "__main__":
    main()