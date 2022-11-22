// String html_page_1 = R"LIT1(<!DOCTYPE HTML><html><head><title>Vape Settings</title></head>
// <body><div style="text-align: center; margin-top: 1px; margin-bottom: 10px; font-size: 29px;">Voltage -)LIT1";
// String html_page_2 = R"LIT2("</div>
// <div style="text-align: center; margin-bottom: 10px;">
//         <form action="/get">
//             <input type="submit" value="Reboot" name="Reboot">
//            </form></div>
//     <form action="/get">
//       <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;">input1: <input type="text" name="input1">
//       <input type="submit" value="Submit"></p>
//     </form><br>
//     <form action="/get">
//       <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;">input2: <input type="text" name="input2">
//       <input type="submit" value="Submit"></p>
//     </form><br>
//     <form action="/get">
//      <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;">input3: <input type="text" name="input3">
//       <input type="submit" value="Submit"></p> 
//     </form>
//   </body></html>)LIT2";

char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Vape Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    input1: <input type="text" name="input1">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input2: <input type="text" name="input2">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input3: <input type="text" name="input3">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

char buf_html[2200];

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void server_start() {
  // Send web page with input fields to client
    // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  //IPAddress local_IP(192, 168, 1, 184);
// Задаем IP-адрес сетевого шлюза:
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8); // optional
  IPAddress secondaryDNS(8, 8, 4, 4);

// Configures static IP address
// if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//   Serial.println("STA Failed to configure");
// } 
 //WiFi.begin(ssid, password);
 //  WiFi.mode(WIFI_STA);     // Режим клиента
//  WiFi.mode(WIFI_AP);      // Режим точки доступа
//  WiFi.mode(WIFI_AP_STA);  // Включены оба режима
String html_page = R"LIT1(<!DOCTYPE HTML><html><head><title>Vape Settings</title></head>
<body><div style="text-align: center; margin-top: 1px; margin-bottom: 10px; font-size: 29px;">Voltage -)LIT1" +(String)voltage+ R"LIT2(</div>
<div style="text-align: center; margin-bottom: 10px;">
        <form action="/get">
            <input type="submit" value="Reboot" name="Reboot">
           </form></div>
    <form action="/get">
      <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;">Frequency: <input style="width: 70px;" 
      type="text" name="frequency" value=")LIT2"+(String)frequency+ R"LIT3(")>
      <input type="submit" value="Submit"></p>
    </form><br>
    <form action="/get">
     <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;"> Sleep timer: <input style="width: 70px;"
      type="text" name="sleep_timer" value=")LIT3"+(String)sleep_timer+R"LIT4(")>
      <input type="submit"  value="Submit"></p>
    </form><br>
    <form action="/get">
     <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;"> Vape threshold: <input style="width: 70px;"
      type="text" name="vape_threshold" value=")LIT4"+(String)vape_threshold+R"LIT5(")>
      <input type="submit"  value="Submit"></p>
    </form><br>
    <form action="/get">
     <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;"> Battery Low: <input style="width: 70px;"
      type="text" name="battery_low" value=")LIT5"+(String)battery_low+R"LIT6(")>
      <input type="submit"  value="Submit"></p>
    </form><br>
    <form action="/get">
     <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;"> Watts: <input style="width: 70px;"
      type="text" name="watts" value=")LIT6"+(String)watts+R"LIT7(")>
      <input type="submit"  value="Submit"></p>
    </form><br>
    <form action="/get">
     <p style="text-align: center; margin-top: 1px; margin-bottom: 1px;"> Ohms: <input style="width: 70px;"
      type="text" name="ohms" value=")LIT7"+(String)ohms+R"LIT8(")>
      <input type="submit"  value="Submit"></p>
    </form><br>
  </body></html>)LIT8";
  html_page.toCharArray(buf_html, 2200);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(gateway, gateway, subnet);
  WiFi.softAP(ssid, password);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", buf_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(frequency_input)) {
      inputMessage = request->getParam(frequency_input)->value();
      inputParam = frequency_input;
      data.frequency = inputMessage.toInt();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
      //html_page.toCharArray(buf_html, 800);
    }
    else if (request->hasParam(sleep_timer_input)) {
      inputMessage = request->getParam(sleep_timer_input)->value();
      inputParam = sleep_timer_input;
      data.sleep_timer = inputMessage.toInt();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(vape_threshold_input)) {
      inputMessage = request->getParam(vape_threshold_input)->value();
      inputParam = vape_threshold_input;
      data.vape_threshold = inputMessage.toInt();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
    }
    else if (request->hasParam(battery_low_input)) {
      inputMessage = request->getParam(battery_low_input)->value();
      inputParam = battery_low_input;
      data.battery_low = inputMessage.toFloat();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
    }
    else if (request->hasParam(watts_input)) {
      inputMessage = request->getParam(watts_input)->value();
      inputParam = watts_input;
      data.watts = inputMessage.toInt();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
    }
    else if (request->hasParam(ohms_input)) {
      inputMessage = request->getParam(ohms_input)->value();
      inputParam = ohms_input;
      data.ohms = inputMessage.toFloat();
      set_param();
      EEPROM.put(0, data); //запись в eeprom
      EEPROM.commit(); //применение записи 
    }
    else if (request->hasParam(Reboot)) {
      ESP.restart();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
  server_flag=true;
}