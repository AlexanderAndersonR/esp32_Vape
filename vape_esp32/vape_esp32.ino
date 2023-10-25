

//-----------------------------------НАСТРОЙКИ------------------------------------
int sleep_timer = 10;    // таймер сна в секундах
int vape_threshold = 3;  // отсечка затяжки, в секундах
float battery_low = 3;   // нижний порог срабатывания защиты от переразрядки аккумулятора, в Вольтах!
//#define WDT_TIMEOUT 3       //таймер для сторожевого таймера
//-----------------------------------НАСТРОЙКИ МИГАНИЯ КНОПК----------------------
#define LED_button 5
#define PWM1_Ch 0
#define PWM1_Res 8
#define PWM1_Freq 1000
#define max_led 50
#define blink_led 5
int led_con = max_led;
bool led;
//-----------------------------------НАСТРОЙКИ------------------------------------
//#include <esp_task_wdt.h>   // библиотека для сторожевого таймера
#include <esp32-hal-ledc.h>  // библиотека для работы с шим
#include <EEPROM.h>
#define pwmChannel 1
#define resolution 10
int frequency;
//-----------кнопки-----------
#define button 33        // кнопка
#define bat_vol_pin 34   //пин измерения напряжения
#define time_http 10000  //таймер включения http 10 сек
//-----------кнопки-----------

//-----------флажки-----------
boolean button_state;
boolean up_state, down_state, set_state, vape_state;
boolean up_flag, down_flag, set_flag, set_flag_hold, set_hold, vape_btt, vape_btt_f;
volatile boolean wake_up_flag, vape_flag;
boolean change_v_flag, change_w_flag, change_o_flag;
volatile byte mode, mode_flag = 1;
boolean flag;  // флаг, разрешающий подать ток на койл (защита от КЗ, обрыва, разрядки)
//-----------флажки-----------

//-----------пины-------------
#define mosfet 27  // пин мосфета (нагрев спирали)
//#define battery 6      // пин измерения напряжения акума
//-----------пины-------------

float bat_vol_old, bat_volt_f;  // хранит напряжение на акуме
float PWM, PWM_f, PWM_old;      // хранит PWM сигнал

//-------переменные и коэффициенты для питания-------
float filter_k = 0.04;
float PWM_filter_k = 0.1;

#define VREF 3.3      // напряжение esp32
#define DIV_R1 10000  // значение 10 кОм резистора
#define DIV_R2 4700   // значение 4.7 кОм резистора
//-------переменные и коэффициенты для питания-------
float voltage;
unsigned long last_time, vape_press, set_press, last_vape, wake_timer, button_led_time, stop_led;  // таймеры
int watts;                                                                                         // храним вольты и ватты
float ohms, volts;                                                                                 // храним омы
//float my_vcc_const;  // константа вольтметра
volatile byte vape_mode, vape_release_count;

//------------------wifi-----------------------
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// Replace with your network credentials
const char* ssid = "Vape-Point";
const char* password = "123456789";
const char* frequency_input = "frequency";
const char* sleep_timer_input = "sleep_timer";
const char* vape_threshold_input = "vape_threshold";
const char* battery_low_input = "battery_low";
const char* watts_input = "watts";
const char* ohms_input = "ohms";
const char* Reboot = "Reboot";
// Set web server port number to 80
AsyncWebServer server(80);

bool server_flag, http_flag;
struct Data {
  int frequency;
  int sleep_timer;     // таймер сна в секундах
  int vape_threshold;  // отсечка затяжки, в секундах
  float battery_low;   // минимальный предел аккумулятора
  int watts;           // храним вольты и ватты
  float ohms, volts;   // храним омы
};
Data data;
bool but_pres_1, but_pres_2;
//------------------wifi-----------------------
void setup() {
  Serial.begin(115200);
  EEPROM.begin(200);
  EEPROM.get(0, data);  //чтение данных из памяти
  set_param();
  //---настройка кнопок и выходов-----
  pinMode(button, INPUT_PULLUP);
  pinMode(LED_button, OUTPUT);
  digitalWrite(LED_button, HIGH);
  pinMode(mosfet, OUTPUT);
  digitalWrite(mosfet, LOW);  // принудительно отключить койл
  //---настройка кнопок и выходов-----
  ledcSetup(pwmChannel, frequency, resolution);
  ledcSetup(PWM1_Ch, PWM1_Freq, PWM1_Res);
  // подключаем ШИМ-канал к пину светодиода:
  ledcAttachPin(mosfet, pwmChannel);
  ledcAttachPin(LED_button, PWM1_Ch);
  delay(300);
  // проверка заряда акума, если разряжен то прекратить работу
  voltage = readVcc();  //выводим напряжение в вольтах
  bat_vol_old = voltage;
  PWM_old = (float)volts / voltage * 1024;
  //Serial.println(voltage);
  if (voltage < 3.75) {
    led_blink(1);
  }
  if (voltage < 3.5) {
    led_blink(2);
  }
  if (voltage < 3.25) {
    led_blink(3);
  }
  if (voltage < battery_low) {
    flag = 0;
    digitalWrite(mosfet, LOW);  // принудительно отключить койл
    led_blink(4);
  } else {
    flag = 1;
  }
  // ----инициализация сторожевого таймера-----
  //  esp_task_wdt_init(WDT_TIMEOUT, true);
  //  esp_task_wdt_add(NULL);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
  //  // ----инициализация сторожевого таймера-----4 if(http_flag){
  wake_up();
}

void loop() {
  //esp_task_wdt_reset(); // сброс сторожевого таймера
  if (millis() - last_time > 50) {  // 20 раз в секунду измеряем напряжение
    last_time = millis();
    voltage = readVcc();                                             // фильтруем
    bat_volt_f = filter_k * voltage + (1 - filter_k) * bat_vol_old;  //фильтруем
    bat_vol_old = bat_volt_f;
    // Serial.println(bat_volt_f);
    if (bat_volt_f < battery_low) {  // если напряжение меньше минимального
      flag = 0;                      // прекратить работу
      digitalWrite(mosfet, LOW);     // принудительно отключить койл
      ledcWrite(pwmChannel, 0);
      led_blink(4);
    } else {
      flag = 1;
    }
  }
  //-----------мигание кнопоки-----------
  if (millis() - button_led_time > 50 && !stop_led && !server_flag) {  // 20 раз в секунду измеряем напряжение
    button_led_time = millis();
    if (led) {
      led_con += blink_led;
      if (led_con >= max_led) {
        led = false;
        led_con = max_led;
      }
    } else {
      led_con -= blink_led;
      if (led_con <= 0) {
        led = true;
        led_con = 0;
      }
    }
    ledcWrite(PWM1_Ch, led_con);
  }
  if (millis() - button_led_time > 50 && server_flag) {  // 20 раз в секунду измеряем напряжение
    button_led_time = millis();
    if (led) {
      led_con += blink_led * 2;
      if (led_con >= max_led) {
        led = false;
        led_con = max_led;
      }
    } else {
      led_con -= blink_led * 2;
      if (led_con <= 0) {
        led = true;
        led_con = 0;
      }
    }
    ledcWrite(PWM1_Ch, led_con);
  }
  //-----------опрос кнопоки-----------
  button_state = !digitalRead(button);  // если нажата любая кнопка, "продлить" таймер ухода в сон
  if (button_state) wake_timer = millis();

  //---------отработка нажатия кнопки парения-----------
  if (button_state && flag && !wake_up_flag) {
    if (!vape_flag) {
      vape_flag = 1;
      vape_mode = 1;  // первичное нажатие
      delay(20);
      vape_press = millis();  // первичное нажатие
      but_pres_1 = true;
      but_pres_2 = true;
    }

    // if (vape_release_count == 1) {
    //   vape_mode = 2;               // двойное нажатие
    //  // server_start();
    //   delay(70);
    // }
    // if (vape_release_count == 2) {
    //   vape_mode = 3;
    // }

    if (millis() - vape_press > vape_threshold * 1000 /*&& but_pres_1*/) {  // "таймер затяжки"
      vape_mode = 0;
      ledcWrite(pwmChannel, 0);
      stop_led = false;
      but_pres_1 = false;
    }
    if (millis() - vape_press > time_http /*&& but_pres_2*/) {  // "таймер включения сервера
      //Serial.print("Server start");
      //Serial.println(vape_press);
      but_pres_2 = false;
      if (!server_flag)
        server_start();
    }
    if (vape_mode == 1) {
      stop_led = true;
      ledcWrite(PWM1_Ch, 0);                   // обычный режим парения
      PWM = (float)volts / bat_volt_f * 1024;  // считаем значение для ШИМ сигнала
      if (PWM > 1023) PWM = 1023;              // ограничил PWM "по тупому", потому что constrain сука не работает!
      PWM_f = PWM_filter_k * PWM + (1 - PWM_filter_k) * PWM_old;
      PWM_old = PWM_f;
      ledcWrite(pwmChannel, PWM_f);
    }
    // if (vape_mode == 3) {                                           // тройное нажатие
    //   vape_release_count = 0;
    //   vape_mode = 1;
    //   vape_flag = 0;
    //  // esp_deep_sleep_start();     // вызвать функцию сна
    //  //server_start();
    // }
    vape_btt = 1;
  }
  if (!button_state && vape_btt) {  // если кнопка ПАРИТЬ отпущена
    if (millis() - vape_press > 180) {
      vape_release_count = 0;
      vape_mode = 0;
      vape_flag = 0;
    }
    ledcWrite(pwmChannel, 0);
    ledcWrite(PWM1_Ch, led_con);
    stop_led = false;
    but_pres_2 = false;
    but_pres_1 = false;
    vape_btt = 0;
    if (vape_mode == 1) {
      vape_release_count = 1;
      //vape_press = millis();
    }
    // if (vape_mode == 2) vape_release_count = 2;
  }
  if (!button_state) {
    vape_press = millis();
    // Serial.print("millis - ");
    // Serial.print(millis());
    // Serial.print(" vape_press - ");
    // Serial.println(vape_press);
  }
  //     //---------отработка нажатия кнопки парения-----------

  if (wake_up_flag) wake_puzzle();  // вызвать функцию 5 нажатий для пробудки

  if ((millis() - wake_timer > sleep_timer * 1000) && !server_flag) {  // если кнопки не нажимались дольше чем sleep_timer секунд
    esp_deep_sleep_start();
  }
  if ((millis() - wake_timer > 50 * 1000) && server_flag) {  // если кнопки не нажимались дольше чем sleep_timer секунд
    server.end();
    esp_deep_sleep_start();
  }
  if (server_flag) {
    if (WiFi.softAPgetStationNum() > 0) {
      led_con += blink_led * 2;
    }
  }
  //Serial.println(WiFi.softAPgetStationNum());
  //Serial.println(vape_press);
}  // конец loop

//------функция, вызываемая при выходе из сна (прерывание)------
void wake_up() {
  ledcWrite(PWM1_Ch, max_led);  // включить кнопку
  ledcWrite(pwmChannel, 0);     // принудительно отключить койл
  wake_timer = millis();        // запомнить время пробуждения
  wake_up_flag = 1;
  vape_release_count = 0;
  vape_mode = 0;
  vape_flag = 0;
  mode_flag = 1;
}
// //------функция, вызываемая при выходе из сна (прерывание)------

// //------функция 5 нажатий для полного пробуждения------
void wake_puzzle() {
  vape_btt_f = 0;
  boolean wake_status = 0;
  byte click_count = 0;
  while (1) {
    button_state = !digitalRead(button);

    if (button_state && !vape_btt_f) {
      vape_btt_f = 1;
      click_count++;
      switch (click_count) {
        case 1:
          break;
        case 2:
          break;
        case 3:
          break;
        case 4:
          break;
      }
      if (click_count > 4) {  // если 5 нажатий сделаны за 3 секунды
        wake_status = 1;      // флаг "проснуться"
        break;
      }
    }
    if (!button_state && vape_btt_f) {
      vape_btt_f = 0;
      ledcWrite(PWM1_Ch, 0);
      delay(70);
      ledcWrite(PWM1_Ch, max_led);
    }
    if (millis() - wake_timer > 5000) {  // если 5 нажатий не сделаны за 3 секунды
      wake_status = 0;                   // флаг "спать"
      break;
    }
  }
  if (wake_status)
    wake_up_flag = 0;
  else
    esp_deep_sleep_start();
}
// //----------режим теста кнопок----------

void led_blink(int con) {
  for (int i = 0; i <= con; i++) {
    ledcWrite(PWM1_Ch, 0);
    delay(300);
    ledcWrite(PWM1_Ch, max_led);
    delay(300);
  }
}
void set_param() {
  frequency = data.frequency;
  sleep_timer = data.sleep_timer;        // таймер сна в секундах
  vape_threshold = data.vape_threshold;  // отсечка затяжки, в секундах
  battery_low = data.battery_low;
  watts = data.watts;  // храним вольты и ватты
  ohms = data.ohms;
  volts = sqrt(watts * ohms);  // храним омы
  Serial.println("frequency " + (String)frequency);
  Serial.println("sleep timer " + (String)sleep_timer);
  Serial.println("vape threshold " + (String)vape_threshold);
  Serial.println("battery low " + (String)battery_low);
  Serial.println("watts " + (String)watts);
  Serial.println("ohms " + (String)ohms);
  Serial.println("volts " + (String)volts);
}
float readVcc() {
  return (float)((analogRead(bat_vol_pin) * VREF) / 4094) * 3.5;  // расчёт реального VCC
}
