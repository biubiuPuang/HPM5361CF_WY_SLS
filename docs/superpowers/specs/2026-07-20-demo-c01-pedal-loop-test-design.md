# DemoC01 双踏板前进回退循环测试设计

## 结论

在 `src/app/main.c` 中写一份硬件测试程序，用 `python-C_three-times/Demo-C_01.c` 和 `python-C_three-times/Demo-C_01.h` 暴露的函数控制双踏板。测试流程为：初始化系统和 RS485_CH2，吸合继电器切到本板控制，双踏板回零，然后无限循环“前进 5mm → 等待 → 回退 5mm → 等待”。

## 目标

- 验证双踏板能被本板通过 RS485_CH2 控制。
- 控制动作必须全部调用 `Demo-C_01.c/.h` 中已有接口。
- 不使用 `RS485_DoubleKeepReturn_Run()`。
- 不在 `main.c` 中手写 Modbus 寄存器控制。

## 关键依据

- `python-C_three-times/Demo-C_01.h` 已提供：
  - `DemoC01_GetDefaultConfig()`
  - `DemoC01_InitWithConfig()`
  - `DemoC01_HomeBoth()`
  - `DemoC01_MoveBothForwardMm()`
  - `DemoC01_MoveBothBackwardMm()`
  - `DemoC01_SafeCleanup()`
- `src/bsp/drv_rs485.h` 中 RS485_CH2 对应 UART2：`RS485_2_UART`。
- `Demo-C_01.c` 内部直接使用 HPM SDK UART 字节收发函数，所以 `main.c` 需要把 UART2 初始化为 38400 8N1 后传给 DemoC01。

## 测试流程

1. 调用 `board_init()` 完成板级初始化。
2. 初始化 LED、继电器、系统 1ms 定时器。
3. 初始化 RS485_CH2 / UART2，波特率 38400。
4. 调用 `relay_set_all(true)`，让本板接管双踏板通信。
5. 获取 DemoC01 默认配置。
6. 设置测试用安全参数：
   - `move_max_rpm = 300`
   - 其它参数先使用默认值。
7. 调用 `DemoC01_InitWithConfig(&ctx, RS485_2_UART, &config)`。
8. 调用 `DemoC01_HomeBoth(&ctx)`。
9. 进入无限循环：
   - `DemoC01_MoveBothForwardMm(&ctx, 5.0f)`
   - `board_delay_ms(1000)`
   - `DemoC01_MoveBothBackwardMm(&ctx, 5.0f)`
   - `board_delay_ms(1000)`
10. 任意步骤失败时：
    - 调用 `DemoC01_SafeCleanup(&ctx)`。
    - LED 快闪提示错误。

## 错误处理

- 初始化失败：LED 慢闪。
- DemoC01 控制失败：先安全清理，再 LED 快闪。
- 测试代码只负责硬件验证，不恢复原监听状态机。

## 不做的内容

- 不实现主机监听超时逻辑。
- 不实现异常状态机。
- 不使用 `RS485_DoubleKeepReturn_*` 控制函数。
- 不新增底层 Modbus 控制代码。

## 自检

- 没有 TBD/TODO。
- 流程只服务“验证双踏板能否被控制”。
- 控制动作只来自 `Demo-C_01.c/.h`。
- main.c 只负责初始化、继电器切换、调用 DemoC01 API、错误提示。
