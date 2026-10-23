# XF Swarm 二进制 SDK

闭源编队库 `xf_swarm` 与光电载荷对接库 `xf_eots` 的 C ABI 交付包：动态库、公开头文件、CMake 导入配置和调用示例。包版本 **1.0.0**（写在 `cmake/XFSwarmConfig.cmake` 的 `XFSwarm_VERSION`），C ABI **1**（`XF_ABI_VERSION`，也可在运行时用 `xf_swarm_abi_version()` / `xf_eots_abi_version()` 读取）。本仓库没有算法源码。

## 当前状态

**维护**。这是 2026-09-14 放入的二进制交付快照；当天之后没有新提交。

- 只有 `main`。全部历史是 2026-09-14 的两次提交：`6f66caf` 放入动态库、头文件、示例和说明；`7a86257` 改了一句 README。之后没有新提交，也没有其它分支。
- 算法不能在这里编译。Linux 上的 `.so` 已 strip；Windows DLL 的 file 信息是 stripped to external PDB，仓库里没有 PDB。
- 本次在 Linux x86-64 上复现了示例的配置、编译、`ctest` 和运行。Windows、MinGW、MSVC 的构建命令没有在本环境执行。

## 功能

| 库 | 作用 |
|---|---|
| `xf_swarm` | 队形配置、成员状态输入、领航位置与目标、周期控制输出、停止与复位。输出是 ENU 速度和偏航角速度，留在调用方缓冲里，由宿主检查权限后发送 |
| `xf_eots` | 把跟踪命令转给厂商回调，接收观测，维护一份可读取的载荷快照。检测模型、相机驱动、厂商 SDK 和网络传输由宿主提供 |

两个 `.so` 的动态依赖都不包含对方。`xf_eots.h` 通过 `#include "xf_swarm.h"` 使用其中的 `xf_result`、`XF_API` 和 `XF_CALL`。示例把两边分开调用，没有把像素框写进 `xf_swarm_push_state`。

公开导出与头文件一致，本次用 `nm -D` 核对过 Linux 动态符号：`xf_swarm` 11 个，`xf_eots` 6 个。Windows 的 `.def` 是同一组名字。

## 目录结构

```text
include/xf_swarm.h                 编队 C 接口与数据结构
include/xf_eots.h                  EOTS C 接口与数据结构
lib/linux-x86_64/xf_swarm.so       Linux x86-64 编队库，SONAME 为 xf_swarm.so
lib/linux-x86_64/xf_eots.so        Linux x86-64 EOTS 库，SONAME 为 xf_eots.so
lib/windows-x86_64/xf_swarm.dll    Windows x64 编队库
lib/windows-x86_64/xf_eots.dll     Windows x64 EOTS 库
lib/windows-x86_64/libxf_*.dll.a   MinGW 导入库
lib/windows-x86_64/xf_*.def        导出声明；MSVC 导入库不在仓库里
cmake/XFSwarmConfig.cmake          CMake 导入配置，包版本 1.0.0
examples/basic.c                   公开接口示例，使用写死的模拟输入
CMakeLists.txt                     只编译示例并注册 ctest，不编译 SDK
SHA256SUMS                         上述库、导入库和 .def 的校验值
notices/                           GCC runtime、GPL-3 文本、winpthreads 声明
```

`.gitignore` 忽略 `/build/` 和 `*.lib`。仓库没有 `docs/`、`test/`、`.github/` 或 LICENSE。

## 环境与依赖

没有 ROS 1 / ROS 2、PX4、ArduPilot、Gazebo 或其它仿真工程，也没有目标飞控、机载计算机或相机的构建文件。

平台限制写在 `cmake/XFSwarmConfig.cmake`：指针必须是 64 位，处理器名要匹配 `x86_64` / `AMD64` / `amd64`，系统只能是 Windows 或 Linux，否则配置直接 `FATAL_ERROR`。本次环境是 x86-64 Linux，失败分支没有触发。

Linux 动态依赖（`readelf -d`）：`libm.so.6`、`libc.so.6`、`ld-linux-x86-64.so.2`。没有 `libstdc++.so`。二进制字符串里的编译器是 `GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0`。`xf_swarm.so` 引用 `hypot@GLIBC_2.35`，`xf_eots.so` 最高到 `GLIBC_2.34`，所以加载编队库需要 glibc 2.35 或更新。本次系统是 glibc 2.39，可以加载。

Windows DLL 的导入表只有 `KERNEL32.dll` 和 `msvcrt.dll`。编译器字符串是 `GCC: (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r6) 15.2.0`。`libgcc` / `libstdc++` / winpthreads 的代码在 DLL 内部，不是额外要分发的 DLL。这条来自二进制检查，不表示本次运行过这些 DLL。

调用约定在头文件里：Windows 是 `__cdecl`，并从 C++ 使用 `extern "C"`。业务程序不要定义 `XF_BUILD`。结构体使用默认对齐，头文件里没有 `#pragma pack`。

第三方运行库说明在 `notices/README.md`、`notices/GCC-RUNTIME.txt`、`notices/GPL-3.txt`、`notices/winpthreads.txt`。这些文件写明它们不授予 SDK 专有实现的许可。仓库没有单独的 SDK 许可证。

本仓库没有 upstream remote，提交历史里也没有 PX4、ArduPilot 或 ego-planner 的版本标记。能对上名字的第三方只有上面这些随二进制附带的运行库。

## 构建与运行

需要 CMake 和 C99 编译器。`CMakeLists.txt` 的 `cmake_minimum_required` 是 3.16。本次实际使用 CMake 3.28.3，`ctest --help` 中有 `--test-dir`。3.16 到 3.27 能否使用同一条命令，本次未验证。

### Linux（本次已运行）

环境：x86-64，glibc 2.39，CMake 3.28.3，`cc` 为 Clang 18.1.3。本机没有 `ninja`，CMake 使用默认的 Unix Makefiles。在仓库根目录：

```bash
sha256sum -c SHA256SUMS
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/xf_example
```

`sha256sum -c SHA256SUMS` 的 8 个文件全部为 OK。`ctest` 只有一项 `usage_example`，通过。示例退出码 0，标准输出是：

```text
uav0 velocity=(1.000,-0.000,0.000), yaw_rate=0.000
demo EOTS action=0 mode=0
EOTS state=2 target=7
demo EOTS action=2 mode=0
```

`1.000` 与默认 `max_speed_mps` 一致。`state=2` 是 `XF_EOTS_STATE_TRACKING`，`target=7` 来自示例里写死的 `track_id`。示例不连接飞控或相机。`examples/basic.c` 里的 `demo_vendor` 直接返回 `XF_OK`，注释写明要换成真实厂商受理。

生成的 `xf_example` 带有指向源码树 `lib/linux-x86_64` 的绝对 `RUNPATH`（本次是配置时的仓库路径）。所以在原目录布局下可以直接运行。把 `build/` 或整个仓库挪走之后，需要自己设置 `LD_LIBRARY_PATH`，或者在业务工程里写 RPATH。`build/` 被 git 忽略。

同一套 `find_package` 也在仓库外的小工程里配置并链接通过，见下文「集成」。

### Windows / MinGW（未验证）

原说明中的命令与 `CMakeLists.txt` 的 Windows 分支一致：`POST_BUILD` 会把两个 DLL 复制到示例可执行文件旁边。本次没有 Windows 环境，下面命令未执行。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
.\build\xf_example.exe
```

`-G Ninja` 依赖已安装的 Ninja。本次 Linux 上未安装 Ninja，因此没有用这个生成器。

### MSVC 导入库（未验证）

仓库没有 `xf_swarm.lib` / `xf_eots.lib`，`.gitignore` 也忽略 `*.lib`。`cmake/XFSwarmConfig.cmake` 在 MSVC 下会检查这两个文件，不存在就 `FATAL_ERROR`。原说明给出的生成方式未在本环境执行：

```bat
lib /machine:x64 /def:lib\windows-x86_64\xf_swarm.def /out:lib\windows-x86_64\xf_swarm.lib
lib /machine:x64 /def:lib\windows-x86_64\xf_eots.def /out:lib\windows-x86_64\xf_eots.lib
```

`.def` 里的导出名也可以由调用方用 `LoadLibrary` / `GetProcAddress` 加载。回调须使用头文件中的 `XF_CALL`。这条加载方式未验证。

## 测试与仿真

测试只有 `CMakeLists.txt` 里的 `usage_example`，内容就是运行 `examples/basic.c`。没有单元测试、回放数据、Gazebo / 其它仿真，也没有真机或厂商联调脚本。

另用公开接口做过一次性探测（程序不在仓库里）。探测结果写在「开发说明」里，并标明测到哪一步。没有做并发、MSVC、ARM 或长时间停流测试。

## 开发说明

### 分支与入口

没有分支命名约定，远程只有 `main`。没有 CI。

| 入口 | 作用 |
|---|---|
| `include/xf_swarm.h` | 编队类型、错误码、11 个函数 |
| `include/xf_eots.h` | EOTS 常量、观测 / 快照、6 个函数 |
| `examples/basic.c` | 单机模拟输入的调用顺序 |
| `cmake/XFSwarmConfig.cmake` | 导入 `XF::swarm`、`XF::eots` |
| `CMakeLists.txt` | 示例工程，`include()` 上述配置 |

改算法或换 ABI 不能在本仓库完成，需要替换 `lib/` 下的二进制，并让头文件、`SHA256SUMS` 和 `XFSwarm_VERSION` 一起更新。

### 集成

本次在仓库外用下面的写法配置并链接了一个只检查 ABI 的程序。`PATHS` 使用本仓库 `cmake/` 的绝对路径，运行退出码为 0。Linux 可执行文件的 `RUNPATH` 同样指向本仓库的 `lib/linux-x86_64`。

```cmake
find_package(XFSwarm CONFIG REQUIRED
    PATHS "<仓库根目录>/cmake" NO_DEFAULT_PATH)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE XF::swarm XF::eots)
```

`CMake` 配置里两个目标可以分开链接。本次只编过同时链接 `XF::swarm` 和 `XF::eots` 的程序，没有再单独编只链接其中一个的目标。

### 配置

先 `xf_swarm_default_config`，再改字段。`struct_size == 0`、`abi_version == 2`、速度为 NaN 时，`xf_swarm_create` 返回 `XF_INVALID_ARGUMENT`。`xf_swarm_create(NULL, &handle)` 同样返回 `XF_INVALID_ARGUMENT`，且 handle 为 NULL。`xf_swarm_destroy(NULL)` 可以调用。

本次读到的默认值，以及实际调用过的接受 / 拒绝端点如下。端点之间的浮点数没有逐个测试。

| 字段 | 本次读到的默认值 | 本次接受 | 本次拒绝 |
|---|---|---|---|
| `control_period_sec` | 0.05 | 0.005，1 | 0.0049，1.0001 |
| `max_speed_mps` | 1 | 0.05，20 | 0.049，20.001 |
| `max_vertical_speed_mps` | 1 | 0，20 | -0.001，20.001 |
| `max_yaw_rate_rps` | 0.5 | 0，10 | -0.001，10.001 |
| `separation_m` | 2 | 0.1，100 | 0.099，100.001 |
| `telemetry_timeout_sec` | 1 | 0.001，1 | 0，1.001 |

原说明把 `separation_m` 描述为规划用的间距，并写明它不是物理防撞保证。本仓库没有碰撞仿真，防撞这一句未另行验证。

### 编队调用顺序

`examples/basic.c` 的顺序是：`default_config` → `create` → `set_formation` → `set_leader` → `set_goal` → `push_state` → `step` → EOTS 的 create / command / feed / read → `stop` → `destroy`。

成员与坐标的类型见 `xf_member`、`xf_leader_pose`、`xf_vehicle_state`。`id` 是 `XF_ID_SIZE`（64）字节的字符数组。原说明要求 NUL 结尾、有效内容最多 63 字节、全表不重复；63 与 64 字节的边界本次未复测。位置和速度在头文件里是 `xf_vec3`，姿态是 `xf_quaternion`，没有单位注释。原说明写的是共同原点 ENU、米、米/秒，以及机体 FLU 到 ENU 的四元数。把像素数值填进 `position` 会不会被拒绝，本次未测。

本次对成员表的调用结果：

- `count` 为 0 或 129：`XF_INVALID_ARGUMENT`。`count` 为 128 且第一名 `local == 1`：`XF_OK`。`XF_MAX_MEMBERS` 是 128。
- 唯一成员 `local == 0`：`XF_INVALID_ARGUMENT`。
- 两个成员使用同一个 id：`XF_INVALID_ARGUMENT`。
- 不在队形表里的 id 仍可以 `push_state`，本次返回 `XF_OK`。这只说明调用被接受，没有再检查它是否改变控制量。

### 本次核对过的编队行为

下面都是单线程、按顺序调用的结果。

- `confidence == 0.5` 的 `push_state` 返回 `XF_OK`，紧接着的 `step` 为 `XF_OK` 且 `diagnostic == XF_DIAG_OK`。`confidence == 0.499` 的 `push_state` 返回 `XF_INVALID_ARGUMENT`。
- 零四元数（`w == 0` 且其余为 0）的 `push_state` 返回 `XF_INVALID_ARGUMENT`。
- 在还没有任何有效观测时，`tracking_valid == 0` 或 `velocity_valid == 0` 的 `push_state` 返回 `XF_STALE`。紧接着的 `step` 返回 `XF_OK`，`diagnostic` 为 `XF_DIAG_INCOMPLETE_TELEMETRY`，速度为 0。已经有一帧有效观测之后再推这种帧，本次没有测。
- `step` 的容量为 0 时返回 `XF_BUFFER_TOO_SMALL`，`count` 为 1（当时只有一个 local 成员）。
- 用与上一拍相同或更早的时间再 `step`，返回 `XF_STALE`，`diagnostic` 为 `XF_DIAG_NOT_READY`，速度为 0。相邻两拍间隔正好 1.0 秒时返回 `XF_OK`；间隔 1.2 秒时返回 `XF_STALE`。之后用更新的采集时间和更新的 `step` 时间可以再次得到 `XF_OK`。1.0 秒和 1.2 秒之间的阈值没有再细分。
- `xf_swarm_stop` 返回 `XF_OK`。之后的 `step` 返回 `XF_STOPPED`，`diagnostic` 为 `XF_DIAG_STOPPED`，速度为 0。此时 `set_goal` 仍返回 `XF_OK`，但下一拍 `step` 依旧是 `XF_STOPPED`。
- `xf_swarm_reset` 返回 `XF_OK`。复位后不重新 `set_leader` 就 `step`，返回 `XF_NOT_READY`，`diagnostic` 为 `XF_DIAG_NOT_READY`。

头文件中的返回值：`XF_OK` 0，`XF_INVALID_ARGUMENT` 1，`XF_NOT_READY` 2，`XF_BUFFER_TOO_SMALL` 3，`XF_STALE` 4，`XF_CAPACITY` 5，`XF_STOPPED` 6，`XF_VENDOR_ERROR` 7，`XF_NO_MEMORY` 8，`XF_INTERNAL_ERROR` 9。诊断值：`XF_DIAG_OK` 0，`XF_DIAG_MISSING_POSE` 1，`XF_DIAG_MISSING_SLOT` 2，`XF_DIAG_INCOMPLETE_TELEMETRY` 3，`XF_DIAG_INVALID_INPUT` 4，`XF_DIAG_STOPPED` 5，`XF_DIAG_NOT_READY` 6。`XF_CAPACITY`、`XF_NO_MEMORY`、`XF_INTERNAL_ERROR`、`XF_DIAG_MISSING_POSE`、`XF_DIAG_MISSING_SLOT`、`XF_DIAG_INVALID_INPUT` 本次没有故意造出。

`step` 返回 `XF_OK` 时仍要看每架飞机的 `diagnostic`。上面几条打印过速度的失败路径都是 0。容量为 0 的那次按接口不能读取输出，本次没有读。控制结果留在调用方提供的数组里，库不会把它发给飞控。

原说明还要求：一个控制线程负责配置和 `step`，遥测回调里 `push_state`，`destroy` 不要与其它调用并发，时钟或原点变化后要 `reset`。并发与时钟重置本次未验证。

### EOTS

命令常量：`XF_EOTS_TRACK` / `CLEAR` / `STOP`，模式 `XF_EOTS_SOT` / `MOT`。事件：`APPEARED`、`UPDATED`、`LOCKED`、`LOST`、`RECOVERED`、`RESET`、`OVERRIDDEN`。状态：`IDLE`、`SEARCHING`、`TRACKING`、`LOST`、`RECOVERING`、`OVERRIDDEN`。数值与头文件顺序一致，从 0 递增。

`xf_eots_create(NULL, ...)` 返回 `XF_INVALID_ARGUMENT`，handle 为 NULL。`xf_eots_destroy(NULL)` 可以调用。

本次调用结果：

- 创建后、尚未 `command` 时，`read` 返回 `XF_OK`：`present == 1`，`enabled == 0`，`state == XF_EOTS_STATE_IDLE`，没有 primary / frame / fps。
- handler 返回 `XF_VENDOR_ERROR` 时，`command` 返回 `XF_VENDOR_ERROR`，随后 `feed` 返回 `XF_INVALID_ARGUMENT`。这时 `read` 仍是 `XF_OK` 的 IDLE 快照。
- handler 返回 `XF_OK` 时，`TRACK` + `SOT`（示例和本次探测）以及 `TRACK` + `MOT`（本次探测）都会在 `command` 返回前同步调用 handler。`TRACK` + `SOT` 发出之后、第一次成功 `feed` 之前，`read` 返回 `XF_NOT_READY`。不要把这次输出当成有效跟踪快照。`MOT` 这次只确认了 handler 被调用，没有在 `feed` 前再读一次快照。
- 按 `examples/basic.c` 的框（640×480 内、宽高大于 0、seq 为 1）`feed` 一帧 `LOCKED` 后，`read` 返回 `XF_OK`，`state == XF_EOTS_STATE_TRACKING`，`has_primary`、`has_frame`、`has_fps` 均为 1，`track_id` 为 7。使用 `MOT` 时快照里的 `mode` 为 1。
- 第一帧 `seq == 0` 本次被接受。重复上一帧的 `seq`，或 `capture_ms` 比上一帧小，`feed` 返回 `XF_INVALID_ARGUMENT`。`width == 0`，或框超出图像（x=600、width=50、图像宽 640），同样返回 `XF_INVALID_ARGUMENT`。
- `LOST` 且 `has_primary == 0` 的 `feed` 返回 `XF_OK`。随后快照 `state == XF_EOTS_STATE_LOST`，`primary.locked == 0`，但 `has_primary` 仍为 1，`track_id` 仍为 7。
- `STOP` 之后 `read` 为 `XF_OK`，`state == XF_EOTS_STATE_IDLE`。

`xf_eots_snapshot` 只有一个 `primary`。`MOT` 可以被转发，头文件里没有多目标数组。

原说明中的这些约束本次没有复测：停流超过 1 秒撤销锁定、`SEARCHING` / `RECOVERING` / `OVERRIDDEN` 的状态迁移、除 `LOST` 以外哪些事件允许 `has_primary == 0`、label 长度、置信度和归一化偏移的全部边界、新相机会话必须销毁旧句柄。回调线程和「handler 内不得重入或销毁同一实例」也未验证。

宿主若要让编队跟某个视觉目标，需要自己把目标变到与成员相同的 ENU 坐标，再调用 `xf_swarm_set_goal`。示例没有这条路径。

## 已知问题

- 没有算法源码、内部头文件、调试符号和算法测试，不能在本仓库重编或单步调试库内部。
- 没有飞控、厂商相机、ROS、Gazebo 或真机联调。示例里的厂商回调是空的成功返回。
- Windows / MinGW 构建、MSVC `lib.exe` 导入库、`LoadLibrary` 路径都未在本次验证。仓库不附带 `.lib` 和 PDB。
- 没有 CI，测试只有模拟输入的 `usage_example`。
- 没有 SDK 自己的许可证文件。`notices/` 只覆盖随二进制带上的 GCC runtime、GPL-3 文本和 winpthreads。
- `velocity_valid == 0` 不会作为「速度未知」被收下，本次直接得到 `XF_STALE`。需要速度未知的宿主不能靠这一位把该帧送进库。
- EOTS 进入 `LOST` 后，本次读到的快照仍保留上一个 `track_id`，并且 `has_primary` 仍为 1，只是 `locked` 变为 0。
- 原说明里的并发约束、停流 1 秒撤销锁定，以及上文标明未复测的 EOTS 边界，都不能当成这次已经跑过的结果。

仓库里没有 TODO 列表，也没有做了一半的源码模块。还要由宿主完成、且本仓库没有对应代码的部分：把 `xf_command` 发给飞控、接入真实厂商 SDK、统一坐标系，以及在需要 MSVC 时自行生成导入库。
