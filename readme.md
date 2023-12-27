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

## How to develop based on existing programs?

### Client :

1. Implement your class based on CEventHandler.

### Server:

1. Implement your class based on CEventHandler.
2. Implement your window and process messages which send by CXXXHandler. If you want to know detail steps,please reference already developed modules.