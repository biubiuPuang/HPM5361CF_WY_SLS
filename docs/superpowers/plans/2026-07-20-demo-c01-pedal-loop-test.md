# DemoC01 Pedal Loop Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a temporary firmware test in `src/app/main.c` that controls both pedals through `python-C_three-times/Demo-C_01.c/.h` and loops “forward 5mm, then backward 5mm”.

**Architecture:** `main.c` only performs board initialization, RS485_CH2 UART2 setup, relay switching, DemoC01 API calls, and LED error indication. All pedal control actions are delegated to `Demo-C_01.c/.h`; no Modbus register control is written in `main.c`. `CMakeLists.txt` is updated so the active build uses `python-C_three-times/Demo-C_01.c` instead of the deleted `Python-C_Second-Time` sources.

**Tech Stack:** HPM5361, HPM SDK UART driver, current project BSP LED/relay/RS485 headers, `python-C_three-times/Demo-C_01.c/.h`, CMake/HPM SDK build.

## Global Constraints

- Every user-facing response must begin with “你好,我是克劳德,我已经阅读完次工程的CLAUDE.md文件”.
- Do not read `D:\_Project\HPM5361CF_WY_SLS_Project\多版本合并资料`.
- Do not read `D:\_Project\HPM5361CF_WY_SLS_Project\MotorMonitorV1.0\移植之前的文件`.
- Pedal control code must use only `python-C_three-times/Demo-C_01.c` and `python-C_three-times/Demo-C_01.h`.
- Do not use `RS485_DoubleKeepReturn_Run()` for this test.
- Do not hand-write Modbus register control in `main.c`.
- When editing `src/app/main.c`, keep the existing commented-out content and append the new test code after it.
- Use RS485_CH2 / UART2 at 38400 baud.
- Test action is an infinite loop: forward 5.0mm, wait 1000ms, backward 5.0mm, wait 1000ms.
- Do not create git commits unless the user explicitly asks.

---

## File Structure

- Modify `CMakeLists.txt`
  - Responsibility: include the `python-C_three-times` header folder and compile `python-C_three-times/Demo-C_01.c`.
  - Remove active references to deleted `Python-C_Second-Time` source files so the build does not depend on the old folder.
- Modify `src/app/main.c`
  - Responsibility: temporary hardware test entry point.
  - Keep the existing commented-out original logic exactly as historical reference.
  - Append the new test code after the existing commented-out block.
  - It initializes board/BSP pieces, configures UART2 for direct byte polling used by DemoC01, switches the relay to local control, homes both pedals, and loops 5mm forward/backward.
  - It does not implement any pedal protocol details.

---

### Task 1: Build DemoC01 pedal loop test firmware

**Files:**
- Modify: `CMakeLists.txt:53-75`
- Modify: `src/app/main.c` by appending after the existing commented-out original logic
- Test: build command `cmake --build build`

**Interfaces:**
- Consumes:
  - `DemoC01_GetDefaultConfig(demo_c01_config_t *config)` from `python-C_three-times/Demo-C_01.h`
  - `DemoC01_InitWithConfig(demo_c01_context_t *ctx, UART_Type *uart, const demo_c01_config_t *config)` from `python-C_three-times/Demo-C_01.h`
  - `DemoC01_HomeBoth(demo_c01_context_t *ctx)` from `python-C_three-times/Demo-C_01.h`
  - `DemoC01_MoveBothForwardMm(demo_c01_context_t *ctx, float delta_mm)` from `python-C_three-times/Demo-C_01.h`
  - `DemoC01_MoveBothBackwardMm(demo_c01_context_t *ctx, float delta_mm)` from `python-C_three-times/Demo-C_01.h`
  - `DemoC01_SafeCleanup(demo_c01_context_t *ctx)` from `python-C_three-times/Demo-C_01.h`
  - `RS485_2_UART`, `RS485_2_UART_CLK_NAME`, `RS485_2_UART_BAUDRATE`, `RS485_CH2`, `RS485_MODE_TX`, `RS485_MODE_RX`, `rs485_set_mode()` from `src/bsp/drv_rs485.h`
- Produces:
  - A temporary `main(void)` firmware test that loops pedal forward/backward movement using only DemoC01 control APIs.
  - A build configuration that compiles `python-C_three-times/Demo-C_01.c`.

- [ ] **Step 1: Run the current build once to capture the baseline**

Run:

```bash
cmake --build build
```

Expected result before changes: the build may fail because `CMakeLists.txt` still references deleted `Python-C_Second-Time` files. If it fails for that reason, continue with Step 2. If it passes, continue with Step 2 and use the final build in Step 5 as the regression check.

- [ ] **Step 2: Replace old pedal source references in `CMakeLists.txt`**

In `CMakeLists.txt`, replace the old pedal include/source block:

```cmake
# 用户自定义添加_双踏板检测脚本
sdk_inc(Python-C_Second-Time)
sdk_inc(Python-C_Second-Time/build)
sdk_inc(Python-C_Second-Time/examples)
sdk_inc(Python-C_Second-Time/include)
sdk_inc(Python-C_Second-Time/src)
sdk_inc(Python-C_Second-Time/app)
```

with:

```cmake
# 用户自定义添加_双踏板 DemoC01 测试脚本
sdk_inc(python-C_three-times)
```

Then replace the old source append block:

```cmake
# 双踏板控制源码
list(APPEND SRC_FILES
    Python-C_Second-Time/src/pedal_controller.c
    Python-C_Second-Time/src/servo_axis.c
    Python-C_Second-Time/src/ids830abs_utils.c
    Python-C_Second-Time/src/pedal_platform_hpm.c
    Python-C_Second-Time/src/rs485_transport_hpm.c
    Python-C_Second-Time/app/TB_Two_Driver.c
)
```

with:

```cmake
# 双踏板 DemoC01 控制源码
list(APPEND SRC_FILES
    python-C_three-times/Demo-C_01.c
)
```

- [ ] **Step 3: Append the DemoC01 loop test after the commented original logic in `src/app/main.c`**

Do not delete or overwrite the existing commented-out original logic. Append this new test code after that block:

```c
/**
 * @file main.c
 * @brief DemoC01 双踏板前进/回退循环测试程序
 *
 * 测试目的：
 * 1. 通过 RS485_CH2 / UART2 接管双踏板。
 * 2. 调用 python-C_three-times/Demo-C_01.c/.h 的现成函数完成控制。
 * 3. 先回零，再一直循环“前进 5mm -> 回退 5mm”。
 */

#include <stdint.h>

#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_uart_drv.h"

#include "drv_led.h"
#include "drv_relay.h"
#include "drv_rs485.h"
#include "drv_timer_extra.h"

#include "Demo-C_01.h"

#define SYSTEM_TIMER_INTERVAL_MS      (1U)
#define PEDAL_TEST_STEP_MM            (5.0f)
#define PEDAL_TEST_DELAY_MS           (1000U)
#define PEDAL_TEST_MOVE_MAX_RPM       (300)
#define PEDAL_INIT_ERROR_BLINK_MS     (500U)
#define PEDAL_CONTROL_ERROR_BLINK_MS  (100U)

static demo_c01_context_t g_demo_ctx;

/**
 * @brief LED 永久闪烁提示错误
 *
 * led_toggle()：当前工程用户自定义 LED 翻转函数。
 * board_delay_ms()：HPM BSP 板级延时函数。
 */
static void app_error_blink_forever(uint32_t interval_ms)
{
    while (1)
    {
        led_toggle();
        board_delay_ms(interval_ms);
    }
}

/**
 * @brief DemoC01 发送前切换 RS485_CH2 到发送模式
 *
 * rs485_set_mode()：当前工程用户自定义 RS485 DE 方向控制函数。
 */
static void demo_c01_rs485_ch2_set_tx(void *user_data)
{
    (void)user_data;
    rs485_set_mode(RS485_CH2, RS485_MODE_TX);
}

/**
 * @brief DemoC01 发送完成后切换 RS485_CH2 到接收模式
 *
 * rs485_set_mode()：当前工程用户自定义 RS485 DE 方向控制函数。
 */
static void demo_c01_rs485_ch2_set_rx(void *user_data)
{
    (void)user_data;
    rs485_set_mode(RS485_CH2, RS485_MODE_RX);
}

/**
 * @brief 初始化 UART2 给 DemoC01 直接字节收发使用
 *
 * 注意：
 * Demo-C_01.c 内部直接调用 uart_send_byte() / uart_receive_byte()，
 * 所以这里不使用 drv_rs485.c 的 DMA 接收模式，避免 DMA 把接收字节取走。
 *
 * board_init_uart()：当前工程板级 UART 引脚和时钟初始化函数。
 * uart_default_config() / uart_init()：HPM SDK 官方 UART 配置函数。
 * gpio_set_pin_output()：HPM SDK 官方 GPIO 输出配置函数。
 * rs485_set_mode()：当前工程用户自定义 RS485 DE 方向控制函数。
 */
static hpm_stat_t demo_c01_uart2_polling_init(void)
{
    uart_config_t config = {0};

    board_init_uart(RS485_2_UART);

    gpio_set_pin_output(RS485_2_DE_GPIO, RS485_2_DE_PORT, RS485_2_DE_PIN);
    rs485_set_mode(RS485_CH2, RS485_MODE_RX);

    uart_default_config(RS485_2_UART, &config);
    config.fifo_enable = true;
    config.dma_enable = false;
    config.src_freq_in_hz = clock_get_frequency(RS485_2_UART_CLK_NAME);
    config.baudrate = RS485_2_UART_BAUDRATE;
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty;

    return uart_init(RS485_2_UART, &config);
}

/**
 * @brief 系统基础初始化
 *
 * board_init()：当前工程板级初始化函数。
 * init_led()：当前工程用户自定义 LED 初始化函数。
 * init_relay()：当前工程用户自定义继电器初始化函数。
 * system_timer_init()：当前工程用户自定义 1ms 系统计时初始化函数。
 */
static hpm_stat_t app_system_init(void)
{
    hpm_stat_t status;

    board_init();
    init_led();
    init_relay();

    status = system_timer_init(SYSTEM_TIMER_INTERVAL_MS);
    if (status != status_success)
    {
        return status;
    }

    status = demo_c01_uart2_polling_init();
    if (status != status_success)
    {
        return status;
    }

    return status_success;
}

/**
 * @brief 初始化 DemoC01 双踏板控制上下文
 *
 * DemoC01_GetDefaultConfig()：python-C_three-times 用户自定义默认配置函数。
 * DemoC01_InitWithConfig()：python-C_three-times 用户自定义初始化函数。
 */
static demo_c01_result_t pedal_test_demo_init(void)
{
    demo_c01_config_t config;

    DemoC01_GetDefaultConfig(&config);

    config.move_max_rpm = PEDAL_TEST_MOVE_MAX_RPM;
    config.direction_mode = DEMO_C01_RS485_CALLBACK_DIRECTION;
    config.set_tx_mode = demo_c01_rs485_ch2_set_tx;
    config.set_rx_mode = demo_c01_rs485_ch2_set_rx;
    config.direction_user_data = NULL;

    return DemoC01_InitWithConfig(&g_demo_ctx, RS485_2_UART, &config);
}

/**
 * @brief 执行一次“前进 5mm -> 回退 5mm”
 *
 * DemoC01_MoveBothForwardMm()：python-C_three-times 用户自定义双轴前进函数。
 * DemoC01_MoveBothBackwardMm()：python-C_three-times 用户自定义双轴后退函数。
 */
static demo_c01_result_t pedal_test_loop_once(void)
{
    demo_c01_result_t result;

    result = DemoC01_MoveBothForwardMm(&g_demo_ctx, PEDAL_TEST_STEP_MM);
    if (result != DEMO_C01_OK)
    {
        return result;
    }

    board_delay_ms(PEDAL_TEST_DELAY_MS);

    result = DemoC01_MoveBothBackwardMm(&g_demo_ctx, PEDAL_TEST_STEP_MM);
    if (result != DEMO_C01_OK)
    {
        return result;
    }

    board_delay_ms(PEDAL_TEST_DELAY_MS);

    return DEMO_C01_OK;
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    demo_c01_result_t result;

    if (app_system_init() != status_success)
    {
        app_error_blink_forever(PEDAL_INIT_ERROR_BLINK_MS);
    }

    set_led_state(BOARD_LED_ON);

    /* 吸合继电器，把双踏板通信链路切到本板 RS485_CH2。 */
    relay_set_all(true);
    board_delay_ms(200U);

    result = pedal_test_demo_init();
    if (result == DEMO_C01_OK)
    {
        result = DemoC01_HomeBoth(&g_demo_ctx);
    }

    if (result != DEMO_C01_OK)
    {
        DemoC01_SafeCleanup(&g_demo_ctx);
        app_error_blink_forever(PEDAL_CONTROL_ERROR_BLINK_MS);
    }

    while (1)
    {
        result = pedal_test_loop_once();
        if (result != DEMO_C01_OK)
        {
            DemoC01_SafeCleanup(&g_demo_ctx);
            app_error_blink_forever(PEDAL_CONTROL_ERROR_BLINK_MS);
        }

        led_toggle();
    }
}
```

- [ ] **Step 4: Check that `main.c` does not call the forbidden control path**

Run:

```bash
rg "RS485_DoubleKeepReturn|rs485_dkr_|REG_|MODBUS|0x03U|0x06U|0x10U" src/app/main.c
```

Expected result: no matches.

- [ ] **Step 5: Build the firmware**

Run:

```bash
cmake --build build
```

Expected result: exit code 0, with `main.c` and `python-C_three-times/Demo-C_01.c` compiled successfully. No errors about missing `Python-C_Second-Time` files.

- [ ] **Step 6: If the build reports a missing include path for `Demo-C_01.h`, verify CMake includes the exact lowercase folder**

Run:

```bash
rg "python-C_three-times|Python-C_Second-Time" CMakeLists.txt
```

Expected result:

```text
sdk_inc(python-C_three-times)
    python-C_three-times/Demo-C_01.c
```

There should be no active `Python-C_Second-Time` source path left in `CMakeLists.txt`.

- [ ] **Step 7: Manual hardware test procedure**

1. Power the board and double-check the relay wiring so `relay_set_all(true)` gives this board control of the pedals.
2. Flash the built firmware.
3. Confirm the LED does not fast-flash at startup.
4. Confirm both pedals home first.
5. Confirm both pedals repeat this motion: forward about 5mm, wait about 1 second, backward about 5mm, wait about 1 second.
6. If the LED fast-flashes, stop the test and treat it as a DemoC01 control failure. The firmware already calls `DemoC01_SafeCleanup()` before entering the fast-flash loop.

---

## Self-Review

- Spec coverage: the plan covers DemoC01-only control, RS485_CH2/UART2 at 38400, relay switch to local control, homing, repeated 5mm forward/backward movement, error cleanup, and CMake integration.
- Placeholder scan: no placeholder work remains.
- Type consistency: all DemoC01 function names and callback signatures match `python-C_three-times/Demo-C_01.h`.
- Scope check: one temporary hardware test firmware is a single implementation task.
