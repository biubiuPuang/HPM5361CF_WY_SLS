# IDS830ABS 双踏板接管控制层 C 语言移植版

本目录是从 Python 版 `PedalController` 思路移植出来的 C 语言接管控制层。

它面向你的任务需求中的这一段：

```text
你的监听/切换层已经判断主机 1 秒无通讯，
并且已经断开原主机与双踏板，
然后切换到你的备用控制器，
接下来需要立刻控制双踏板气缸收回到不再顶踏板的位置。
```

本 C 版本只负责“接管后怎么控制双踏板”，不负责监听和切换。

---

## 1. 功能范围

### 已实现

- Windows 串口 RS485 / Modbus RTU 主站。
- IDS830ABS 单轴寄存器封装。
- 双踏板控制器封装。
- 默认油门 ID=1、刹车 ID=2。
- 接管后双踏板标准初始化：
  - 清故障。
  - 切 PC 控制。
  - 开启通信超时保护。
- 双踏板位置模式绝对位置控制。
- 双踏板安全收回接口：

```c
pedal_controller_retract_to_safe(&controller, 1);
```

- 双踏板状态读取。
- 双踏板缓停。
- 双踏板软件急停。
- 软限位检查。
- 双轴位置差告警。
- 三个示例程序：
  - `retract_dual_pedals.exe`
  - `read_status.exe`
  - `emergency_stop.exe`

### 未实现

- 不实现原主机通讯监听。
- 不实现 1 秒无通讯判断。
- 不实现继电器/RS485 总线切换。
- 不实现 `home_all()`。
- 不实现力矩回零。
- 不实现速度+电流回零。
- 不实现找机械原点、清零、探端点。
- 不处理 `赛力斯踏板机器人.rar`。

也就是说，你的完整系统应该分成两层：

```text
你自己写的监听/切换层
    ↓
本 C 库：接管后控制双踏板收回
```

---

## 2. 安全警告

这个程序会直接控制真实运动硬件，调试前必须确认：

1. 原主机已经被你的硬件切换层断开。
2. 当前 RS485 总线上只有你的备用控制器在发控制命令。
3. 驱动器坐标系是可信的。
4. `safe_retract_cnt` 已经经过现场验证。
5. 双踏板周围没有人员和障碍物。
6. 硬件急停有效。
7. 软件急停不能替代硬件急停。

特别注意：

```text
本 C 库不做回零。
本 C 库直接发送绝对位置目标。
如果驱动器坐标不可信，直接发送绝对位置可能有危险。
```

如果发生过以下情况，必须重新确认坐标：

- 驱动器断电重启。
- 编码器/驱动器参数变化。
- 机械结构调整。
- 原主机没有完成过回零/标定。
- 当前反馈位置明显异常。

---

## 3. 坐标方向

本工程沿用 Python 项目的方向约定：

| cnt 方向 | 机械含义 |
|---|---|
| 负 cnt | 前进 / 伸出 / 顶踏板 |
| 正 cnt | 后退 / 缩回 / 离开踏板 |
| 0 cnt | 通常表示回零后建立的缩回端逻辑零点 |

所以：

```text
越负，越往前顶踏板。
越接近 0，越收回。
```

例如从：

```text
-100000 cnt → -500 cnt
```

这是收回/回待机。

但是 `-500` 本身仍然是一个负数，它通常表示“离开机械零点一点点的待机偏置”。它能不能作为你的安全收回位置，必须现场确认。

---

## 4. `safe_retract_cnt` 怎么理解

`safe_retract_cnt` 是你接管后希望双踏板气缸收回到的位置。

它对应你的业务语义：

```text
气缸收回来以后，不再顶踏板。
踏板虽然还能踩，但是气缸已经离开，不起作用。
```

它不是固定值，项目里也没有一个叫 `SAFE_RETRACT_CNT` 的现成常量。

你可以从下面两个候选值开始低速验证：

```text
0
-500
```

其中：

- `0`：更接近逻辑缩回端。
- `-500`：Python demo 中常见的 `STANDBY_CNT` / 待机偏置。

最终值必须以现场为准：

```text
移动到该位置后，气缸确实不再顶踏板；
不会撞机械限位；
双轴可以稳定重复到位；
反馈位置和电流正常。
```

---

## 5. 默认参数

| 参数 | 默认值 | 说明 |
|---|---:|---|
| 油门踏板 ID | 1 | accelerator |
| 刹车踏板 ID | 2 | brake |
| 波特率 | 38400 | 与 Python `PedalController` 默认一致 |
| 串口超时 | 500 ms | 读写超时 |
| 前进软限位 | -50000 cnt | 现场可改 |
| 后退软限位 | 50000 cnt | 现场可改 |
| `safe_retract_cnt` | 0 | 必须现场标定 |
| 双轴位置差告警 | 800 cnt | 超过则 `dual_sync_warning=1` |
| 到位位置容差 | 40 cnt | 判断到位 |
| 到位转速阈值 | 40 rpm | 判断停稳 |
| 到位保持时间 | 120 ms | 连续满足才算到位 |
| 运动超时 | 12000 ms | 超时返回错误 |
| 位置模式最大转速 | 3000 rpm | 可按现场调低 |
| 加速 | 1 × 100ms | 位置模式参数 |
| 减速 | 2 × 100ms | 位置模式参数 |
| 双轴目标下发间隔 | 80 ms | 与 Python 逻辑一致 |

---

## 6. 文件结构

```text
Python-C/
├── README.md
├── Makefile
├── build_mingw.bat
├── include/
│   ├── pedal_controller.h
│   ├── servo_axis.h
│   ├── rs485_transport.h
│   ├── ids830abs_registers.h
│   └── pedal_error.h
├── src/
│   ├── pedal_controller.c
│   ├── servo_axis.c
│   ├── rs485_transport_win32.c
│   └── ids830abs_utils.c
└── examples/
    ├── retract_dual_pedals.c
    ├── read_status.c
    └── emergency_stop.c
```

---

## 7. 编译

### 使用 MinGW / MSYS2

在 Windows 命令行中：

```bat
cd /d D:\_Project\赛力斯回退具体操作\Python-C
build_mingw.bat
```

或者在 Git Bash / MSYS2 中：

```bash
cd /d/_Project/赛力斯回退具体操作/Python-C
make
```

编译成功后会生成：

```text
build/retract_dual_pedals.exe
build/read_status.exe
build/emergency_stop.exe
```

---

## 8. 示例程序

### 8.1 读取状态

```bat
build\read_status.exe COM4
```

如果需要打印 Modbus TX/RX 帧：

```bat
build\read_status.exe COM4 --debug
```

注意：示例为了与接管控制层一致，调用 `pedal_controller_open()` 时会执行标准初始化寄存器写入，包括清故障、切 PC 控制、通信超时使能。

---

### 8.2 接管后收回双踏板

```bat
build\retract_dual_pedals.exe COM4 0
```

或者：

```bat
build\retract_dual_pedals.exe COM4 -500
```

参数含义：

```text
第 1 个参数：串口号
第 2 个参数：safe_retract_cnt
```

这个示例会：

1. 打开 RS485。
2. 初始化刹车和油门两个轴。
3. 读取初始状态。
4. 控制双踏板一起运动到 `safe_retract_cnt`。
5. 等待两轴到位。
6. 打印最终状态。
7. 关闭串口。

---

### 8.3 软件急停

```bat
build\emergency_stop.exe COM4
```

说明：

```text
这个程序只是通过 RS485 给驱动器发送软件急停命令。
它不能替代硬件急停。
如果通信断线，软件急停命令可能无法送达。
```

---

## 9. 二次开发最小调用示例

```c
#include "pedal_controller.h"

int main(void)
{
    PedalController controller;
    PedalControllerConfig config;
    PedalResult result;

    config = pedal_controller_default_config("COM4");

    /* 必须现场标定：0 或 -500 只是候选值 */
    config.safe_retract_cnt = 0;

    result = pedal_controller_init(&controller, &config);
    if (result != PEDAL_OK) {
        return 1;
    }

    result = pedal_controller_open(&controller);
    if (result != PEDAL_OK) {
        return 1;
    }

    /* 你的监听/切换层确认已经接管总线后，调用这一句 */
    result = pedal_controller_retract_to_safe(&controller, 1);

    pedal_controller_close(&controller);
    return result == PEDAL_OK ? 0 : 1;
}
```

推荐你的上层状态机这样调用：

```text
正常监听
  ↓
1 秒无主机通讯
  ↓
硬件切换到备用控制器
  ↓
pedal_controller_open()
  ↓
pedal_controller_retract_to_safe()
  ↓
继续监听主机恢复
  ↓
pedal_controller_stop_all()
pedal_controller_close()
  ↓
硬件切回原主机
```

---

## 10. 主要 API

### 默认配置

```c
PedalControllerConfig pedal_controller_default_config(const char *port_name);
```

### 初始化和打开

```c
PedalResult pedal_controller_init(PedalController *controller, const PedalControllerConfig *config);
PedalResult pedal_controller_open(PedalController *controller);
void pedal_controller_close(PedalController *controller);
```

### 控制双踏板

```c
PedalResult pedal_controller_move_all_absolute(PedalController *controller, int32_t target_cnt, int wait);
PedalResult pedal_controller_retract_to_safe(PedalController *controller, int wait);
```

`wait=1`：等待两轴到位后返回。  
`wait=0`：只下发命令，不等待到位。

### 停止

```c
PedalResult pedal_controller_stop_all(PedalController *controller);
PedalResult pedal_controller_emergency_stop_all(PedalController *controller);
```

`stop_all()` 会尽量执行：

```text
速度目标置 0
力矩目标置 0
缓冲停止
```

`emergency_stop_all()` 会发送驱动器急停寄存器命令。

### 读取状态

```c
PedalResult pedal_controller_status(PedalController *controller, PedalControllerStatus *out_status);
```

状态包含：

- 油门位置。
- 刹车位置。
- 电流。
- 转速。
- 状态字。
- 是否有故障。
- 双轴位置差。
- 双轴位置差告警。

---

## 11. Python 到 C 的对应关系

| Python | C |
|---|---|
| `RS485Transport.read_holding_registers()` | `rs485_read_holding_registers()` |
| `RS485Transport.write_single_register()` | `rs485_write_single_register()` |
| `RS485Transport.write_position_32()` | `rs485_write_position_32()` |
| `ServoAxisRS485.standard_init()` | `servo_axis_standard_init()` |
| `ServoAxisRS485.prepare_position_mode()` | `servo_axis_prepare_position_mode()` |
| `ServoAxisRS485.set_target_position_cnt()` | `servo_axis_set_target_position_cnt()` |
| `ServoAxisRS485.read_fast_status()` | `servo_axis_read_fast_status()` |
| `PedalController.open()` | `pedal_controller_open()` |
| `PedalController.move_all_absolute()` | `pedal_controller_move_all_absolute()` |
| `PedalController.stop_all()` | `pedal_controller_stop_all()` |
| `PedalController.status()` | `pedal_controller_status()` |

没有 C 对应项：

```text
PedalController.home_all()
TorqueHomingConfig
CurrentHomingConfig
```

原因：本次需求不是重新回零，而是接管后收回气缸。

---

## 12. Modbus/寄存器说明

本 C 库使用：

- `0x03`：读 holding registers。
- `0x06`：写单寄存器。
- `0x10`：写 32 位目标位置。

目标位置写入方式：

```text
从寄存器 0x0050 开始
一次写 2 个寄存器
共 4 字节
int32_t 补码
高字节在前
```

例如 `-500` 的 32 位补码是：

```text
0xFFFFFE0C
```

发送位置 payload 时顺序为：

```text
FF FF FE 0C
```

注意：Modbus CRC 在帧尾是低字节在前。

---

## 13. 常见问题

### 打不开 COM 口

可能原因：

- 串口号不对。
- USB-RS485 驱动未安装。
- 串口被其他程序占用。
- COM10 以上串口路径问题。

本库内部会把 `COM10` 转换成 `\\.\COM10`。

---

### 读状态超时

可能原因：

- 驱动器没上电。
- A/B 线接反。
- 波特率不一致。
- 从站 ID 不对。
- 原主机没有被断开，总线冲突。
- RS485 转换器异常。

---

### 运动方向不对

先停止测试，重新确认坐标方向。

当前项目约定：

```text
负 cnt = 前进/伸出/顶踏板
正 cnt = 后退/缩回/离开踏板
```

如果现场方向和这个相反，不能继续套用默认参数，必须先查驱动器/机械配置。

---

### 运动超时

可能原因：

- 目标不可达。
- 软限位或机械限位设置不合理。
- 速度/加速度过低。
- 某个轴故障。
- 坐标不可信。
- 两个踏板有一个没有执行运动。

---

### 双轴位置差告警

可能原因：

- 两个轴坐标不一致。
- 一个轴没有运动。
- 一个轴有故障。
- 机械阻力不同。
- 接管前状态已经不同步。

---

## 14. 建议现场验证顺序

1. 确认硬件急停有效。
2. 确认原主机已经由你的切换层断开。
3. 运行：

```bat
build\read_status.exe COM4
```

4. 确认两个轴都有合理位置和状态。
5. 选择一个非常保守的 `safe_retract_cnt`，例如 `0` 或 `-500`。
6. 低速、短行程测试：

```bat
build\retract_dual_pedals.exe COM4 -500
```

7. 观察是否真的不再顶踏板。
8. 多次重复验证。
9. 将最终值写入你的上层程序配置。

---

## 15. 重要结论

你后续二次开发时，只需要把本库当成这个模块：

```text
备用控制器接管后，让双踏板收回到安全位置的控制模块
```

不要把它当成：

```text
主机通讯监听模块
总线切换模块
回零模块
完整运动控制平台
```

推荐你的业务代码只依赖这个核心调用：

```c
pedal_controller_retract_to_safe(&controller, 1);
```
