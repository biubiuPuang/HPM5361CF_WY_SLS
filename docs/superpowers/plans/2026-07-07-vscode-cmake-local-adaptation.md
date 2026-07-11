# VSCode CMake Local Adaptation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Adapt the copied VSCode CMake Tools configuration so this HPM5361 project configures and builds on the user's current Windows machine.

**Architecture:** Keep the project source unchanged and only adapt workspace-level VSCode configuration. CMake Tools will be pinned to the user's local HPM SDK v1.11 environment, Ninja generator, existing `build` directory, board `hpm5361`, and build type `flash_xip`; IntelliSense will consume CMake-generated `compile_commands.json`.

**Tech Stack:** VSCode CMake Tools, Microsoft C/C++ extension, HPMicro HPM SDK v1.11, CMake, Ninja, RISC-V GCC, Cortex-Debug.

## Global Constraints

- Workspace root: `d:/_Project/HPM5361CF_WY_SLS_Project/MotorMonitorV1.0`.
- SDK environment root: `D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0`.
- `HPM_SDK_BASE`: `D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk`.
- RISC-V toolchain root: `D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win`.
- CMake executable: `D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/cmake/bin/cmake.exe`.
- Board: `hpm5361`.
- Build type: `flash_xip`.
- Generator: `Ninja`.
- Do not change application source files.
- This directory is not a git repository, so skip commit steps and report changed files instead.

---

## File Structure

- Modify `.vscode/settings.json`: Owns VSCode CMake Tools workspace configuration and existing editor/plugin preferences.
- Modify `.vscode/c_cpp_properties.json`: Owns Microsoft C/C++ IntelliSense fallback configuration. It should point to CMake's compile database instead of copied absolute include paths.
- Modify `.vscode/launch.json`: Owns Cortex-Debug launch configurations. Replace SDK v1.8 paths with SDK v1.11 paths while leaving unknown J-Link installation path for later confirmation if needed.
- Create/Update `docs/superpowers/specs/2026-07-07-vscode-cmake-local-adaptation-design.md`: Records the approved design.
- Create/Update `docs/superpowers/plans/2026-07-07-vscode-cmake-local-adaptation.md`: Records this implementation plan.

---

### Task 1: Adapt CMake Tools workspace settings

**Files:**
- Modify: `.vscode/settings.json`

**Interfaces:**
- Consumes: Existing PlantUML settings, `files.associations`, `cmake.defaultVariants` from `.vscode/settings.json`.
- Produces: CMake Tools settings used by VSCode and by the verification command: `cmake.cmakePath`, `cmake.generator`, `cmake.buildDirectory`, `cmake.configureEnvironment`, and `cmake.configureArgs`.

- [ ] **Step 1: Replace `.vscode/settings.json` with normalized JSON**

Write the file exactly with this content:

```json
{
    "markdown-preview-enhanced.plantumlServer": "https://www.plantuml.com/plantuml",
    "plantuml.server": "https://www.plantuml.com/plantuml",
    "plantuml.render": "PlantUMLServer",
    "markdown-preview-enhanced.previewTheme": "github-light.css",
    "plantuml.exportOutDir": "docs/diagrams",
    "plantuml.exportFormat": "png",
    "plantuml.diagramsRoot": "docs",
    "cmake.cmakePath": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/cmake/bin/cmake.exe",
    "cmake.generator": "Ninja",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureEnvironment": {
        "HPM_SDK_BASE": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk",
        "GNURISCV_TOOLCHAIN_PATH": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win",
        "PATH": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/ninja;D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin;${env:PATH}"
    },
    "cmake.configureArgs": [
        "-DBOARD_SEARCH_PATH=${workspaceFolder}/boards",
        "-DBOARD=hpm5361",
        "-DCMAKE_BUILD_TYPE=flash_xip",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ],
    "cmake.enabledOutputParsers": [
        "cmake"
    ],
    "cmake.defaultVariants": {
        "buildType": {
            "choices": {
                "flash_xip": {
                    "short": "flash_xip",
                    "long": "hpm flash xip",
                    "buildType": "flash_xip"
                }
            }
        }
    },
    "files.associations": {
        "*.C": "c",
        "board.h": "c",
        "hpm_clock_drv.h": "c",
        "hpm_common.h": "c",
        "hpm_debug_console.h": "c",
        "hpm_trgmmux_src.h": "c",
        "stdbool.h": "c",
        "ad9833.h": "c",
        "math.h": "c",
        "string.h": "c",
        "stdint.h": "c",
        "pinmux.h": "c",
        "hpm_gpio_drv.h": "c",
        "hpm_gpiom_drv.h": "c",
        "cps122.h": "c",
        "io_iic.h": "c",
        "hpm_adc16_drv.h": "c",
        "hpm_l1c_drv.h": "c",
        "hpm_dmav2_drv.h": "c",
        "hpm_uart_drv.h": "c",
        "hpm_dma_drv.h": "c",
        "hpm_dmav2_regs.h": "c",
        "hpm_interrupt.h": "c",
        "hpm_trgm_drv.h": "c",
        "drv_gpio.h": "c",
        "drv_pwm.h": "c",
        "hpm_pwm_drv.h": "c",
        "param_mgr.h": "c",
        "hpm_iomux.h": "c",
        "drv_motor_homing.h": "c",
        "stepper.h": "c",
        "drv_bldc.h": "c",
        "hpm_gptmr_drv.h": "c",
        "modbus_app.h": "c",
        "motor_ctrl.h": "c",
        "drv_timer.h": "c",
        "hpm_soc.h": "c",
        "hpm_pmp_drv.h": "c",
        "drv_rs485.h": "c",
        "hpm_dmamux_drv.h": "c",
        "drv_uart.h": "c",
        "hpm_i2c_drv.h": "c"
    },
    "files.encoding": "utf8",
    "cortex-debug.variableUseNaturalFormat": false,
    "C_Cpp.errorSquiggles": "enabled",
    "Codegeex.RepoIndex": true
}
```

- [ ] **Step 2: Validate JSON syntax**

Run:

```bash
python -m json.tool .vscode/settings.json >/dev/null
```

Expected: command exits with status `0` and no output.

- [ ] **Step 3: Check required SDK paths exist**

Run:

```bash
test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/cmake/bin/cmake.exe' && test -d 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk' && test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin/riscv32-unknown-elf-gcc.exe'
```

Expected: command exits with status `0` and no output.

---

### Task 2: Adapt C/C++ IntelliSense configuration

**Files:**
- Modify: `.vscode/c_cpp_properties.json`

**Interfaces:**
- Consumes: `build/compile_commands.json` generated by Task 1 + verification configure.
- Produces: Microsoft C/C++ extension configuration named `HPM` that uses the real CMake compile database.

- [ ] **Step 1: Replace `.vscode/c_cpp_properties.json`**

Write the file exactly with this content:

```json
{
    "configurations": [
        {
            "name": "HPM",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "compilerPath": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin/riscv32-unknown-elf-gcc.exe",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x86"
        }
    ],
    "version": 4
}
```

- [ ] **Step 2: Validate JSON syntax**

Run:

```bash
python -m json.tool .vscode/c_cpp_properties.json >/dev/null
```

Expected: command exits with status `0` and no output.

---

### Task 3: Replace old SDK paths in debug launch configurations

**Files:**
- Modify: `.vscode/launch.json`

**Interfaces:**
- Consumes: Existing Cortex-Debug launch configurations.
- Produces: Debug configurations whose SDK files point to HPM SDK v1.11 instead of HPM SDK v1.8.

- [ ] **Step 1: Replace `.vscode/launch.json`**

Write the file exactly with this content:

```jsonc
{
    // 使用 IntelliSense 了解相关属性。
    // 悬停以查看现有属性的描述。
    // 欲了解更多信息，请访问: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "HPM Debug",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/output/demo.elf",
            "request": "launch",
            "type": "cortex-debug",
            "device": "HPM5361",
            "servertype": "openocd",
            "configFiles": [
                "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/probes/jlink.cfg",
                "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/soc/hpm5300.cfg",
                "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/boards/hpm5300evk.cfg"
            ],
            "svdFile": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/soc/HPM5300/HPM5361_svd.xml",
            "runToEntryPoint": "main",
            "openOCDLaunchCommands": [
                "adapter speed 1000"
            ],
            "showDevDebugOutput": "raw",
            "rttConfig": {
                "enabled": true,
                "address": "auto",
                "clearSearch": false,
                "polling_interval": 20,
                "decoders": [
                    {
                        "label": "",
                        "port": 0,
                        "type": "console"
                    }
                ]
            }
        },
        {
            "name": "HPM Debug (JLink Simple)",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/output/demo.elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "jlink",
            "device": "HPM5361XEGX",
            "interface": "jtag",
            "serverpath": "D:/APPS/Tools/JLink_V828/JLinkGDBServerCL.exe",
            "svdFile": "D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/soc/HPM5300/HPM5361_svd.xml",
            "showDevDebugOutput": "vscode",
            "runToEntryPoint": "main",
            "rttConfig": {
                "enabled": true,
                "address": "auto",
                "decoders": [
                    {
                        "label": "RTT Console",
                        "port": 0,
                        "type": "console"
                    }
                ]
            }
        }
    ]
}
```

- [ ] **Step 2: Validate JSONC-compatible syntax as JSON after comment stripping is not required**

Run this lightweight parse check with Python's `json` after removing leading `//` comments:

```bash
python - <<'PY'
import json, pathlib, re
p = pathlib.Path('.vscode/launch.json')
text = p.read_text(encoding='utf-8')
text = re.sub(r'(?m)^\s*//.*$', '', text)
json.loads(text)
PY
```

Expected: command exits with status `0` and no output.

- [ ] **Step 3: Check SDK debug files exist**

Run:

```bash
test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/probes/jlink.cfg' && test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/soc/hpm5300.cfg' && test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/boards/openocd/boards/hpm5300evk.cfg' && test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk/soc/HPM5300/HPM5361_svd.xml'
```

Expected: command exits with status `0` and no output. If this fails, inspect the SDK `boards/openocd` and `soc/HPM5300` directory names before changing the launch config.

---

### Task 4: Verify configure and build from the adapted environment

**Files:**
- Read/Generated: `build/CMakeCache.txt`
- Generated: `build/compile_commands.json`
- Generated: `build/output/demo.elf`

**Interfaces:**
- Consumes: Modified VSCode configuration from Tasks 1-3.
- Produces: Evidence that CMake configure/build works or exact failure output for follow-up debugging.

- [ ] **Step 1: Check Ninja exists in SDK tools**

Run:

```bash
test -f 'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/ninja/ninja.exe'
```

Expected: command exits with status `0` and no output. If it fails, search under the SDK environment for `ninja.exe` and adjust `PATH` in `.vscode/settings.json` before continuing.

- [ ] **Step 2: Configure with the same values VSCode CMake Tools will use**

Run:

```bash
export HPM_SDK_BASE='D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk'
export GNURISCV_TOOLCHAIN_PATH='D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win'
export PATH='D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/ninja:D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin:'"$PATH"
'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/cmake/bin/cmake.exe' -S . -B build -G Ninja -DBOARD_SEARCH_PATH='d:/_Project/HPM5361CF_WY_SLS_Project/MotorMonitorV1.0/boards' -DBOARD=hpm5361 -DCMAKE_BUILD_TYPE=flash_xip -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Expected: output includes `-- Configuring done` and `-- Generating done`, and the command exits with status `0`.

- [ ] **Step 3: Build the default target**

Run:

```bash
export PATH='D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/ninja:D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin:'"$PATH"
'D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/tools/cmake/bin/cmake.exe' --build build
```

Expected: command exits with status `0` and produces `build/output/demo.elf`.

- [ ] **Step 4: Verify generated files**

Run:

```bash
test -f build/compile_commands.json && test -f build/output/demo.elf
```

Expected: command exits with status `0` and no output.

- [ ] **Step 5: Report final status**

Report exactly:

- Modified files: `.vscode/settings.json`, `.vscode/c_cpp_properties.json`, `.vscode/launch.json`.
- Whether configure passed.
- Whether build passed.
- Whether `build/output/demo.elf` exists.
- Whether J-Link `serverpath` still needs user confirmation.

---

## Self-Review

- Spec coverage: Tasks 1-3 cover workspace CMake, IntelliSense, and debug-path adaptation. Task 4 covers configure/build verification and generated ELF evidence.
- Placeholder scan: No placeholders remain; all paths and commands are concrete.
- Type/property consistency: VSCode setting keys and JSON property names match the files being modified.
