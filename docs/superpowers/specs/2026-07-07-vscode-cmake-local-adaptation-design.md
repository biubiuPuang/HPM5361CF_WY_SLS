# VSCode CMake 本机适配设计

## 背景

当前工程和 VSCode 配置来自其他电脑，路径仍指向原环境，例如 `D:/APPS/EDA/sdk_env_v1.8.0` 和 `E:/CodeProjectFiles/...`。用户当前电脑上的 HPM SDK 环境根目录为：

```text
D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0
```

已确认该目录下存在：

- `hpm_sdk/cmake/hpm-sdk-config.cmake`
- `hpm_sdk/cmake/toolchain.cmake`
- `toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin/riscv32-unknown-elf-gcc.exe`
- `tools/cmake/bin/cmake.exe`

因此 `HPM_SDK_BASE` 应设置为：

```text
D:/1_Sofrware/HPM5300_SoftWare/sdk_env_v1.11_backups/sdk_env_v1.11.0/hpm_sdk
```

## 目标

让用户在当前 Windows 电脑上打开该工程后，VSCode 的 CMake Tools 能直接执行 Configure / Build，并尽量修正 IntelliSense 与调试配置中的旧路径。

## 方案

采用“直接写入项目 `.vscode` 配置”的方式适配当前电脑路径。该方式不追求跨电脑通用性，优先保证当前用户本机快速可用。

## 配置边界

### `.vscode/settings.json`

补齐 CMake Tools 所需的关键配置：

- `cmake.cmakePath` 指向 SDK 环境自带 CMake。
- `cmake.generator` 固定为 `Ninja`。
- `cmake.buildDirectory` 固定为 `${workspaceFolder}/build`。
- `cmake.configureEnvironment` 设置：
  - `HPM_SDK_BASE`
  - `GNURISCV_TOOLCHAIN_PATH`
- `cmake.configureArgs` 保留并扩展：
  - `-DBOARD_SEARCH_PATH=${workspaceFolder}/boards`
  - `-DBOARD=hpm5361`
  - `-DCMAKE_BUILD_TYPE=flash_xip`
  - `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`

保留原有 PlantUML、文件关联和 CMake variant 设置。

### `.vscode/c_cpp_properties.json`

替换来自旧电脑的手写 include path 和 compiler path。新的配置应：

- 使用 `${workspaceFolder}/build/compile_commands.json` 作为 IntelliSense 来源。
- 使用本机 SDK 环境下的 RISC-V GCC 作为 `compilerPath`。
- 保留 C11 / C++17 标准。
- 将 IntelliSense 模式设置为 GCC 交叉编译更合适的模式。

### `.vscode/launch.json`

将 SDK 相关路径从旧版本 SDK 替换为当前 SDK v1.11：

- OpenOCD config files
- SVD 文件

J-Link `serverpath` 暂不强行替换为未确认路径。如果当前路径不存在，后续根据用户本机 J-Link 安装位置继续适配调试。

## 验证

修改后执行等价于 VSCode CMake Tools 的命令验证：

1. 使用 SDK 自带 CMake 和 Ninja 重新 configure。
2. 尝试 build。
3. 确认是否生成 `build/output/demo.elf`。

如果 configure/build 失败，根据实际错误继续调整配置；不能只修改文件就声称完成。
