# ✈️ MSFS-SimGPStoAndroid

> 将微软模拟飞行（MSFS 2020 / 2024）中飞机的实时位置，模拟到安卓手机的系统定位上，让你可以使用第三方地图进行导航，现实中的 EFB 软件理论上也兼容。

![预览图](IMG/1.jpg)

![截图一](IMG/2.jpg)

## ✨ 功能特色

- 📍 **模拟定位**：通过 Android 模拟位置提供者注入位置，支持最多 3 台手机同时连接
- 🖥️ **电脑端可调**：数据刷新间隔（默认 50ms）、监听端口（默认 36666）均可自定义
- 🎯 **平滑补偿**：手机端按可调频率（5~1000Hz，默认 25Hz）在真实帧之间做预测插值，航向/速度平滑系数可调，减少地图上的卡顿
- 🪟 **状态悬浮窗**：半透明小卡片，显示状态、坐标、地速/空速、航向、高度与网络延迟，透明度可调，通知栏可一键开关
- 🌐 **公网使用**：Qt 6 版自动选择可用的公网 IPv6；使用公网前需在路由器或防火墙中手动放行 UDP 端口
- ⚡ **网络延迟**：PONG 往返测量 + 平滑显示，局域网延迟稳定在个位数毫秒级

## 📲 使用

1. 📥 手机安装 APK，在 系统设置 → 开发者选项 → 选择模拟位置信息应用 中选中本应用
2. 🚀 电脑端运行 MSFS-SimConnect（首次运行允许防火墙，放行 UDP 36666）
3. 🔗 手机 App 填写电脑 IP 与端口（默认 36666），或扫描电脑端二维码
4. 🗺️ 连接成功后自动开启模拟定位，打开高德/谷歌等第三方地图即可看到飞机位置
5. 🪟 悬浮窗默认开启，可随时开关并调节透明度

## ❓ 常见问题

- 📡 局域网连接：手机与电脑需在同一网络；Windows 防火墙需放行 UDP 36666
- 🌍 公网连接：需电脑有公网 IPv6；开启“公网使用”后，按界面显示的 IPv6 地址连接，并在路由器或防火墙中手动放行端口
- 🐢 位置卡顿：适当调低电脑端刷新间隔（如 50ms）或调高手机端注入频率

## 📁 项目结构

```text
MSFS-SimGPStoAndroid
├─ Android   (安卓手机端，包名 com.msfs.simconnect.client)
└─ CPP
   └─ MSFSSimConnect   (电脑端 Qt 6 程序，C++ / Qt Widgets)
```

## 🔧 构建

### 📱 安卓端

需要 Android SDK 36 + JDK 17，在 `Android` 目录执行：

```text
gradlew assembleDebug
```

### 🖥️ 电脑端

电脑端使用 CMake + Qt 6（Core、Gui、Widgets、Network），不再需要 Visual Studio 工程文件。

#### 依赖

- CMake 3.21 或更新版本
- C++20 编译器（Linux：GCC / Clang；Windows：MSVC）
- Qt 6.4 或更新版本，安装 `Core`、`Gui`、`Widgets`、`Network` 模块

以 Debian / Ubuntu 为例：

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev
```

#### 配置与构建

从仓库根目录执行：

```bash
cmake -S CPP/MSFSSimConnect -B build/qt6 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/qt6
```

不使用 Ninja 时，删除 `-G Ninja` 即可。生成的程序为：

```text
build/qt6/MSFS-SimConnect
```

#### 运行

Linux / macOS：

```bash
./build/qt6/MSFS-SimConnect
```

Windows PowerShell：

```powershell
.\build\qt6\MSFS-SimConnect.exe
```

首次启动时，允许系统防火墙接收 UDP 36666（或你在界面中设置的端口）。如果出现 `The proxy type is invalid for this operation`，请重新构建并运行最新程序；UDP 服务已显式禁用 Qt 的 HTTP/SOCKS 代理。

#### MSFS 数据源（Windows）

在 Windows 上如需从本机 MSFS 读取数据，另需安装 SimConnect SDK，并在配置时指定 SDK 根目录：

```powershell
cmake -S CPP/MSFSSimConnect -B build/qt6 -G Ninja `
  -DSIMCONNECT_SDK_DIR="D:/MSFS SDK/SimConnect SDK"
cmake --build build/qt6
```

Linux 构建提供 Qt 界面和 UDP 服务；MSFS/SimConnect 本机数据源仍需要 Windows 与 SimConnect SDK。公网模式使用 IPv6 时，仍需在路由器或防火墙中手动放行对应 UDP 端口。



## 🙏 参考与致谢

- 模拟定位与悬浮窗思路参考 [ZCShou/GoGoGo（影梭）](https://github.com/ZCShou/GoGoGo)
- 本项目基于 [sunmutian88/msfs_map](https://github.com/sunmutian88/msfs_map) 修改而来

## 🍋 支持

觉得好就请我喝杯柠檬水吧～

![支付宝赞助码](IMG/zfb.jpg)

## 📄 许可证

[CC BY-NC-SA 4.0](LICENSE)，不得用于商业用途。
