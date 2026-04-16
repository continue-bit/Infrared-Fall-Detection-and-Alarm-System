项目笔记与 AI 代理使用指南

目的
- 帮助 AI 代理快速理解本仓库的整体结构、关键开发流程和可修改位置，以便进行代码导航、补丁生成与局部重构。

大局（Big picture）
- 本工程为 STM32H7 系列（STM32H723）基于 Keil/MDK 的固件工程，使用 STM32 HAL 驱动。主要产物为 `stm32h723_TFT_LCD.axf`，链接脚本 `stm32h723_TFT_LCD.sct`。
- 关键子系统：启动与系统时钟（startup_*、`system_stm32h7xx.*`）、HAL 外设封装（`stm32h7xx_hal_*.crf` / 对应源文件组）、外设驱动（`spi.*`, `gpio.*`）、LCD 初始化/驱动（`lcd_init.*`, `lcd.*`）。

关键文件/目录（经常查阅）
- 程序入口：`Core/Src/main.c` —— 系统初始化与主循环。
- LCD 驱动与初始化：`lcd_init.crf`、`lcd.crf`（对应的源文件组，在工程中搜索 `lcd_init`、`lcd` 的 `.c/.h` 文件）。
- SPI/GPIO 驱动：`spi.crf`、`gpio.crf`（修改通信逻辑请在这些模块对应的源文件中实现）。
- HAL 组件分组：以 `stm32h7xx_hal_*.crf` 为分组提示，避免直接修改 HAL 库实现，优先在项目层封装。
- Keil 工程文件：`stm32h723_TFT_LCD.uvprojx`（在 uVision 中打开/构建）、`DebugConfig/*.dbgconf`（调试配置）。

可修改与不可修改区域
- 可修改：`Core/Src` 下的应用代码、工程中的 `spi`/`lcd`/`gpio` 模块源文件、项目级别的驱动封装。
- 尽量避免修改：`stm32h7xx_hal_*`（HAL 库源）和由工具/厂商生成的外部库文件，除非明确需要修补。

构建与调试流程（可被直接采纳）
- 常用方式：在 Keil uVision 中打开 `stm32h723_TFT_LCD.uvprojx`，选择对应 Target，使用 Build / Rebuild。
- 调试：使用 `DebugConfig/*.dbgconf` 或 uVision 的调试器（ST-Link）直接启动调试会话。
- 产物：构建后产物位于工程根，例如 `stm32h723_TFT_LCD.axf`，可通过 uVision 导出或使用 Keil 命令行工具在 CI 中构建（当前仓库未发现现成 CI 脚本 — 如需 CI，请提供首选的命令行构建方式）。

代码风格与约定（项目特有）
- 文件分组以 `.crf` 命名为线索，`.crf` 中声明的模块通常在项目根或 `Core/Src` 下有对应 `.c/.h`。
- 外设初始化遵循 STM32Cube HAL 模式：`MX_<PERIPH>_Init()` 在 `main.c` 或相关 init 文件中调用，驱动级别功能放在 `spi` / `lcd` 模块内。
- 时钟与系统配置放在 `system_stm32h7xx.*` 与 startup 文件中，谨慎改动以避免全局时序问题。

典型任务示例（供 AI 参考）
- 添加新的 LCD 命令：在 `lcd` 模块内新增函数，保持 HAL SPI 调用集中在 `spi` 模块，通过头文件在 `main.c` 中调用初始化顺序（参考 `lcd_init.*`）。
- 修复显示刷新错误：先阅读 `lcd_init` 和 `spi` 的实现，查找帧缓冲和 DMA 使用点；优先在驱动层添加断言和状态日志。

集成点与外部依赖
- 依赖：STM32 HAL（仓库内以 `stm32h7xx_hal_*` 分组体现）、Keil MDK/RTT（RTE 目录包含 `RTE_Components.h` 提示使用 RTE 组件）。
- 链接脚本：`stm32h723_TFT_LCD.sct`（影响内存布局和启动）。

限制与未发现项（需要人工确认）
- 未在仓库中发现 CI / make 脚本或 README 指南：如果希望自动化构建/验证，请提供首选的命令行构建流程或 CI 配置样例。
- 未找到现成的单元测试或硬件抽象的模拟层；硬件交互相关修改建议在真实板子或仿真器上验证。

修订与协作建议
- 当修改初始化顺序或时钟配置时，先在 `main.c` 做小范围更改并在硬件上验证以防止全局失效。
- 如果需要我把本文件合并到仓库，请确认希望放置的路径（默认：仓库根的 `.github/copilot-instructions.md`）。

反馈
- 请告知是否需要补充：CI 命令、常用调试步骤（例如具体 ST-Link 命令）、或指向某些源文件的确切路径示例，我会基于反馈迭代该文件。
