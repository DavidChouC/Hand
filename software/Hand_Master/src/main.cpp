#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "XboxSeriesXControllerESP32_asukiaaa.hpp"

/* CAN引脚定义 */
#define CAN_TX_PIN    5
#define CAN_RX_PIN    4

/* CAN消息ID */
typedef enum {
    CAN_MSG_ID_BROADCAST = 0x200,   /* 广播ID */
    CAN_MSG_ID_FINGER1 = 0x201,     /* 手指1 ID */
    CAN_MSG_ID_FINGER2 = 0x202,     /* 手指2 ID */
    CAN_MSG_ID_FINGER3 = 0x203,     /* 手指3 ID */
    CAN_MSG_ID_FINGER4 = 0x204,     /* 手指4 ID */
    CAN_MSG_ID_FINGER5 = 0x205,     /* 手指5 ID */
    CAN_MSG_ID_REBOOT = 0x700       /* 重启ID */
} Can_Msg_Id_t;

/* 手指动作类型 */
typedef enum {
    FINGER_OK,
    FINGER_ROCK,
    FINGER_POINT
} Finger_Action_t;

/* 每个手指的滑块值结构 */
typedef struct {
    float slider1;
    float slider2;
    float slider3;
} FingerSliders;

/* WiFi AP配置 */
const char* AP_SSID = "Hand_Control";
const char* AP_PASSWORD = "12345678";

/* WiFi AP配置 */
const char* STA_SSID = "nszndmwbp";
const char* STA_PASSWORD = "12345677";

/* 存储每个手指的滑块值 */
FingerSliders fingerValues[5] = {
    {0, 0, 0},  // 拇指
    {0, 0, 0},  // 食指
    {0, 0, 0},  // 中指
    {0, 0, 0},  // 无名指
    {0, 0, 0}   // 小指
};

/* WebSocket服务器 */
WebSocketsServer webSocket = WebSocketsServer(81);
/* Web服务器 */
AsyncWebServer server(80);

/* 手指ID到索引的映射 */
struct FingerMapping {
    const char* name;
    int index;
    Can_Msg_Id_t canId;
} fingerMap[] = {
    {"thumb", 0, CAN_MSG_ID_FINGER1},
    {"index", 1, CAN_MSG_ID_FINGER2},
    {"middle", 2, CAN_MSG_ID_FINGER3},
    {"ring", 3, CAN_MSG_ID_FINGER4},
    {"pinky", 4, CAN_MSG_ID_FINGER5}
};

void can_finger_ctrl(Can_Msg_Id_t id, int16_t motor1, int16_t motor2, int16_t motor3, bool feedback);

// 通过名称获取手指索引和CAN ID
bool getFingerIndexAndCanId(const char* fingerName, int& index, Can_Msg_Id_t& canId) {
    for (int i = 0; i < 5; i++) {
        if (strcmp(fingerName, fingerMap[i].name) == 0) {
            index = fingerMap[i].index;
            canId = fingerMap[i].canId;
            return true;
        }
    }
    return false;
}

// 处理WebSocket事件
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] 断开连接!\n", num);
            break;
            
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] 连接成功! IP: %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            }
            break;
            
        case WStype_TEXT:
            {
                // 解析JSON数据
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);
                
                if (error) {
                    Serial.println("JSON解析错误");
                    return;
                }

                // 打印接收到的数据
                Serial.println("收到WebSocket消息:");
                serializeJsonPretty(doc, Serial);
                Serial.println();

                // 根据消息类型处理
                const char* msgType = doc["type"];
                
                if (strcmp(msgType, "control") == 0) {
                    // 处理手指控制命令
                    const char* finger = doc["finger"];
                    const char* action = doc["action"];
                    float value = doc["value"];
                    
                    Serial.printf("手指控制: %s, 动作: %s, 值: %.2f\n", 
                                 finger, action, value);
                    
                    // 获取手指索引和CAN ID
                    int fingerIndex;
                    Can_Msg_Id_t canId;
                    
                    if (getFingerIndexAndCanId(finger, fingerIndex, canId)) {
                        // 更新对应滑块的值
                        if (strcmp(action, "slider1") == 0) {
                            fingerValues[fingerIndex].slider1 = value;
                        } else if (strcmp(action, "slider2") == 0) {
                            fingerValues[fingerIndex].slider2 = value;
                        } else if (strcmp(action, "slider3") == 0) {
                            fingerValues[fingerIndex].slider3 = value;
                        }
                        
                        // 发送CAN控制命令
                        can_finger_ctrl(
                            canId,
                            fingerValues[fingerIndex].slider1,
                            fingerValues[fingerIndex].slider2,
                            fingerValues[fingerIndex].slider3,
                            1
                        );
                        
                        // 发送状态更新到客户端
                        DynamicJsonDocument statusDoc(1024);
                        statusDoc["type"] = "status";
                        
                        char keyBuffer[20];
                        sprintf(keyBuffer, "%sPitch", finger);
                        statusDoc[keyBuffer] = fingerValues[fingerIndex].slider1;
                        
                        sprintf(keyBuffer, "%sYaw", finger);
                        statusDoc[keyBuffer] = fingerValues[fingerIndex].slider2;
                        
                        sprintf(keyBuffer, "%sRoll", finger);
                        statusDoc[keyBuffer] = fingerValues[fingerIndex].slider3;
                        
                        String statusMessage;
                        serializeJson(statusDoc, statusMessage);
                        webSocket.sendTXT(num, statusMessage);
                    } else {
                        Serial.println("无效的手指名称");
                    }
                }
                else if (strcmp(msgType, "preset") == 0) {
                    // 处理预设动作命令
                    const char* action = doc["action"];
                    Serial.printf("预设动作: %s\n", action);
                    
                    if (strcmp(action, "wave") == 0) {      
                    } else if (strcmp(action, "peace") == 0) {
                        // 实现比耶动作
                    } else if (strcmp(action, "ok") == 0) {
                        // 实现OK动作
                    } else if (strcmp(action, "fist") == 0) {
                        // 实现握拳动作
                    } else if (strcmp(action, "point") == 0) {
                        // 实现指向动作
                    } else if (strcmp(action, "rock") == 0) {
                        // 实现摇滚动作
                    }
                }
                else if (strcmp(msgType, "heartbeat") == 0) {
                    // 处理心跳包
                    // 仅回复一个简单的确认，保持连接活跃
                    DynamicJsonDocument heartbeatDoc(64);
                    heartbeatDoc["type"] = "heartbeat_ack";
                    
                    String heartbeatResponse;
                    serializeJson(heartbeatDoc, heartbeatResponse);
                    webSocket.sendTXT(num, heartbeatResponse);
                }
            }
            break;
    }
}

void can_finger_ctrl(Can_Msg_Id_t id, int16_t motor1, int16_t motor2, int16_t motor3, bool feedback) {
    CanFrame obdFrame = { 0 };
    
    /* 设置消息参数 */
    obdFrame.identifier = id;
    obdFrame.extd = 0;
    obdFrame.rtr = 0;
    obdFrame.data_length_code = 8;

    /* 填充数据 */
    obdFrame.data[0] = (uint8_t)(motor1 >> 8);
    obdFrame.data[1] = (uint8_t)(motor1 & 0xFF);
    obdFrame.data[2] = (uint8_t)(motor2 >> 8);
    obdFrame.data[3] = (uint8_t)(motor2 & 0xFF);
    obdFrame.data[4] = (uint8_t)(motor3 >> 8);
    obdFrame.data[5] = (uint8_t)(motor3 & 0xFF);
    obdFrame.data[6] = feedback;
    obdFrame.data[7] = 0;
    
    /* 发送消息 */
    ESP32Can.writeFrame(obdFrame);  // timeout defaults to 1 ms
}

void setup() {
    // Serial.end();
    // Serial.begin(115200);
    
    // 设置CAN引脚
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
    
    // 设置队列大小
    ESP32Can.setRxQueueSize(20);
    ESP32Can.setTxQueueSize(20);

    // 设置CAN通信速度
    ESP32Can.setSpeed(ESP32Can.convertSpeed(1000));

    // 启动CAN总线
    ESP32Can.begin();

    Serial.println("系统启动...");

    // 设置AP模式
    WiFi.mode(WIFI_STA);
    // WiFi.softAP(AP_SSID, AP_PASSWORD);
    WiFi.begin(STA_SSID,STA_PASSWORD);

    // 启动WebSocket服务器
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    server.begin();
    Serial.println("WebSocket服务器已启动");
}

void loop() {
    webSocket.loop();
}

