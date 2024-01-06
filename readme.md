# HoldingHands

## Introduction

This is free remote access trojan.

## Architecture:

### Relationships between classes

![Untitled](assets/Untitled.svg)

### Module's event handling path

![Untitled](assets/Untitled%201.svg)

![Untitled](assets/Untitled%202.svg)

## Supported functions:

1. RemoteDesktop:
    - View victim's desktop
    - Clipboard data synchronization (only text)
    - Screenshot
    - Gdi grab and x264 encode.
    - Record (Not implemented yet)
    - Support multiple minitors
2. Camera:
    - Open Camera
    - Screenshot
    - Record (Not implemented yet)
3. Microphone
    - Listen victim's microphone
    - Send local voice to remote machine.
    - Record (Not implemented yet)
4. FileManager:
    - View victim's files
    - Upload file from local disk.
    - Upload file from url
    - Download files
    - Copy,cut,and delete files
    - Search file
5. Cmd
    - Remote command execute.
6. ChatBox
    - Chat with victim
7. Proxy:
    - SocksProxy
        - support connect and udp.
        - support socks4 and socks5.
    - HTTP(s) proxy (Not implemented yet)
    - Reverse proxy(like frp) (Not implemented yet)
8. Keyload listen:
    - Implemented by setting a global GetMessage hook and hooking WM_CHAR message
9. ProcessManager:
    - View processes running on victim's PC (Support the display of process's icon)
    - Kill processes
10. Window Manager(Not implemented yet)
11. Service Manager(Not implemented yet)


## 支持的功能:

1. 远程桌面:
    - 观看对方屏幕
    - 剪切办文本自动同步
    - 截屏
    - DXGI(快) 与GDI两种抓屏
    - 录屏 (还未实现)
    - 支持多个显示器的切换
2. 摄像头:
    - 打开摄像头
    - 截图
    - 录制(未实现)
  
3. 麦克风
    - 监听麦克风
    - 发送本地语音到远程
    - 录音
4. 文件管理:
    - 文件浏览
    - 从本地上传文件
    - 从URL上传文件
    - 下载文件到本地
    - 文件复制，剪切，拷贝等
    - 文件搜索
5. CMD
    - 命令执行
6. 聊天
    - 与肉鸡对话
7. 代理:
    - Socks代理
        - 支持connect和udp associate 命令
        - 支持socs4,socks5
    - HTTP(s) 代理 (未实现)
    - 类似frp的内网穿透 (未实现)
8. 键盘记录:
    - 通过全局get message hook实现
9. 进程管理:
    - 进程浏览，支持显示进程图标
    - 结束进程
10. 窗口管理 (未实现)
11. 服务管理 (未实现)
12. 
## How to develop based on existing programs?

### Client :

1. Implement your class based on CEventHandler.

### Server:

1. Implement your class based on CEventHandler.
2. Implement your window and process messages which send by CXXXHandler. If you want to know detail steps,please reference already developed modules.



## 如何二次开发?

### Client :

1. 继承CEventHandler 实现主要的逻辑

### Server:

1. 继承CEventHandler 实现主要的逻辑
2. 设计Window 并且处理 handler 通知的消息,同时通过handler发起命令操作. 详细的步骤请参考已经开发好的模块