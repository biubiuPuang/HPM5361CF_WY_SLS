# Python-C_Second-Time 双踏板库 HPM5361 移植总结与排查报告

## 1. 移植目标

本次修改目标是让 `Python-C_Second-Time` 双踏板控制库在 HPM5361 工程中编译并对接板载第二路 RS485。

原则：

- 上层双踏板接口尽量不变。
- `main.c` 仍按原方式调用 `pedal_controller_default_config("RS485_CH2")`、`pedal_controller_init()`、`pedal_controller_open()`、`pedal_controller_retract_to_safe()`。
- 不把 Windows 串口实现 `rs485_transport_win32.c` 编入 HPM 固件。
- 新增 HPM 专用 RS485 transport，把 `"RS485_CH2"` 映射到板子的 `RS485_CH2`。

## 2. 修改文件清单

### HPM RS485 BSP

- `src/bsp/drv_rs485.h`
- `src/bsp/drv_rs485.c`

主要修改：

- 新增 `rs485_init_channel()`，支持按通道和波特率初始化。
- 新增 `rs485_rx_available()`。
- 新增 `rs485_rx_read()`。
- 新增 `rs485_rx_flush()`。
- 新增 `rs485_wait_rx_available()`。
- 保留 `old_rs485_init()`，兼容原 `main.c` 初始化调用。
- `app_rs485_poll()` 只处理 `RS485_CH1`，不再读取/回发 `RS485_CH2`。

### 双踏板库

- `Python-C_Second-Time/include/pedal_platform.h`
- `Python-C_Second-Time/src/pedal_platform_hpm.c`
- `Python-C_Second-Time/src/pedal_platform_win32.c`
- `Python-C_Second-Time/src/pedal_controller.c`
- `Python-C_Second-Time/src/rs485_transport_hpm.c`
- `Python-C_Second-Time/build_mingw.bat`

主要修改：

- 新增平台时间接口 `pedal_platform_sleep_ms()` / `pedal_platform_now_ms()`。
- `pedal_controller.c` 不再直接包含 `windows.h`，改为调用平台接口。
- 新增 HPM 版 `rs485_transport_hpm.c`，实现 `rs485_transport.h` 声明的全部接口。
- Windows 构建脚本改为加入 `pedal_platform_win32.c`。

### 构建和主流程

- `CMakeLists.txt`
- `src/app/main.c`

主要修改：

- 增加 `PEDAL_PLATFORM_HPM=1` 编译宏。
- HPM 固件加入：
  - `Python-C_Second-Time/src/pedal_platform_hpm.c`
  - `Python-C_Second-Time/src/rs485_transport_hpm.c`
- HPM 固件不加入：
  - `Python-C_Second-Time/src/rs485_transport_win32.c`
- `MOTOR_TIMEOUT_THRESHOLD` 从 `5000` 改为 `1000`，对应需求中的 1 秒无总线数据进入异常。
- 恢复外部控制前调用 `safety_close_pedal_controller()`，释放双踏板内部控制状态。

## 3. 保持不变的上层接口

以下接口没有修改签名：

```c
PedalControllerConfig pedal_controller_default_config(const char *port_name);
PedalResult pedal_controller_init(PedalController *controller, const PedalControllerConfig *config);
PedalResult pedal_controller_open(PedalController *controller);
void pedal_controller_close(PedalController *controller);
PedalResult pedal_controller_retract_to_safe(PedalController *controller, int wait);
PedalResult pedal_controller_stop_all(PedalController *controller);
```

`main.c` 中仍可这样使用：

```c
g_pedal_config = pedal_controller_default_config("RS485_CH2");
result = pedal_controller_init(&g_pedal_controller, &g_pedal_config);
result = pedal_controller_open(&g_pedal_controller);
result = pedal_controller_retract_to_safe(&g_pedal_controller, 1);
```

## 4. HPM transport 工作方式

HPM 版 transport 文件：

```text
Python-C_Second-Time/src/rs485_transport_hpm.c
```

它实现了：

```c
void rs485_init(RS485Transport *transport);
PedalResult rs485_open(RS485Transport *transport, const RS485Config *config);
void rs485_close(RS485Transport *transport);
int rs485_is_open(const RS485Transport *transport);
uint16_t modbus_crc16(const uint8_t *data, size_t len);
PedalResult rs485_read_holding_registers(...);
PedalResult rs485_write_single_register(...);
PedalResult rs485_write_position_32(...);
```

关键逻辑：

- `rs485_open()` 识别 `"RS485_CH1"` / `"RS485_CH2"`。
- 当前双踏板配置传入 `"RS485_CH2"`，最终映射到 HPM 工程里的 `RS485_CH2`。
- 发送使用 `rs485_send_data()`。
- 接收使用 `rs485_rx_available()`、`rs485_rx_read()`、`rs485_rx_flush()`。
- 保留 Modbus CRC、功能码、异常响应、响应长度校验。

## 5. CH1 / CH2 分工

### RS485_CH1

用于外部总线监听。

`app_rs485_poll()` 现在只处理 CH1：

- 收到外部数据后清 `monitor_timeout`。
- 清 `busTimeoutFlag`。
- 用于判断外部总线是否恢复。

### RS485_CH2

用于双踏板 Modbus 控制。

`app_rs485_poll()` 不再读取 CH2，避免：

- 抢走电机驱动器返回帧。
- 把 Modbus 响应误当普通数据 echo 回去。
- 导致 `PEDAL_ERR_TIMEOUT` / `PEDAL_ERR_RESPONSE_MISMATCH` / CRC 错误。

## 6. 安全状态机变化

`main.c` 中：

- `MOTOR_TIMEOUT_THRESHOLD` 改为 `1000`，1ms 定时器下等于 1 秒。
- 进入 `STATE_SAFETY` 后吸合继电器，并调用 `safety_retract_pedal_once_or_retry()`。
- 回退成功后 `g_motor_at_origin = true`。
- 只有满足以下条件才恢复外部控制：
  - 急停释放。
  - 外部总线恢复。
  - 双踏板已回安全位置。
- 恢复前先调用 `safety_close_pedal_controller()`，然后断开继电器并回到 `STATE_NORMAL`。

## 7. 常见报错和排查方法

### 7.1 undefined reference to `pedal_controller_xxx`

原因：`pedal_controller.c` 没有编进工程。

检查 `CMakeLists.txt` 是否包含：

```cmake
Python-C_Second-Time/src/pedal_controller.c
```

### 7.2 undefined reference to `rs485_open` / `rs485_read_holding_registers`

原因：HPM transport 没有编进工程。

检查 `CMakeLists.txt` 是否包含：

```cmake
Python-C_Second-Time/src/rs485_transport_hpm.c
```

同时确认没有误用：

```cmake
Python-C_Second-Time/src/rs485_transport_win32.c
```

### 7.3 `windows.h` 找不到

原因：Windows 文件被编入 HPM 固件，或者控制层直接依赖 Windows。

HPM 固件不应编译：

```text
Python-C_Second-Time/src/rs485_transport_win32.c
Python-C_Second-Time/src/pedal_platform_win32.c
```

### 7.4 `PEDAL_ERR_TIMEOUT`

可能原因：

- RS485_CH2 接线不对。
- 电机驱动器波特率和 `g_pedal_config.baudrate` 不一致。
- 电机从站 ID 和 `accelerator_pedal_id` / `brake_pedal_id` 不一致。
- CH2 被其他逻辑读取。
- DE 收发切换时序过早。
- RX idle 阈值不合适。

优先检查：

1. `app_rs485_poll()` 是否仍处理 CH2。
2. `RS485_2_UART_BAUDRATE` 和驱动器实际波特率。
3. 逻辑分析仪查看 CH2 是否有请求和响应。

### 7.5 `PEDAL_ERR_MODBUS_CRC`

可能原因：

- 响应帧被截断。
- 波特率错误。
- 收到了旧帧残留。
- RX idle 阈值导致帧分段。

优先检查：

- `rs485_rx_flush()` 是否在发送前执行。
- 逻辑分析仪查看完整响应帧。
- `RS485_IDLE_THRESHOLD` 是否需要现场调试。

### 7.6 `PEDAL_ERR_RESPONSE_MISMATCH`

可能原因：

- 从站 ID 不一致。
- 功能码不一致。
- CH2 收到的不是当前请求的响应。
- 响应被 CH2 echo 或其他逻辑干扰。

## 8. Windows 依赖检查结果

静态搜索项：

```text
windows.h
Sleep(
GetTickCount
DWORD
HANDLE
CreateFileA
ReadFile
WriteFile
CRITICAL_SECTION
```

当前结果：

- HPM 固件编译文件中未发现 Windows-only API。
- Windows-only API 只保留在：
  - `Python-C_Second-Time/src/rs485_transport_win32.c`
  - `Python-C_Second-Time/src/pedal_platform_win32.c`

这是预期结果。

## 9. 构建验证结果

已使用 HPM SDK 环境变量执行构建：

```bash
cmake --build d:/_Project/HPM5361CF_WY_SLS_Project/MotorMonitorV1.0/build --config Debug --target all --
```

结果：

- 固件链接成功。
- 生成 `output/demo.elf`。
- 本次双踏板相关未出现链接错误。
- `rs485_transport_hpm.c` 和 `pedal_platform_hpm.c` 已参与编译。

仍存在一些原工程已有警告，例如：

- `drv_relay.h` 中 `BOARD_BUTTON_GPIO_PORT` 重定义。
- `board.c` 中部分函数隐式声明。
- `drv_mcan.c` 中 const 参数警告。

这些不是本次双踏板 HPM transport 移植引入的阻塞问题。

## 10. 遗留风险

1. `rs485_send_data()` 当前等待 DMA TX 完成后切回 RX。DMA 完成不一定等于 UART 最后一位已经发完。若现场偶发无响应，可在 TX 完成后增加 0.5~1ms 延时，或改为等待 UART 发送空闲标志。
2. `RS485_IDLE_THRESHOLD = 40U` 是否适合 38400 baud 需要现场验证。
3. `pedal_controller_retract_to_safe(..., 1)` 是阻塞式等待，最长默认 12 秒；符合“必须回原点”的安全需求，但期间主循环不会快速处理其他逻辑。
4. IDS830ABS 的实际波特率、地址 ID、回退位置、软限位必须现场确认。
5. `safety_close_pedal_controller()` 会先 `stop_all`，而 `pedal_controller_close()` 内部也会 stop，一般安全但可能多发一次停止命令。

## 11. 后续现场测试建议

1. 先只测 RS485_CH2 单独读取驱动器状态。
2. 再测 `pedal_controller_open()` 是否返回 `PEDAL_OK`。
3. 再测 `pedal_controller_retract_to_safe(..., 1)` 是否实际回退。
4. 最后再联调继电器和 CH1 外部总线恢复逻辑。
5. 每次失败优先记录 `g_pedal_last_result`，根据错误码按本报告第 7 节排查。
