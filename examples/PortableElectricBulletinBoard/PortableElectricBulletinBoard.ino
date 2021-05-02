#include <ROG_32x16_DotMatrixLEDPanel.h>
#include <ESP32_SD_EasyWebSocket.h>
#include "kaomoji_64x16_01.c"
#include "kaomoji_64x16_01_Csource.c"
#include "kaomoji_64x16_02_mabataki_01.c"
#include "kaomoji_64x16_02_mabataki_02.c"
#include "arrow_animation.c"


/*** ROG_32x16_DotMatrixLEDPanel *************************/
/* pin assign ->      [ESP32] [PANEL]*/
const uint8_t SE_PIN  =  5;  // 1p
const uint8_t ABB_PIN = 18;  // 2p
const uint8_t A3_PIN  = 27;  // 3p
const uint8_t A2_PIN  = 19;  // 4p
const uint8_t A1_PIN  = 26;  // 5p
const uint8_t A0_PIN  = 21;  // 6p
/*      Controller  Vss         7p */
const uint8_t DG_PIN  = 25;  // 8p
const uint8_t CLK_PIN = 33;  // 9p
const uint8_t WE_PIN  = 32;  // 10p
const uint8_t DR_PIN  = 22;  // 11p
const uint8_t ALE_PIN = 23;  // 12p

const uint8_t num_panel = 2;  // number of panel
const uint16_t led_width = 32 * num_panel;
const uint8_t led_height = 16;

/* Limit number of characters to display */
const uint8_t MAX_WORD = 200;  // Max word count (shoud to set 4 or more)

ROG_32x16_DotMatrixLEDPanel matrix(
  num_panel,
  SE_PIN,
  ABB_PIN,
  A3_PIN,
  A2_PIN,
  A1_PIN,
  A0_PIN,
  DG_PIN,
  CLK_PIN,
  WE_PIN,
  DR_PIN,
  ALE_PIN
);
/*********************************************************/

/*** MicroSD card ****************************************/
/* Usage on ESP32 Breakout board with MicroSD Socket */
const uint8_t SD_CS   = 13;  // CS(SS)
const uint8_t SD_CLK  = 14;
const uint8_t SD_MISO =  2;
const uint8_t SD_MOSI = 15;

const uint32_t sd_freq = 80000000;

/* Function of initialization */
void SD_init()
{
  Serial.print("Initializing SD card...");

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, sd_freq)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}
/*********************************************************/

/***　SD EasyWebSocket ************************************************************************/
const char *ssid = "ESP32_ROG";     // SoftAP mode SSID
const char *password = "12345678";  // SoftAP mode Password (Required 8 characters or more)

const char* HTM_head_file1 = "/EWS/LIP2hed1.txt"; // HTMLヘッダファイル1
const char* HTM_head_file2 = "/EWS/LIP2hed2.txt"; // HTMLヘッダファイル2
const char* HTML_body_file = "/EWS/dummy.txt"; // HTML body要素ファイル（ここではダミーファイルとしておく）
const char* dummy_file =     "/EWS/dummy.txt"; // HTMLファイル連結のためのダミーファイル

SD_EasyWebSocket ews;

IPAddress LIP; // ローカルIPアドレス自動取得用

//------Easy WebSpclet関連　引数初期化----------------
String ret_str;           //ブラウザから送られてくる文字列格納用
int PingSendTime = 10000; //ESP32からブラウザへPing送信する間隔(ms)
bool get_http_req_status = false; //ブラウザからGETリクエストがあったかどうかの判定変数
uint8_t WS_Status = 0;
/*********************************************************************************************/

/*** Shinonome Font **************************************************************************/
ESP32_SD_ShinonomeFNT SFR(SD_CS, sd_freq);

const char* UTF8SJIS_file = "/font/Utf8Sjis.tbl";  // UTF8 Shift_JIS 変換テーブルファイル名を記載しておく
const char* Shino_Zen_Font_file = "/font/shnmk16.bdf";  // 全角フォントファイル名を定義
const char* Shino_Half_Font_file = "/font/shnm8x16.bdf";  // 半角フォントファイル名を定義
/*********************************************************************************************/

/*** Switches ********************************************************************************/
/* Switches pin assign */
const int8_t SW_1 = 39;
const int8_t SW_2 = 34;
const int8_t SW_3 = 35;
const int8_t SW_4 = 36;  // slide switch

volatile uint8_t sw1_Read = 0x00;
volatile uint8_t sw2_Read = 0x00;
volatile uint8_t sw3_Read = 0x00;
volatile uint8_t sw4_Read = 0x00;
volatile boolean sw1_Pressed = 0;
volatile boolean sw2_Pressed = 0;
volatile boolean sw3_Pressed = 0;
volatile boolean sw4_Changed = 0;

/* Hardware Timer */
/* ref: https://garretlab.web.fc2.com/arduino/esp32/examples/ESP32/Timer_RepeatTimer.html */
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  /* Increment the counter and set the time of ISR */
  portENTER_CRITICAL_ISR(&timerMux);

  /* Read the status of switches by use N times matching method */
  /* SW_1 */
  sw1_Read = (0xfe & (sw1_Read << 1)) | (0x01 & (uint8_t)digitalRead(SW_1));
  if (sw1_Read == 0x7f) sw1_Pressed = 1; // 0x7f=0111_1111 : N=7
  /* SW_2 */
  sw2_Read = (0xfe & (sw2_Read << 1)) | (0x01 & (uint8_t)digitalRead(SW_2));
  if (sw2_Read == 0x7f) sw2_Pressed = 1; // 0x7f=0111_1111 : N=7
  /* SW_3 */
  sw3_Read = (0xfe & (sw3_Read << 1)) | (0x01 & (uint8_t)digitalRead(SW_3));
  if (sw3_Read == 0x7f) sw3_Pressed = 1; // 0x7f=0111_1111 : N=7
  /* SW_4 */
  sw4_Read = (0xfe & (sw4_Read << 1)) | (0x01 & (uint8_t)digitalRead(SW_4));
  if (sw4_Read == 0x7f || sw4_Read == 0xfe) sw4_Changed = 1;

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
/*********************************************************************************************/

/*** Application *****************************************************************************/
bool scl_pause = false;       // scroll pause flag
bool disp = true;             // display flag
bool play_all_demo = false;   // play all demo flag
boolean state = 1;      // 0: Setting  1: Display
uint8_t demo_mode = 1;  // demo number
uint8_t demo_num = 6;   // number of demo
uint16_t tWidth = 0;        // text width
int16_t tX0 = led_width;    // the origin of X axis of text
uint16_t scrollDelay = 20;  // default : 20
String displayText = "それぞれ[r赤][g緑][o橙]デフォルトカラーで表示されます。";  // 23 characters
int16_t iX0 = -ARROW_ANIMATION_WIDTH;  // the origin of X axix of image
uint16_t dot_color = DOT_RED;  // color
/*********************************************************************************************/





void setup() {
  /* It is not necessary execut Serial.begin() in twice or more. Execute in ESP32_SD_EasyWebSocket.cpp */
  /* ref: https://teratail.com/questions/326026 */
//  Serial.begin(115200);
//  while (!Serial);

  /*** MicroSD card ***/
  SD_init();
  /********************/

  /*** SD EasyWebSocket *****************************************/
  ews.SoftAP_setup(ssid, password); //Soft AP mode

  delay(1000);

  LIP = WiFi.softAPIP(); //ESP32のSoft AP mode IPアドレスを自動取得

  delay(1000);

  /* Wi-Fi off for decrease power consumption (~80mA) */
  WiFi.mode(WIFI_OFF);  
  /**************************************************************/


  /*** ROG_32x16_DotMatrixLEDPanel ************************************************************/
  matrix.begin();
  Serial.println("Initialized LED panel successfully.");

  matrix.ShinonomeFNT_SD_init(SD_CLK, SD_MISO, SD_MOSI, SD_CS, sd_freq,
                              UTF8SJIS_file, Shino_Zen_Font_file, Shino_Half_Font_file, &SFR);
  matrix.shnmFntBegin(MAX_WORD);  // max number of characters
  matrix.setTextColor(DOT_RED, DOT_BLACK);  // text color, background color
  /********************************************************************************************/

  /*** Switches *******************************************************************************/
  pinMode(SW_1, INPUT);
  pinMode(SW_2, INPUT);
  pinMode(SW_3, INPUT);
  pinMode(SW_4, INPUT);

  /* ref: https://garretlab.web.fc2.com/arduino/esp32/examples/ESP32/Timer_RepeatTimer.html */
  // Hardware Timer
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 20, true);

  // Start an alarm
  timerAlarmEnable(timer);
  /********************************************************************************************/

  delay(1000);

  matrix.shnmFntScrollWithColor("[gSSID: ][r" + (String)ssid + "][g  PASS:][r" + (String)password + "][g  IPaddress:" + LIP.toString() + "]", 10 * 2);

  /* for default display */
//  tWidth = matrix.shnmFntStrConv(displayText);
  tWidth = matrix.shnmFntStrConvWithColor(displayText);
}

void loop() {
  
  WebSocket_handshake(); // WebSocket ハンドシェイクでブラウザにHTML送信
  WebSocket_txt_receive(); // ブラウザからのテキスト受信
  
  /* change mode */
  if (sw1_Pressed) {
    demo_mode++;
    if (demo_mode > demo_num) demo_mode = 1;
    Serial.print("mode changed -> ");
    Serial.println(demo_mode);

    matrix.fillScreen(DOT_BLACK);

    sw1_Pressed = 0;
  }

  if (sw2_Pressed) {
    /* 1. Scroll Pause or Play */
    scl_pause = !scl_pause;
    
    /* 2. Scroll speed down */
//    if (demo_mode == 1) {
//      if (scrollDelay + 5 <= 200) {
//        scrollDelay += 5;
//      }
//    }

    sw2_Pressed = 0;
  }
  
  if (sw3_Pressed) {
    /* 1. display ON or OFF */
    disp = !disp;
    
    if (disp == false) {
      scl_pause = true;
      matrix.fillScreen(DOT_BLACK);
    } else if (disp == true) {
      scl_pause = false;
    }
    
    /* 2. Scroll speed up */
//    if (demo_mode == 1) {
//      if (scrollDelay - 5 >= 0) {
//        scrollDelay -= 5;
//      }
//    }

    sw3_Pressed = 0;
  }

  /* change status */
  if (sw4_Changed) {
    state = !state;
    Serial.print("state changed -> ");
    Serial.println(state);
    if (state) {
      Serial.println(" -> Display");
    } else {
      Serial.println(" -> Setting");
    }

    matrix.fillScreen(DOT_BLACK);

    sw4_Changed = 0;
  }



  if (state == 0) {
    WiFi.mode(WIFI_AP);  // SoftAP ON

    matrix.setTextColor(DOT_RED, DOT_BLACK);
    matrix.setCursor(0, 0);
    matrix.shnmFntPrint(" 設定中 ");
  } else if (state == 1) {
    WiFi.mode(WIFI_OFF);  // SoftAP OFF
    
    switch (demo_mode) {
      case 1:  // text scrolling
        if (disp == true) {
          if (scl_pause == false) {
            if (tX0 >= -(tWidth + led_width)) {
              matrix.startWrite();
              delay(scrollDelay);
              matrix.setCursor(tX0 + led_width, 0);
              matrix.shnmFntWriteWithColor();
              matrix.endWrite();
  
              tX0--;
            } else {
              tX0 = led_width;
              
              if (play_all_demo) {
                demo_mode++;
            
                if (demo_mode > demo_num) demo_mode = 1;
                Serial.print("mode changed -> ");
                Serial.println(demo_mode);
              }
            }
          }
        }
        break;

      case 2:  // draw image with color
        if (disp == true) {
          matrix.fillScreen(DOT_BLACK);
          matrix.drawGIMP_CSourceImageDump(0,
                                           0,
                                           KAOMOJI_64X16_01_pixel_data,
                                           KAOMOJI_64X16_01_WIDTH,
                                           KAOMOJI_64X16_01_HEIGHT
                                          );
          delay(1000);
          
          if (play_all_demo) {
            demo_mode++;
            
            if (demo_mode > demo_num) demo_mode = 1;
            Serial.print("mode changed -> ");
            Serial.println(demo_mode);
          }
        }
        break;

      case 3:  // draw image
        if (disp == true) {
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_01_bits, 64, 16, DOT_RED);
          delay(1000);
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_01_bits, 64, 16, DOT_GREEN);
          delay(1000);
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_01_bits, 64, 16, DOT_ORANGE);
          delay(1000);
          
          if (play_all_demo) {
            demo_mode++;
            
            if (demo_mode > demo_num) demo_mode = 1;
            Serial.print("mode changed -> ");
            Serial.println(demo_mode);
          }
        }
        break;

      case 4:  // kaomoji animation
        if (disp == true) {
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_01_bits, 64, 16, DOT_RED);
          delay(random(150, 2500));
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_02_bits, 64, 16, DOT_RED);
          delay(100);
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_01_bits, 64, 16, DOT_RED);
          delay(random(150, 2500));
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_02_bits, 64, 16, DOT_RED);
          delay(100);
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_01_bits, 64, 16, DOT_RED);
          delay(random(150, 2500));
          matrix.fillScreen(DOT_BLACK);
          matrix.drawXBitmap(0, 0, kaomoji_64x16_02_mabataki_02_bits, 64, 16, DOT_RED);
          delay(100);

          if (play_all_demo) {
            delay(500);
            
            demo_mode++;
            
            if (demo_mode > demo_num) demo_mode = 1;
            Serial.print("mode changed -> ");
            Serial.println(demo_mode);
          }
        }
        break;

      case 5:  // traffic signal
        if (disp == true) {
          matrix.fillScreen(DOT_BLACK);
          matrix.fillCircle(7, 7, 7, DOT_GREEN);
          delay(5000);
  
          matrix.fillScreen(DOT_BLACK);
          matrix.fillCircle(16 + 7, 7, 7, DOT_ORANGE);
          delay(3000);
  
          matrix.fillScreen(DOT_BLACK);
          matrix.fillCircle(16 * 2 + 7, 7, 7, DOT_RED);
          delay(5000);
          
          matrix.fillScreen(DOT_BLACK);
          matrix.fillCircle(7, 7, 7, DOT_GREEN);
          delay(5000);

          matrix.fillScreen(DOT_BLACK);
          
          if (play_all_demo) {
            demo_mode++;
            
            if (demo_mode > demo_num) demo_mode = 1;
            Serial.print("mode changed -> ");
            Serial.println(demo_mode);
          }
        }
        break;
        
        case 6: // arrow animation
          if (disp == true) {
            if (scl_pause == false) {
              iX0++;
              matrix.fillScreen(DOT_BLACK);
              matrix.drawGIMP_CSourceImageDump(iX0,  // x axis
                                               4,  // y axis
                                               ARROW_ANIMATION_pixel_data,
                                               ARROW_ANIMATION_WIDTH,
                                               ARROW_ANIMATION_HEIGHT
                                              );
              delay(10*5);
            
              iX0++;
              if(iX0 > led_width + ARROW_ANIMATION_WIDTH) iX0 = -ARROW_ANIMATION_WIDTH;

              if (play_all_demo) {
                demo_mode++;
                
                if (demo_mode > demo_num) demo_mode = 1;
                Serial.print("mode changed -> ");
                Serial.println(demo_mode);
              }
            }
          }
          break;
          
//      case 7:  // fill screen
//          /* ### Caution ###                                             */
//          /* Please connect power supply with sufficient output capacity */
//          matrix.fillScreen(DOT_RED);     // 2 panels : Current = 1.56A
//          delay(3000);
//          matrix.fillScreen(DOT_BLACK);
//          matrix.fillScreen(DOT_GREEN);   // 2 panels : Current = 1.89A
//          delay(3000);
//          matrix.fillScreen(DOT_BLACK);
//          matrix.fillScreen(DOT_ORANGE);  // 2 panels : Current = 3.10A
//          delay(3000);
//          matrix.fillScreen(DOT_BLACK);
//          delay(3000);
//
//          if (play_all_demo) {
//            demo_mode++;
//            
//            if (demo_mode > demo_num) demo_mode = 1;
//            Serial.print("mode changed -> ");
//            Serial.println(demo_mode);
//          }
//        break;

//        case 8: // Lchika animation
//        /* ### Caution ###                                             */
//        /* Please connect power supply with sufficient output capacity */
//          if (disp == true) {
//            for(int y=0; y<led_height; y+=2) {
//              for(int x=0; x<led_width; x++) {
//                matrix.drawPixel(x, y, DOT_BLACK);
//                matrix.drawPixel(x, y, dot_color);
//                delay(3);
//              }
//              
//              for(int x=led_width; x>=0; x--) {
//                matrix.drawPixel(x, y+1, DOT_BLACK);
//                matrix.drawPixel(x, y+1, dot_color);
//                delay(3);
//              }
//            }
//          
//            if(dot_color == DOT_RED) {
//              dot_color = DOT_GREEN;
//            } else if (dot_color == DOT_GREEN) {
//              dot_color = DOT_ORANGE;
//            } else if (dot_color == DOT_ORANGE) {
//              dot_color = DOT_BLACK;
//            } else if (dot_color == DOT_BLACK) {
//              dot_color = DOT_RED;
//            }
//
//            if (play_all_demo) {
//              demo_mode++;
//              
//              if (demo_mode > demo_num) demo_mode = 1;
//              Serial.print("mode changed -> ");
//              Serial.println(demo_mode);
//            }
//          }
//          break;
        
    }
  }
}





/*** SD EasyWebSocket ************************************************************************/
void WebSocket_handshake() {
  if (ews.Get_Http_Req_Status()) { //ブラウザからGETリクエストがあったかどうかの判定
    String html_str1 = "", html_str2 = "", html_str3 = "", html_str4 = "", html_str5 = "", html_str6 = "", html_str7 = "";
    //※String変数一つにEWS_Canvas_Slider_T関数は２つまでしか入らない
    html_str1 += "<body style='background:#000; color:#fff;'>\r\n";
    html_str1 += "<font size=3>\r\n";
//    html_str1 += "ESP-WROOM-32(ESP32)\r\n";
//    html_str1 += "<br>\r\n";
    html_str1 += "SD_EasyWebSocket Beta1.60 Sample\r\n";
    html_str1 += "</font><br>\r\n";
    html_str1 += ews.EWS_BrowserSendRate();
    html_str1 += "<br>\r\n";
    html_str1 += ews.EWS_Status_Text2("WebSocket Status", "#555", 20, "#FF00FF");
    html_str1 += "<br><br>\r\n";
    html_str2 += ews.EWS_TextBox_Send("txt", "[r赤][o橙][g緑]で表示", "送信");
    html_str2 += "<br><br>\r\n";
    html_str2 += "Scroll\r\n";
    html_str2 += ews.EWS_On_Momentary_Button("Move", "Start", 80, 25, 15, "#000000", "#AAAAAA");
    html_str2 += ews.EWS_On_Momentary_Button("Pause", "Pause", 80, 25, 15, "#FFFFFF", "#555555");
    html_str2 += "<br><br>\r\n";
    html_str3 += "Default Font Color\r\n";
    html_str3 += "<br>\r\n";
    html_str3 += ews.EWS_On_Momentary_Button("Red", "Red", 80, 25, 15, "#000000", "#ff0000");
    html_str3 += ews.EWS_On_Momentary_Button("Orange", "Orange", 80, 25, 15, "#000000", "#FFa500");
    html_str3 += ews.EWS_On_Momentary_Button("Green", "Green", 80, 25, 15, "#000000", "#00FF00");
    html_str3 += "<br><br>\r\n";
    html_str3 += "Play All Demo";
    html_str3 += "<br>\r\n";
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode0", "ON/OFF", 100, 25, 15, "#000000", "#ff0000");
    html_str3 += "<br><br>\r\n";
    html_str3 += "Demo Mode";
    html_str3 += "<br>\r\n";
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode1", "Mode1", 80, 25, 15, "#000000", "#ff0000");
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode2", "Mode2", 80, 25, 15, "#000000", "#FFa500");
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode3", "Mode3", 80, 25, 15, "#000000", "#00FF00");
    html_str3 += "<br>\r\n";
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode4", "Mode4", 80, 25, 15, "#000000", "#ff0000");
    html_str3 += ews.EWS_On_Momentary_Button("DemoMode5", "Mode5", 80, 25, 15, "#000000", "#FFa500");
//    html_str3 += ews.EWS_On_Momentary_Button("DemoMode6", "Mode6", 80, 25, 15, "#000000", "#00FF00");
    html_str3 += "<br><br>\r\n";
    html_str4 += "Scroll Delay　\r\n";
    html_str4 += "<br>\r\n";
    html_str4 += ews.EWS_Canvas_Slider_T("Speed", 200, 30, "#777777", "#CCCCFF"); //CanvasスライダーはString文字列に２つまでしか入らない
    html_str5 += "<br><br><br><br>\r\n";
    html_str5 += ews.EWS_WebSocket_Reconnection_Button2("WS-Reconnect", "grey", 200, 40, "black" , 17);
    html_str5 += "<br><br>\r\n";
    html_str5 += ews.EWS_Close_Button2("WS CLOSE", "#bbb", 150, 40, "red", 17);
    html_str5 += ews.EWS_Window_ReLoad_Button2("ReLoad", "#bbb", 150, 40, "blue", 17);
    html_str5 += "</body></html>";
    //WebSocket ハンドシェイク関数
    ews.EWS_HandShake_main(3, SD_CS, HTM_head_file1, HTM_head_file2, HTML_body_file, dummy_file, LIP, html_str1, html_str2, html_str3, html_str4, html_str5, html_str6, html_str7);
  }
}

void WebSocket_txt_receive() {
  ret_str = ews.EWS_ESP32CharReceive(PingSendTime);
  if (ret_str != "_close") {
    if (ret_str != "\0") {
      Serial.println(ret_str);
      if (ret_str != "Ping") {
        if (ret_str[0] != 't') {
          int ws_data = (ret_str[0] - 0x30) * 100 + (ret_str[1] - 0x30) * 10 + (ret_str[2] - 0x30);

          switch (ret_str[4]) {
            case 'M': //スクロールスタート
              scl_pause = false;
              break;
              
            case 'P': //スクロール一時停止
              scl_pause = true;
              break;

            case 'R':
              matrix.setTextColor(DOT_RED, DOT_BLACK);
              tWidth = matrix.shnmFntStrConvWithColor(displayText);
              break;
              
            case 'G':
              matrix.setTextColor(DOT_GREEN, DOT_BLACK);
              tWidth = matrix.shnmFntStrConvWithColor(displayText);
              break;
              
            case 'O':
              matrix.setTextColor(DOT_ORANGE, DOT_BLACK);
              tWidth = matrix.shnmFntStrConvWithColor(displayText);
              break;
              
            case 'D':
              switch (ret_str[12]) {
                case '0':
                  play_all_demo = !play_all_demo;
                  break;

                case '1':
                  demo_mode = 1;
                  break;

                case '2':
                  demo_mode = 2;
                  break;

                case '3':
                  demo_mode = 3;
                  break;
                  
                case '4':
                  demo_mode = 4;
                  break;
                  
                case '5':
                  demo_mode = 5;
                  break;
                  
//                case '6':
//                  demo_mode = 6;
//                  break;
              }
              break;

            case 'S':
              scrollDelay = ws_data;
              break;
          }
        } else if (ret_str[0] == 't') {
          /* receive text */
          displayText = ret_str.substring(ret_str.indexOf('|') + 1, ret_str.length() - 1);
          Serial.print("Receive txt => "); Serial.println(displayText);
          
          matrix.fillScreen(DOT_BLACK);
          tWidth = matrix.shnmFntStrConvWithColor(displayText);
        }
        ret_str = "";
      }
    }
  } else if (ret_str == "_close") {
    Serial.println("---------------WebSocket Close");
    ret_str = "";
  }
}
/*********************************************************************************************/
