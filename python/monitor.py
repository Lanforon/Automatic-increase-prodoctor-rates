from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import StaleElementReferenceException

import time, json, re
from datetime import datetime, timedelta


def schedule_bid_updates(driver, session_name, config):
    day_counter = 0
    
    while True:
        day_counter += 1
        print(f"\n{'='*60}")
        print(f"📅 [{session_name}] Цикл #{day_counter} - {datetime.now().strftime('%d.%m.%Y %H:%M:%S')}")
        print(f"{'='*60}")
        
        try:
            with open('config.json', 'r', encoding='utf-8') as f:
                daily_config = json.load(f)
            
            end_bets_time = datetime.strptime(daily_config['end_bets'], "%H:%M").time()
            increase = daily_config['increase']
            in_a_second = daily_config['in_a_second']
            reload = daily_config['reload']
            
            now = datetime.now()
            end_bets_datetime = datetime.combine(now.date(), end_bets_time)
            
            if now >= end_bets_datetime:
                end_bets_datetime += timedelta(days=1)
                print(f"⏭️ [{session_name}] Время на сегодня прошло, планируем на завтра")
            
            update_datetime = end_bets_datetime - timedelta(seconds=in_a_second)
            reload_datetime = end_bets_datetime - timedelta(seconds=reload)
            
            print(f"📅 [{session_name}] Расписание на {end_bets_datetime.strftime('%d.%m.%Y')}:")
            print(f"   🔄 Обновление страницы: {reload_datetime.strftime('%H:%M:%S')}")
            print(f"   🚀 Обновление ставок: {update_datetime.strftime('%H:%M:%S')}")
            print(f"   ⏹️ Окончание торгов: {end_bets_datetime.strftime('%H:%M:%S')}")
            
            # Будильник для обновления страницы
            if now < reload_datetime:
                wait_until_time(reload_datetime, session_name, "обновления страницы")
                print(f"🔄 [{session_name}] Будильник сработал - обновляем страницу")
                refresh_page_and_get_data(driver, session_name)
            else:
                print(f"⏩ [{session_name}] Время обновления страницы прошло")
            
            # Будильник для обновления ставок
            if now < update_datetime:
                wait_until_time(update_datetime, session_name, "обновления ставок")
                print(f"🚀 [{session_name}] Будильник сработал - обновляем ставки")
                update_all_bids(driver, session_name, increase)
            else:
                print(f"⏩ [{session_name}] Время обновления ставок прошло")
            
            # После обновления ставок ждём до завтра
            next_day = end_bets_datetime + timedelta(days=1)
            print(f"💤 [{session_name}] Ожидание следующего дня...")
            wait_until_time(next_day, session_name, "следующего цикла")
            
        except Exception as e:
            print(f"❌ [{session_name}] Критическая ошибка в цикле: {e}")
            print(f"🕒 [{session_name}] Повтор через 5 минут...")
            time.sleep(300)  # Ждём 5 минут перед повторной попыткой


def wait_until_time(target_datetime, session_name, action_name):
    while True:
        now = datetime.now()
        time_left = (target_datetime - now).total_seconds()
        
        if time_left <= 0:
            break
            
        if time_left > 3600:  
            hours = int(time_left // 3600)
            minutes = int((time_left % 3600) // 60)
            status = f"⏳ [{session_name}] До {action_name}: {hours}ч {minutes}м"
        elif time_left > 60:  
            minutes = int(time_left // 60)
            seconds = int(time_left % 60)
            status = f"⏳ [{session_name}] До {action_name}: {minutes}м {seconds}с"
        else: 
            status = f"⏳ [{session_name}] До {action_name}: {int(time_left)}с"
        
        print(status + " " * 10, end='\r')
        
        if time_left > 300:  
            sleep_time = 30
        elif time_left > 60:  
            sleep_time = 10
        elif time_left > 10:  
            sleep_time = 1
        else:  
            sleep_time = 0.5
            
        time.sleep(sleep_time)
    
    print(f"🔔 [{session_name}] Будильник для {action_name}!" + " " * 30)


def refresh_page_and_get_data(driver, session_name):
    try:
        print(f"🔃 [{session_name}] Обновляем страницу...")
        driver.refresh()
        
        # Ждем полной загрузки страницы
        WebDriverWait(driver, 20).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, "table.bid-form__table"))
        )
        
        print(f"✅ [{session_name}] Страница успешно обновлена")
        time.sleep(2)
        
        doctors = find_list_doctors(driver, session_name)
        print(f"📊 [{session_name}] Актуальные данные получены")
        return doctors
        
    except Exception as e:
        print(f"❌ [{session_name}] Ошибка при обновлении страницы: {e}")
        return []


def update_all_bids(driver, session_name, increase):
    max_retries = 3
    retry_count = 0
    
    while retry_count < max_retries:
        try:
            print(f"🔍 [{session_name}] Поиск врачей для обновления (попытка {retry_count + 1}/{max_retries})...")
            doctors = find_list_doctors(driver, session_name)
            updated_count = 0
            
            for doctor in doctors:
                if not doctor['enabled']:
                    print(f"⏭️ [{session_name}] {doctor['name']} пропущен (выключен)")
                    continue
                    
                try:
                    current_bid = int(doctor['current_bid'])
                    recommended_bid = int(doctor['recommended_bid'])
                    
                    new_bid = recommended_bid + increase
                    
                    bid_input = WebDriverWait(driver, 10).until(
                        EC.element_to_be_clickable((By.CSS_SELECTOR, f"#number-{doctor['id']}"))
                    )
                    
                    bid_input.clear()
                    bid_input.send_keys(str(new_bid))
                    
                    print(f"✅ [{session_name}] {doctor['name']}: {current_bid}р. → {new_bid}р. (статус: {doctor['status']})")
                    updated_count += 1
                    
                except StaleElementReferenceException:
                    print(f"🔄 [{session_name}] Элемент устарел для {doctor['name']}, повторяем...")
                    continue
                except Exception as e:
                    print(f"❌ [{session_name}] Ошибка обновления ставки {doctor['name']}: {e}")
                    continue
            
            if updated_count > 0:
                save_all_changes(driver, session_name)
                print(f"💾 [{session_name}] Сохранено {updated_count} ставок включенных врачей")
                break  
            else:
                print(f"ℹ️ [{session_name}] Нет включенных врачей для обновления")
                break
                
        except StaleElementReferenceException:
            retry_count += 1
            print(f"🔄 [{session_name}] Все элементы устарели, попытка {retry_count}/{max_retries}")
            if retry_count < max_retries:
                time.sleep(2)
                continue
            else:
                print(f"❌ [{session_name}] Не удалось обновить ставки после {max_retries} попыток")
                break
        except Exception as e:
            print(f"❌ [{session_name}] Ошибка при обновлении ставок: {e}")
            break


def save_all_changes(driver, session_name):
    max_retries = 3
    
    for retry in range(max_retries):
        try:
            print(f"💾 [{session_name}] Сохранение изменений (попытка {retry + 1}/{max_retries})...")
            
            save_buttons = WebDriverWait(driver, 10).until(
                EC.presence_of_all_elements_located((By.CSS_SELECTOR, "button.bid-form__btn-doc-save"))
            )
            success_count = 0
            for button in save_buttons:
                try:
                    button.click()
                    time.sleep(0.3) 
                    success_count += 1
                except StaleElementReferenceException:
                    print(f"🔄 [{session_name}] Кнопка сохранения устарела, продолжаем...")
                    continue
                except Exception as e:
                    print(f"⚠️ [{session_name}] Ошибка при сохранении: {e}")
                    continue
            
            print(f"✅ [{session_name}] Успешно сохранено {success_count}/{len(save_buttons)} ставок")
            break
            
        except StaleElementReferenceException:
            print(f"🔄 [{session_name}] Элементы сохранения устарели, повтор...")
            if retry < max_retries - 1:
                time.sleep(2)
                continue
        except Exception as e:
            print(f"❌ [{session_name}] Ошибка при массовом сохранении: {e}")
            break


def find_list_doctors(driver, session_name):
    max_retries = 3
    
    for retry in range(max_retries):
        try:
            print(f"🔍 [{session_name}] Поиск списка врачей (попытка {retry + 1}/{max_retries})...")
            
            WebDriverWait(driver, 20).until(
                EC.presence_of_element_located((By.CSS_SELECTOR, "table.bid-form__table"))
            )
            
            doctors = driver.find_elements(By.CSS_SELECTOR, "tr.bid-form__table_full")
            print(f"✅ [{session_name}] Найдено врачей: {len(doctors)}")
            
            doctor_data = []
            
            for doctor in doctors:
                try:
                    doctor_id = doctor.get_attribute('id')
                    name = doctor.find_element(By.CSS_SELECTOR, "td:nth-child(1)").text.replace('\n', ' ').strip()
                    specialty = doctor.find_element(By.CSS_SELECTOR, "td:nth-child(2)").text
                    
                    bid_input = doctor.find_element(By.CSS_SELECTOR, "input.bid-form__inp")
                    current_bid = bid_input.get_attribute('value')
                    recommended_bid = bid_input.get_attribute('data-recommend')
                    
                    try:
                        toggle_inner = doctor.find_element(By.CSS_SELECTOR, ".toggle-inner")
                        style = toggle_inner.get_attribute("style")
                        
                        margin_match = re.search(r'margin-left:\s*([-\d]+)px', style)
                        if margin_match:
                            margin_value = int(margin_match.group(1))
                            is_enabled = (margin_value == 0)
                            status = "включен" if is_enabled else "выключен"
                        else:
                            toggle_off = doctor.find_element(By.CSS_SELECTOR, ".toggle-off")
                            is_enabled = "active" not in toggle_off.get_attribute("class")
                            status = "включен" if is_enabled else "выключен"
                            
                    except Exception as status_error:
                        print(f"   ⚠️ Не удалось определить статус для {name}: {status_error}")
                        is_enabled = True
                        status = "неопределен"
                    
                    doctor_data.append({
                        'id': doctor_id,
                        'name': name,
                        'specialty': specialty,
                        'current_bid': current_bid,
                        'recommended_bid': recommended_bid,
                        'status': status,
                        'enabled': is_enabled
                    })
                    
                    print(f"   👤 {name} - {specialty}")
                    print(f"      💰 Ставка: {current_bid}р. (рекомендовано: {recommended_bid}р.)")
                    print(f"      🔘 Статус: {status}")
                    
                except StaleElementReferenceException:
                    print(f"   🔄 Элемент врача устарел, пропускаем...")
                    continue
                except Exception as e:
                    print(f"   ❌ Ошибка чтения данных врача: {e}")
                    continue
            
            return doctor_data
            
        except StaleElementReferenceException:
            print(f"🔄 [{session_name}] Элементы таблицы устарели, повторная попытка...")
            if retry < max_retries - 1:
                time.sleep(2)
                continue
        except Exception as e:
            print(f"❌ [{session_name}] Ошибка поиска таблицы врачей: {e}")
    
    return []


def main(driver, session_name="Сессия 1"):
    print(f"🎯 [{session_name}] Запуск непрерывного мониторинга")
    print(f"💡 Для остановки нажмите Ctrl+C")
    
    try:
        schedule_bid_updates(driver, session_name, config=None)
    except KeyboardInterrupt:
        print(f"\n🛑 [{session_name}] Работа остановлена пользователем")
    except Exception as e:
        print(f"❌ [{session_name}] Непредвиденная ошибка: {e}")
    finally:
        print(f"👋 [{session_name}] Скрипт завершил работу")


if __name__ == "__main__":
    print("INFO: Этот скрипт должен запускаться из auth.py")