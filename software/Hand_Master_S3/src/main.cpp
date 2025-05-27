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

// 网页内容
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>灵巧手控制中心</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@6.0.0/css/all.min.css" rel="stylesheet">
    <style>
        :root {
            --bg-primary: #f8f9fa;
            --bg-secondary: #ffffff;
            --bg-tertiary: #f1f3f5;
            --accent-primary: #4dabf7;
            --accent-secondary: #339af0;
            --text-primary: #212529;
            --text-secondary: #495057;
            --border-radius: 12px;
            --transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            --shadow-sm: 0 2px 4px rgba(0, 0, 0, 0.05);
            --shadow-md: 0 4px 6px rgba(0, 0, 0, 0.1);
            --shadow-lg: 0 8px 16px rgba(0, 0, 0, 0.1);
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }

        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.05); }
            100% { transform: scale(1); }
        }

        @keyframes glow {
            0% { box-shadow: 0 0 5px var(--accent-primary); }
            50% { box-shadow: 0 0 20px var(--accent-primary); }
            100% { box-shadow: 0 0 5px var(--accent-primary); }
        }

        @keyframes slideIn {
            from { transform: translateX(-20px); opacity: 0; }
            to { transform: translateX(0); opacity: 1; }
        }

        body {
            background: var(--bg-primary);
            color: var(--text-primary);
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
            min-height: 100vh;
            margin: 0;
            padding: 0;
        }

        .app-container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 2rem;
            animation: fadeIn 0.5s ease-out;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 3rem;
            padding: 1.5rem;
            background: var(--bg-secondary);
            border-radius: var(--border-radius);
            box-shadow: var(--shadow-sm);
            transition: var(--transition);
            position: relative;
            overflow: hidden;
        }

        .header::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 4px;
            background: linear-gradient(90deg, var(--accent-primary), var(--accent-secondary));
            opacity: 0;
            transition: var(--transition);
        }

        .header:hover::before {
            opacity: 1;
        }

        .logo {
            display: flex;
            align-items: center;
            gap: 1rem;
            animation: slideIn 0.5s ease-out;
        }

        .logo-icon {
            width: 40px;
            height: 40px;
            background: var(--accent-primary);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 1.2rem;
            transition: var(--transition);
            position: relative;
            overflow: hidden;
        }

        .logo-icon::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, transparent, rgba(255, 255, 255, 0.2), transparent);
            transform: translateX(-100%);
            transition: var(--transition);
        }

        .logo-icon:hover::after {
            transform: translateX(100%);
        }

        .logo-text {
            font-size: 1.5rem;
            font-weight: 700;
            background: linear-gradient(135deg, var(--accent-primary), var(--accent-secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            position: relative;
        }

        .logo-text::after {
            content: '';
            position: absolute;
            bottom: 0;
            left: 0;
            width: 100%;
            height: 2px;
            background: linear-gradient(90deg, transparent, var(--accent-primary), transparent);
            opacity: 0;
            transition: var(--transition);
        }

        .logo:hover .logo-text::after {
            opacity: 1;
        }

        .connection-status {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            padding: 0.8rem 1.2rem;
            background: var(--bg-tertiary);
            border-radius: var(--border-radius);
            transition: var(--transition);
        }

        .connection-status:hover {
            transform: translateY(-2px);
            box-shadow: var(--shadow-md);
        }

        .ip-input-container {
            position: relative;
            max-width: 200px;
            margin-right: 0.7rem;
        }

        .ip-input-container .form-control {
            border-radius: var(--border-radius);
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 0.5rem 1rem 0.5rem 2.5rem;
            transition: var(--transition);
            font-size: 0.9rem;
        }

        .ip-input-container .form-control:focus {
            border-color: var(--accent-primary);
            box-shadow: 0 0 0 3px rgba(77, 171, 247, 0.2);
            outline: none;
        }

        .ip-input-container .input-icon {
            position: absolute;
            left: 0.7rem;
            top: 50%;
            transform: translateY(-50%);
            color: var(--accent-primary);
            font-size: 0.9rem;
        }

        .ip-input-container .input-group-text {
            background-color: transparent;
            border: none;
            padding-right: 0;
        }

        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #ff6b6b;
            transition: var(--transition);
        }

        .status-indicator.connected {
            background: var(--accent-primary);
            animation: pulse 2s infinite, glow 2s infinite;
        }

        .main-content {
            display: grid;
            grid-template-columns: 1fr 300px;
            gap: 2rem;
        }

        .control-panel {
            background: var(--bg-secondary);
            border-radius: var(--border-radius);
            padding: 2rem;
            box-shadow: var(--shadow-sm);
            transition: var(--transition);
        }

        .control-panel:hover {
            box-shadow: var(--shadow-md);
        }

        .finger-control {
            background: var(--bg-tertiary);
            border-radius: var(--border-radius);
            padding: 1.5rem;
            margin-bottom: 1.5rem;
            border: 1px solid rgba(0, 0, 0, 0.05);
            transition: var(--transition);
            animation: fadeIn 0.5s ease-out;
        }

        .finger-control:hover {
            transform: translateY(-2px);
            box-shadow: var(--shadow-sm);
        }

        .finger-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1rem;
            padding-bottom: 0.5rem;
            border-bottom: 2px solid rgba(0, 0, 0, 0.05);
        }

        .finger-name {
            font-size: 1.2rem;
            font-weight: 600;
            color: var(--text-primary);
            position: relative;
        }

        .finger-name::after {
            content: '';
            position: absolute;
            bottom: -0.5rem;
            left: 0;
            width: 0;
            height: 2px;
            background: var(--accent-primary);
            transition: var(--transition);
        }

        .finger-control:hover .finger-name::after {
            width: 100%;
        }

        .slider-container {
            margin: 1.5rem 0;
            animation: slideIn 0.5s ease-out;
            position: relative;
        }

        .slider-label {
            display: flex;
            justify-content: space-between;
            margin-bottom: 0.5rem;
            color: var(--text-secondary);
            font-size: 0.9rem;
        }

        .slider {
            width: 100%;
            height: 4px;
            background: #e9ecef;
            border-radius: 2px;
            outline: none;
            -webkit-appearance: none;
            transition: background 0.2s ease;
            cursor: pointer;
        }

        .slider:focus {
            outline: none;
        }

        .slider:hover {
            background: #dee2e6;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 16px;
            height: 16px;
            background: var(--accent-primary);
            border-radius: 50%;
            cursor: pointer;
            transition: all 0.2s ease;
            border: 2px solid white;
            box-shadow: var(--shadow-sm);
        }

        .slider::-webkit-slider-thumb:hover {
            transform: scale(1.2);
            box-shadow: var(--shadow-md);
        }

        .slider::-webkit-slider-thumb:active {
            transform: scale(1.1);
            box-shadow: 0 0 0 4px rgba(77, 171, 247, 0.2);
        }

        .slider::-moz-range-thumb {
            width: 16px;
            height: 16px;
            background: var(--accent-primary);
            border-radius: 50%;
            cursor: pointer;
            transition: all 0.2s ease;
            border: 2px solid white;
            box-shadow: var(--shadow-sm);
        }

        .slider::-moz-range-thumb:hover {
            transform: scale(1.2);
            box-shadow: var(--shadow-md);
        }

        .slider::-moz-range-thumb:active {
            transform: scale(1.1);
            box-shadow: 0 0 0 4px rgba(77, 171, 247, 0.2);
        }

        .slider-track {
            position: absolute;
            top: 50%;
            left: 0;
            width: 100%;
            height: 4px;
            background: #e9ecef;
            border-radius: 2px;
            transform: translateY(-50%);
            pointer-events: none;
        }

        .slider-progress {
            position: absolute;
            top: 50%;
            left: 0;
            height: 4px;
            background: var(--accent-primary);
            border-radius: 2px;
            transform: translateY(-50%);
            pointer-events: none;
            transition: width 0.1s ease;
        }

        .actions-panel {
            background: var(--bg-secondary);
            border-radius: var(--border-radius);
            padding: 2rem;
            box-shadow: var(--shadow-sm);
            transition: var(--transition);
        }

        .actions-panel:hover {
            box-shadow: var(--shadow-md);
        }

        .action-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 1rem;
            margin-top: 1.5rem;
        }

        .action-btn {
            background: var(--bg-tertiary);
            border: 1px solid rgba(0, 0, 0, 0.05);
            border-radius: var(--border-radius);
            padding: 1.2rem;
            color: var(--text-primary);
            font-weight: 600;
            cursor: pointer;
            transition: var(--transition);
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 0.8rem;
            position: relative;
            overflow: hidden;
        }

        .action-btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, transparent, rgba(255, 255, 255, 0.2), transparent);
            transform: translateX(-100%);
            transition: var(--transition);
        }

        .action-btn:hover {
            background: var(--accent-primary);
            color: white;
            transform: translateY(-2px);
            box-shadow: var(--shadow-md);
        }

        .action-btn:hover::before {
            transform: translateX(100%);
        }

        .action-icon {
            font-size: 1.8rem;
            transition: var(--transition);
            width: 24px;
            height: 24px;
            object-fit: contain;
        }

        .action-btn:hover .action-icon {
            transform: scale(1.2);
        }

        .btn-primary {
            background: var(--accent-primary);
            border: none;
            color: white;
            padding: 0.8rem 1.5rem;
            border-radius: var(--border-radius);
            font-weight: 600;
            transition: var(--transition);
            position: relative;
            overflow: hidden;
        }

        .btn-primary::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, transparent, rgba(255, 255, 255, 0.2), transparent);
            transform: translateX(-100%);
            transition: var(--transition);
        }

        .btn-primary:hover {
            background: var(--accent-secondary);
            transform: translateY(-2px);
            box-shadow: var(--shadow-md);
        }

        .btn-primary:hover::before {
            transform: translateX(100%);
        }

        @media (max-width: 1024px) {
            .main-content {
                grid-template-columns: 1fr;
            }
        }

        @media (max-width: 768px) {
            .app-container {
                padding: 1rem;
            }

            .header {
                flex-direction: column;
                gap: 1rem;
            }

            .action-grid {
                grid-template-columns: 1fr;
            }
        }

        /* 添加弹窗样式 */
        .toast-container {
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 1000;
        }

        .toast {
            background: rgba(0, 0, 0, 0.8);
            color: white;
            padding: 12px 24px;
            border-radius: 8px;
            margin-bottom: 10px;
            display: flex;
            align-items: center;
            gap: 10px;
            animation: slideIn 0.3s ease-out, fadeOut 0.3s ease-out 2.7s;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        }

        .toast i {
            color: #4dabf7;
        }

        .toast.error i {
            color: #ff6b6b;
        }

        .toast.success i {
            color: #51cf66;
        }

        @keyframes slideIn {
            from {
                transform: translateX(100%);
                opacity: 0;
            }
            to {
                transform: translateX(0);
                opacity: 1;
            }
        }

        @keyframes fadeOut {
            from {
                opacity: 1;
            }
            to {
                opacity: 0;
            }
        }
    </style>
</head>
<body>
    <div class="toast-container" id="toastContainer"></div>
    <div class="app-container">
        <header class="header">
            <div class="logo">
                <div class="logo-icon">
                    <i class="fas fa-hand"></i>
                </div>
                <div class="logo-text">灵巧手控制中心</div>
            </div>
            <div class="connection-status">
                <div class="status-indicator" id="connectionStatus"></div>
                <span id="statusText" class="me-2">未连接</span>
                <div class="ip-input-container">
                    <div class="input-icon">
                        <i class="fas fa-network-wired"></i>
                    </div>
                    <input type="text" class="form-control" id="ipInput" placeholder="输入IP地址" value="192.168.137.110">
                </div>
                <button class="btn btn-primary" id="connectBtn">连接设备</button>
            </div>
        </header>

        <main class="main-content">
            <div class="control-panel">
                <div class="finger-control">
                    <div class="finger-header">
                        <div class="finger-name">食指</div>
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块1</span>
                            <span id="thumbPitchValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="thumbPitch" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块2</span>
                            <span id="thumbYawValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="thumbYaw" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块3</span>
                            <span id="thumbRollValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="thumbRoll" min="0" max="30" value="0">
                    </div>
                </div>

                <div class="finger-control">
                    <div class="finger-header">
                        <div class="finger-name">中指</div>
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块1</span>
                            <span id="indexPitchValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="indexPitch" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块2</span>
                            <span id="indexYawValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="indexYaw" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块3</span>
                            <span id="indexRollValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="indexRoll" min="0" max="30" value="0">
                    </div>
                </div>

                <div class="finger-control">
                    <div class="finger-header">
                        <div class="finger-name">无名指</div>
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块1</span>
                            <span id="middlePitchValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="middlePitch" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块2</span>
                            <span id="middleYawValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="middleYaw" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块3</span>
                            <span id="middleRollValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="middleRoll" min="0" max="30" value="0">
                    </div>
                </div>

                <div class="finger-control">
                    <div class="finger-header">
                        <div class="finger-name">小拇指</div>
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块1</span>
                            <span id="ringPitchValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="ringPitch" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块2</span>
                            <span id="ringYawValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="ringYaw" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块3</span>
                            <span id="ringRollValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="ringRoll" min="0" max="30" value="0">
                    </div>
                </div>

                <div class="finger-control">
                    <div class="finger-header">
                        <div class="finger-name">大拇指</div>
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块1</span>
                            <span id="pinkyPitchValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="pinkyPitch" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块2</span>
                            <span id="pinkyYawValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="pinkyYaw" min="0" max="30" value="0">
                    </div>
                    <div class="slider-container">
                        <div class="slider-label">
                            <span>滑块3</span>
                            <span id="pinkyRollValue">0mm</span>
                        </div>
                        <input type="range" class="slider" id="pinkyRoll" min="0" max="30" value="0">
                    </div>
                </div>
            </div>

            <div class="actions-panel">
                <h3>预设动作</h3>
                <div class="action-grid">
                    <button class="action-btn" id="waveBtn">
                        <i class="fas fa-hand-paper action-icon"></i>
                        <span>挥手</span>
                    </button>
                    <button class="action-btn" id="peaceBtn">
                        <i class="fas fa-hand-peace action-icon"></i>
                        <span>比耶</span>
                    </button>
                    <button class="action-btn" id="bendBtn">
                        <i class="fas fa-hand-point-up action-icon"></i>
                        <span>弯曲指头</span>
                    </button>
                    <button class="action-btn" id="fistBtn">
                        <i class="fas fa-hand-rock action-icon"></i>
                        <span>握拳</span>
                    </button>
                    <button class="action-btn" id="rockBtn">
                        <img src="..\icon\rock.png" class="action-icon" alt="摇滚">
                        <span>摇滚</span>
                    </button>
                </div>
            </div>
        </main>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"></script>
    <script>
        // WebSocket连接
        let ws = null;
        // 使用可输入的IP地址
        let ipAddress = localStorage.getItem('handControlIp') || "192.168.137.110";
        let wsUrl = `ws://${ipAddress}:81`;
        let reconnectInterval = null;
        let reconnectAttempts = 0;
        const maxReconnectAttempts = 10;
        const reconnectDelay = 3000; // 3秒重连间隔
        
        // 节流函数
        function throttle(func, limit) {
            let inThrottle;
            return function(...args) {
                if (!inThrottle) {
                    func.apply(this, args);
                    inThrottle = true;
                    setTimeout(() => inThrottle = false, limit);
                }
            }
        }

        // 防抖函数
        function debounce(func, wait) {
            let timeout;
            return function(...args) {
                clearTimeout(timeout);
                timeout = setTimeout(() => {
                    func.apply(this, args);
                }, wait);
            };
        }

        // 连接状态管理
        const connectBtn = document.getElementById('connectBtn');
        const connectionStatus = document.getElementById('connectionStatus');
        const statusText = document.getElementById('statusText');
        let isConnected = false;

        // 初始化滑动条
        function initSliders() {
            const sliders = document.querySelectorAll('.slider');
            sliders.forEach(slider => {
                const container = slider.parentElement;
                const progress = document.createElement('div');
                progress.className = 'slider-progress';
                container.appendChild(progress);

                const updateProgress = () => {
                    const value = slider.value;
                    const max = slider.max;
                    const min = slider.min;
                    const percent = ((value - min) / (max - min)) * 100;
                    progress.style.width = `${percent}%`;
                };

                updateProgress();

                const valueDisplay = document.getElementById(`${slider.id}Value`);
                
                // 提高节流限制，减少发送频率
                const updateValue = throttle((value) => {
                    valueDisplay.textContent = `${value}mm`;
                    
                    if (isConnected && ws) {
                        let fingerName = slider.id;
                        // 确定滑块类型和手指名称
                        let sliderType = '';
                        if (fingerName.includes('Pitch')) {
                            sliderType = 'slider1';
                            fingerName = fingerName.replace('Pitch', '');
                        } else if (fingerName.includes('Yaw')) {
                            sliderType = 'slider2';
                            fingerName = fingerName.replace('Yaw', '');
                        } else if (fingerName.includes('Roll')) {
                            sliderType = 'slider3';
                            fingerName = fingerName.replace('Roll', '');
                        }

                        const command = {
                            type: 'control',
                            finger: fingerName,
                            action: sliderType,
                            value: parseFloat(value)
                        };
                        
                        // 使用requestAnimationFrame避免阻塞UI
                        requestAnimationFrame(() => {
                            ws.send(JSON.stringify(command));
                            // 调试输出
                            console.log(`发送WebSocket消息:`, command);
                        });
                    }
                }, 300); // 提高到100ms以减少网络请求频率

                // 使用即时反馈进度条，但延迟发送WebSocket消息
                slider.addEventListener('input', (e) => {
                    const value = e.target.value;
                    // 立即更新UI
                    updateProgress();
                    valueDisplay.textContent = `${value}mm`;
                    // 节流发送消息
                    updateValue(value);
                });
            });
        }

        // 页面加载完成后初始化滑动条
        document.addEventListener('DOMContentLoaded', () => {
            initSliders();
            
            // 设置默认IP地址
            const ipInput = document.getElementById('ipInput');
            ipInput.value = ipAddress;
            
            // 监听IP输入变化
            ipInput.addEventListener('change', (e) => {
                ipAddress = e.target.value.trim();
                if (!ipAddress) {
                    ipAddress = "192.168.137.110";
                    ipInput.value = ipAddress;
                }
                wsUrl = `ws://${ipAddress}:81`;
                localStorage.setItem('handControlIp', ipAddress);
                showToast('IP地址已更新', 'info');
            });
            
            // 按回车键连接
            ipInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    if (!isConnected) {
                        connectWebSocket();
                    } else {
                        disconnectWebSocket();
                    }
                }
            });
        });

        function connectWebSocket() {
            // 再次获取最新的IP地址
            const ipInput = document.getElementById('ipInput');
            ipAddress = ipInput.value.trim();
            wsUrl = `ws://${ipAddress}:81`;
            localStorage.setItem('handControlIp', ipAddress);
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = () => {
                isConnected = true;
                connectionStatus.classList.add('connected');
                statusText.textContent = '已连接';
                connectBtn.textContent = '断开连接';
                showToast('设备连接成功', 'success');
                
                // 重置重连计数器
                reconnectAttempts = 0;
                if (reconnectInterval) {
                    clearInterval(reconnectInterval);
                    reconnectInterval = null;
                }
                
                // 添加心跳包以保持连接
                startHeartbeat();
            };

            ws.onclose = () => {
                isConnected = false;
                connectionStatus.classList.remove('connected');
                statusText.textContent = '连接断开, 尝试重连中...';
                connectBtn.textContent = '连接设备';
                showToast('设备连接断开，正在尝试重连...', 'error');
                
                // 停止心跳
                stopHeartbeat();
                
                // 自动重连
                if (!reconnectInterval && reconnectAttempts < maxReconnectAttempts) {
                    reconnectInterval = setInterval(() => {
                        if (reconnectAttempts < maxReconnectAttempts) {
                            reconnectAttempts++;
                            console.log(`尝试重连 ${reconnectAttempts}/${maxReconnectAttempts}`);
                            connectWebSocket();
                        } else {
                            clearInterval(reconnectInterval);
                            reconnectInterval = null;
                            statusText.textContent = '重连失败，请手动连接';
                        }
                    }, reconnectDelay);
                }
            };

            ws.onerror = (error) => {
                showToast('WebSocket连接错误', 'error');
                console.error('WebSocket错误:', error);
            };

            ws.onmessage = (event) => {
                console.log('收到消息:', event.data);
                try {
                    const data = JSON.parse(event.data);
                    updateUI(data);
                } catch (e) {
                    console.error('解析消息失败:', e);
                }
            };
        }

        function disconnectWebSocket() {
            // 停止心跳
            stopHeartbeat();
            
            // 停止自动重连
            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }
            
            if (ws) {
                ws.close();
                ws = null;
            }
        }
        
        // 心跳包保持连接
        let heartbeatInterval = null;
        
        function startHeartbeat() {
            stopHeartbeat(); // 确保不会有多个心跳
            heartbeatInterval = setInterval(() => {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    // 发送心跳包
                    ws.send(JSON.stringify({type: 'heartbeat'}));
                }
            }, 30000); // 每30秒发送一次心跳
        }
        
        function stopHeartbeat() {
            if (heartbeatInterval) {
                clearInterval(heartbeatInterval);
                heartbeatInterval = null;
            }
        }

        connectBtn.addEventListener('click', () => {
            if (!isConnected) {
                connectWebSocket();
            } else {
                disconnectWebSocket();
            }
        });

        // 预设动作按钮
        const actionButtons = document.querySelectorAll('.action-btn');
        actionButtons.forEach(button => {
            button.addEventListener('click', (e) => {
                if (isConnected && ws) {
                    const buttonId = e.currentTarget.id.replace('Btn', '');
                    
                    // 特殊处理挥手动作
                    if (buttonId === 'wave') {
                        performWaveAction();
                    } else if (buttonId === 'rock') {
                        // 特殊处理摇滚动作
                        performRockAction();
                    } else if (buttonId === 'fist') {
                        // 特殊处理握拳动作
                        performFistAction();
                    } else if (buttonId === 'peace') {
                        // 特殊处理比耶动作
                        performPeaceAction();
                    } else if (buttonId === 'bend') {
                        // 特殊处理弯曲指头动作
                        performBendAction();
                    } else {
                        // 其他动作仍然使用原来的方式
                        const command = {
                            type: 'preset',
                            action: buttonId
                        };
                        ws.send(JSON.stringify(command));
                    }
                }
            });
        });

        // 执行挥手动作的函数
        let isWaveActionInProgress = false; // 添加标志变量
        
        function performWaveAction() {
            if (isWaveActionInProgress) {
                showToast('挥手动作正在执行中，请等待完成', 'error');
                return;
            }
            
            isWaveActionInProgress = true; // 设置标志为正在执行
            const fingers = ['thumb', 'index', 'middle', 'ring', 'pinky'];
            const delay = 400; // 每个动作之间的延迟时间（毫秒）
            
            // 第一步：每个手指的滑块1(Pitch)赋值为5
            fingers.forEach(finger => {
                const command = {
                    type: 'control',
                    finger: finger,
                    action: 'slider1',
                    value: 5
                };
                ws.send(JSON.stringify(command));
            });
            
            // 第二步：每个手指的滑块1(Pitch)复原为0
            setTimeout(() => {
                fingers.forEach(finger => {
                    const command = {
                        type: 'control',
                        finger: finger,
                        action: 'slider1',
                        value: 0
                    };
                    ws.send(JSON.stringify(command));
                });
                
                // 第三步：每个手指的滑块3(Roll)赋值为5
                setTimeout(() => {
                    fingers.forEach(finger => {
                        const command = {
                            type: 'control',
                            finger: finger,
                            action: 'slider3',
                            value: 5
                        };
                        ws.send(JSON.stringify(command));
                    });
                    
                    // 第四步：每个手指的滑块3(Roll)复原为0
                    setTimeout(() => {
                        fingers.forEach(finger => {
                            const command = {
                                type: 'control',
                                finger: finger,
                                action: 'slider3',
                                value: 0
                            };
                            ws.send(JSON.stringify(command));
                        });
                        
                        // 动作完成，重置标志
                        isWaveActionInProgress = false;
                    }, delay);
                }, delay);
            }, delay);
        }
        
        // 执行摇滚动作的函数
        let isRockActionInProgress = false; // 添加标志变量
        
        function performRockAction() {
            if (isRockActionInProgress) {
                showToast('摇滚动作正在执行中，请等待完成', 'error');
                return;
            }
            
            isRockActionInProgress = true; // 设置标志为正在执行
            const fingers = ['index', 'middle']; // 只处理食指和中指
            const delay = 2000; // 每个动作之间的延迟时间（毫秒）
            
            // 第一步：食指和中指的滑块1、2、3赋值为30
            fingers.forEach(finger => {
                // 滑块1 (Pitch)
                const command1 = {
                    type: 'control',
                    finger: finger,
                    action: 'slider1',
                    value: 30
                };
                ws.send(JSON.stringify(command1));
                
                // 滑块2 (Yaw)
                const command2 = {
                    type: 'control',
                    finger: finger,
                    action: 'slider2',
                    value: 30
                };
                ws.send(JSON.stringify(command2));
                
                // 滑块3 (Roll)
                const command3 = {
                    type: 'control',
                    finger: finger,
                    action: 'slider3',
                    value: 30
                };
                ws.send(JSON.stringify(command3));
            });
            
            // 第二步：食指和中指的滑块1、2、3复原为0
            setTimeout(() => {
                fingers.forEach(finger => {
                    // 滑块1 (Pitch)
                    const command1 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider1',
                        value: 0
                    };
                    ws.send(JSON.stringify(command1));
                    
                    // 滑块2 (Yaw)
                    const command2 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider2',
                        value: 0
                    };
                    ws.send(JSON.stringify(command2));
                    
                    // 滑块3 (Roll)
                    const command3 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider3',
                        value: 0
                    };
                    ws.send(JSON.stringify(command3));
                });
                
                // 动作完成，重置标志
                isRockActionInProgress = false;
            }, delay);
        }

        // 执行握拳动作的函数
        let isFistActionInProgress = false; // 添加标志变量
        
        function performFistAction() {
            if (isFistActionInProgress) {
                showToast('握拳动作正在执行中，请等待完成', 'error');
                return;
            }
            
            isFistActionInProgress = true; // 设置标志为正在执行
            const fingers = ['pinky', 'ring', 'middle', 'index', 'thumb']; // 从外到内的顺序
            const delay = 400; // 每个手指之间的延迟时间（毫秒）
            
            // 第一步：依次弯曲每个手指
            fingers.forEach((finger, index) => {
                setTimeout(() => {
                    // 滑块1 (Pitch)
                    const command1 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider1',
                        value: 30
                    };
                    ws.send(JSON.stringify(command1));
                    
                    // 滑块3 (Roll)
                    const command3 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider3',
                        value: 30
                    };
                    ws.send(JSON.stringify(command3));
                    
                    // 滑块2 (Yaw)
                    const command2 = {
                        type: 'control',
                        finger: finger,
                        action: 'slider2',
                        value: 30
                    };
                    ws.send(JSON.stringify(command2));
                    
                    // 如果是最后一个手指，开始复原过程
                    if (index === fingers.length - 1) {
                        // 等待一段时间后开始复原
                        setTimeout(() => {
                            // 从内到外复原（反转数组顺序）
                            [...fingers].reverse().forEach((finger, index) => {
                                setTimeout(() => {
                                    // 滑块1 (Pitch)
                                    const command1 = {
                                        type: 'control',
                                        finger: finger,
                                        action: 'slider1',
                                        value: 0
                                    };
                                    ws.send(JSON.stringify(command1));
                                    
                                    // 滑块3 (Roll)
                                    const command3 = {
                                        type: 'control',
                                        finger: finger,
                                        action: 'slider3',
                                        value: 0
                                    };
                                    ws.send(JSON.stringify(command3));
                                    
                                    // 滑块2 (Yaw)
                                    const command2 = {
                                        type: 'control',
                                        finger: finger,
                                        action: 'slider2',
                                        value: 0
                                    };
                                    ws.send(JSON.stringify(command2));
                                    
                                    // 如果是最后一个手指，重置标志
                                    if (index === fingers.length - 1) {
                                        isFistActionInProgress = false;
                                    }
                                }, index * delay);
                            });
                        }, 1500); // 等待1.5秒后开始复原
                    }
                }, index * delay);
            });
        }

        // 执行比耶动作的函数
        let isPeaceActionInProgress = false; // 添加标志变量
        
        function performPeaceAction() {
            if (isPeaceActionInProgress) {
                showToast('比耶动作正在执行中，请等待完成', 'error');
                return;
            }
            
            isPeaceActionInProgress = true; // 设置标志为正在执行
            const delay = 400; // 每个动作之间的延迟时间（毫秒）
            
            // 第一步：弯曲无名指
            setTimeout(() => {
                // 无名指的滑块1、2、3赋值为30
                const ringCommands = [
                    {
                        type: 'control',
                        finger: 'ring',
                        action: 'slider1',
                        value: 30
                    },
                    {
                        type: 'control',
                        finger: 'ring',
                        action: 'slider2',
                        value: 30
                    },
                    {
                        type: 'control',
                        finger: 'ring',
                        action: 'slider3',
                        value: 30
                    }
                ];
                ringCommands.forEach(cmd => ws.send(JSON.stringify(cmd)));
                
                // 第二步：弯曲中指
                setTimeout(() => {
                    // 中指的滑块1、2、3赋值为30
                    const middleCommands = [
                        {
                            type: 'control',
                            finger: 'middle',
                            action: 'slider1',
                            value: 30
                        },
                        {
                            type: 'control',
                            finger: 'middle',
                            action: 'slider2',
                            value: 30
                        },
                        {
                            type: 'control',
                            finger: 'middle',
                            action: 'slider3',
                            value: 30
                        }
                    ];
                    middleCommands.forEach(cmd => ws.send(JSON.stringify(cmd)));
                    
                    // 第三步：设置大拇指和食指的特殊位置
                    setTimeout(() => {
                        // 大拇指的滑块1赋值为5
                        const thumbCommand = {
                            type: 'control',
                            finger: 'thumb',
                            action: 'slider1',
                            value: 7
                        };
                        ws.send(JSON.stringify(thumbCommand));
                        
                        // 食指的滑块3赋值为5
                        const indexCommand = {
                            type: 'control',
                            finger: 'index',
                            action: 'slider3',
                            value: 7
                        };
                        ws.send(JSON.stringify(indexCommand));
                        
                        // 等待一段时间后开始复原
                        setTimeout(() => {
                            // 按照大拇指到无名指的顺序复原
                            const fingers = ['thumb', 'index', 'middle', 'ring'];
                            fingers.forEach((finger, index) => {
                                setTimeout(() => {
                                    // 复原所有滑块
                                    const resetCommands = [
                                        {
                                            type: 'control',
                                            finger: finger,
                                            action: 'slider1',
                                            value: 0
                                        },
                                        {
                                            type: 'control',
                                            finger: finger,
                                            action: 'slider2',
                                            value: 0
                                        },
                                        {
                                            type: 'control',
                                            finger: finger,
                                            action: 'slider3',
                                            value: 0
                                        }
                                    ];
                                    resetCommands.forEach(cmd => ws.send(JSON.stringify(cmd)));
                                    
                                    // 如果是最后一个手指，重置标志
                                    if (index === fingers.length - 1) {
                                        isPeaceActionInProgress = false;
                                    }
                                }, index * delay);
                            });
                        }, 1500); // 保持比耶手势1.5秒
                    }, delay);
                }, delay);
            }, 0);
        }

        // 执行弯曲指头动作的函数
        let isBendActionInProgress = false; // 添加标志变量
        
        function performBendAction() {
            // 如果动作正在执行中，直接返回
            if (isBendActionInProgress) {
                showToast('动作正在执行中，请等待完成', 'error');
                return;
            }
            
            isBendActionInProgress = true; // 设置标志为正在执行
            const delay = 400; // 每个动作之间的延迟时间（毫秒）
            
            // 定义手指和对应的滑块2值
            const fingers = [
                { name: 'thumb', value: 14 },
                { name: 'index', value: 9 },
                { name: 'middle', value: 8 },
                { name: 'ring', value: 11 }
            ];
            
            // 依次设置每个手指的滑块2值
            fingers.forEach((finger, index) => {
                setTimeout(() => {
                    const command = {
                        type: 'control',
                        finger: finger.name,
                        action: 'slider2',
                        value: finger.value
                    };
                    ws.send(JSON.stringify(command));
                    
                    // 如果是最后一个手指，等待一段时间后开始复原
                    if (index === fingers.length - 1) {
                        setTimeout(() => {
                            // 按照相反的顺序复原
                            [...fingers].reverse().forEach((finger, index) => {
                                setTimeout(() => {
                                    const command = {
                                        type: 'control',
                                        finger: finger.name,
                                        action: 'slider2',
                                        value: 0
                                    };
                                    ws.send(JSON.stringify(command));
                                    
                                    // 如果是最后一个手指，重置标志
                                    if (index === fingers.length - 1) {
                                        isBendActionInProgress = false;
                                    }
                                }, index * delay);
                            });
                        }, 1500); // 保持手势1.5秒
                    }
                }, index * delay);
            });
        }

        // 更新UI显示
        function updateUI(data) {
            if (data.type === 'status') {
                Object.keys(data).forEach(key => {
                    if (key !== 'type') {
                        let finger = '';
                        let action = '';
                        
                        // 解析key获取手指和动作类型
                        if (key.includes('Pitch')) {
                            action = 'Pitch';
                            finger = key.replace('Pitch', '');
                        } else if (key.includes('Yaw')) {
                            action = 'Yaw';
                            finger = key.replace('Yaw', '');
                        } else if (key.includes('Roll')) {
                            action = 'Roll';
                            finger = key.replace('Roll', '');
                        }
                        
                        const slider = document.getElementById(`${finger}${action}`);
                        const valueDisplay = document.getElementById(`${finger}${action}Value`);
                        if (slider && valueDisplay) {
                            slider.value = data[key];
                            valueDisplay.textContent = `${data[key]}mm`;
                        }
                    }
                });
            }
        }

        function showToast(message, type = 'info') {
            const container = document.getElementById('toastContainer');
            const toast = document.createElement('div');
            toast.className = `toast ${type}`;
            
            let icon = 'info-circle';
            if (type === 'error') icon = 'exclamation-circle';
            if (type === 'success') icon = 'check-circle';
            
            toast.innerHTML = `
                <i class="fas fa-${icon}"></i>
                <span>${message}</span>
            `;
            container.appendChild(toast);
            setTimeout(() => {
                toast.remove();
            }, 3000);
        }
    </script>
</body>
</html>
)rawliteral";

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
    Serial.begin(115200);
    
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

    // 设置WIFI AP模式
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    // 启动WebSocket服务器
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // 设置Web服务器路由
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });
    // 启动Web服务器
    server.begin();

    Serial.println("WebSocket服务器已启动");
}

void loop() {
    webSocket.loop();
}

