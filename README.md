# XF Swarm 二进制 SDK 使用说明

版本 **1.0.0**，C ABI **1**。本仓库仅交付动态库、C 接口声明和调用示例。

| 库 | 用途 |
|---|---|
| `xf_swarm` | 编队配置、无人机状态输入、领航目标设置、周期控制输出、停止与复位 |
| `xf_eots` | 对接外部视觉厂商命令和观测，读取光电载荷状态 |

EOTS 库提供接入与状态管理功能，不包含厂商检测模型、相机驱动或厂商 SDK。编队库输出控制指令，由宿主检查控制权限与飞控状态并发送。EOTS 目标不会自动触发编队跟随。

## 1. 文件和平台

```text
include/xf_swarm.h                   编队 C 接口声明与数据结构
include/xf_eots.h                    EOTS C 接口声明与数据结构
lib/windows-x86_64/*.dll             Windows x64 动态库
lib/windows-x86_64/*.dll.a           MinGW 导入库，仅用于链接 DLL
lib/windows-x86_64/*.def             Windows 导出声明
lib/linux-x86_64/*.so                Linux x64 动态库
cmake/XFSwarmConfig.cmake            CMake 导入配置
examples/basic.c                    仅调用公开接口的示例
SHA256SUMS                           库文件校验值
notices/                            第三方运行库声明
```

- 已验证 Windows x64 / MinGW GCC 15.2 和 Linux x86-64 / glibc 2.35。
- Linux 包要求 glibc 2.35 或更新版本；ARM、Android、macOS 和 32 位程序不能加载本包。
- 所有跨库数据为 C 类型，没有 STL 对象。句柄是不透明指针，只能交给对应 SDK 操作。
- 使用默认结构体对齐，不使用 `#pragma pack`。保持头文件和动态库版本一致。
- Windows 使用 `__cdecl`；从 C++ 调用也使用同一头文件中的 C linkage。业务程序不要定义 `XF_BUILD`。
- DLL 已包含所需 C++ 运行库；交付的 `.dll.a` 是导入表，不是算法静态库。二进制不等于不可逆向。

## 2. 运行使用示例

需要 CMake ≥ 3.20 和 C 编译器。这里只编译示例，不重新编译 SDK。

### Windows / MinGW

```powershell
git clone https://github.com/unununnnn/xf-swarm2.git
cd xf-swarm2
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
.\build\xf_example.exe
```

CMake 会把两个 DLL 放在示例可执行文件旁边。若自行集成，应把 DLL 放到程序目录或受控的系统 DLL 搜索目录。

### Linux

```bash
git clone https://github.com/unununnnn/xf-swarm2.git
cd xf-swarm2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/xf_example
```

示例仅使用模拟输入，打印控制结果和模拟 EOTS 状态，不连接飞控或相机。

### MSVC 导入库

本包未执行 MSVC 验证。需要使用 MSVC 时，可在 x64 Developer Command Prompt 中生成导入库后再配置 CMake：

```bat
lib /machine:x64 /def:lib\windows-x86_64\xf_swarm.def /out:lib\windows-x86_64\xf_swarm.lib
lib /machine:x64 /def:lib\windows-x86_64\xf_eots.def /out:lib\windows-x86_64\xf_eots.lib
```

也可使用 `LoadLibrary` / `GetProcAddress` 按 `.def` 中的名字加载 C 函数，回调须保持 `XF_CALL` 调用约定。

## 3. 引用 SDK

在自己的 CMake 工程中：

```cmake
find_package(XFSwarm CONFIG REQUIRED
    PATHS "/absolute/path/to/xf-swarm2/cmake" NO_DEFAULT_PATH)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE XF::swarm XF::eots)
```

只需要编队时链接 `XF::swarm`；只需要 EOTS 时链接 `XF::eots`。Linux 部署时通过应用 RPATH 或受控的 `LD_LIBRARY_PATH` 提供 `.so` 路径。

## 4. 编队调用顺序

```text
default_config → create → set_formation → set_leader → set_goal
                                            ↓
                     持续 push_state → 周期 step → 宿主发送结果
                                            ↓
                                      stop / reset → destroy
```

| 接口 | 用法 |
|---|---|
| `xf_swarm_abi_version()` | 检查库 ABI 是否为 `XF_ABI_VERSION` |
| `xf_swarm_default_config(&config)` | 获取有效配置默认值，包括结构体大小与版本 |
| `xf_swarm_create(&config, &handle)` | 创建实例，失败时 handle 为 NULL |
| `xf_swarm_set_formation(handle, members, count)` | 设置完整成员和偏移，清空旧状态并停用控制 |
| `xf_swarm_set_leader(handle, &current)` | 设置可信领航当前位置/朝向，初始化并启用控制 |
| `xf_swarm_set_goal(handle, &goal)` | 修改领航目标，不直接发送飞行指令 |
| `xf_swarm_push_state(handle, &state)` | 提交一个成员或邻机的当前有效/失效观测 |
| `xf_swarm_step(handle, now, out, capacity, &count)` | 执行一周期，返回配置中所有 local 成员的控制结果 |
| `xf_swarm_stop(handle)` | 锁存停止状态；后续 step 返回零速 |
| `xf_swarm_reset(handle)` | 清空观测和控制历史，保留成员配置，要求重新设置领航 |
| `xf_swarm_destroy(handle)` | 销毁实例；允许 NULL |

### 4.1 配置

先调用 `default_config`，再修改需要的业务参数。非法配置会被拒绝。

| 字段 | 单位与范围 |
|---|---|
| `struct_size` | 保持为 `sizeof(xf_config)` |
| `abi_version` | 保持为 `XF_ABI_VERSION` |
| `control_period_sec` | 控制周期，0.005～1 秒；默认 0.05 秒 |
| `max_speed_mps` | 水平速度限制，0.05～20 米/秒 |
| `max_vertical_speed_mps` | 垂直速度限制，0～20 米/秒 |
| `max_yaw_rate_rps` | 偏航角速度限制，0～10 弧度/秒 |
| `separation_m` | 规划使用的间距配置，0.1～100 米；不是物理防碰撞保证 |
| `telemetry_timeout_sec` | 遥测最长允许年龄，必须 > 0 且 ≤ 1 秒 |

### 4.2 成员与坐标

`xf_member` 包含：

- `id`：以 NUL 结尾的稳定 ID，最多 63 字节，整个成员表不得重复。
- `offset`：相对领航的队形偏移，x 前、y 左、z 上，单位米。
- `local`：1 表示本实例需要输出该机控制；0 表示只作为任务成员参与。

成员总数为 1～128，至少一个 local 成员。成员表必须包含完整任务花名册，不能因为暂时看不到某架飞机就删掉它。

`xf_leader_pose` 的 `position` 为共同原点 ENU 坐标，`yaw` 为弧度。`set_leader` 设置当前状态并把目标初始化为当前位置；之后用 `set_goal` 设置要移动到的位置/朝向。库在 `step` 中推进领航状态并产生各机控制，调用方不需要计算槽位。

`set_formation` 会停用控制并清空观测。切换队形后重新设置可信领航状态、目标和全体成员观测。`set_goal` 不解除 `stop`；恢复控制应显式调用 `set_leader`，必要时先 `reset`。

### 4.3 无人机观测

`xf_vehicle_state` 必须来自同一时刻相干的真实测量或外部状态估计：

| 字段 | 要求 |
|---|---|
| `id` | 与成员配置相同的稳定 ID；额外邻机也可提交 |
| `capture_time_sec` | 正值采集时间，与 step 的 `source_now_sec` 属于同一时钟 |
| `position` | 共同原点 ENU，米 |
| `orientation` | 机体 FLU 到 ENU 的四元数 `(x,y,z,w)`；零四元数无效 |
| `velocity` | ENU 世界系线速度，米/秒 |
| `confidence` | 有限值 `[0,1]`；当前观测有效阈值为 0.5 |
| `tracking_valid` | 1 为定位有效；0 为定位丢失 |
| `velocity_valid` | 1 为确有速度测量/估计；0 表示未知 |

位置和速度采用同一个采集时刻。速度未知不能填零冒充静止。输入前须在宿主统一 ENU 原点、单位和 FLU/FRD 约定；本接口不接受未经标定的相机像素、机体系速度或彼此不同的 VIO 原点。

SDK 在 `push_state` 时自行记录本地单调接收时间，并在 `step` 中同时检查采集年龄和收包年龄。重复/乱序采集时间不会刷新年龄；未来帧、过期帧、丢失或非法数据不会授权控制。不要把接收时间改写成采集时间。

成员/邻机身份缓存总容量为 128。大量更换身份时重新配置任务或 reset。时钟重置或原点改变时，应 reset 后重新初始化，不要继续沿用旧观测。

### 4.4 控制结果和停止

输出顺序与 `members` 中 local 成员的顺序一致。`xf_command` 包含 `id`、ENU `velocity`、`yaw_rate` 和 `diagnostic`。

| diagnostic | 含义 |
|---|---|
| `XF_DIAG_OK` | 有效控制结果；不代表已经执行或到达 |
| `XF_DIAG_MISSING_POSE` | 缺少本机位姿 |
| `XF_DIAG_MISSING_SLOT` | 未形成有效本机目标 |
| `XF_DIAG_INCOMPLETE_TELEMETRY` | 全体任务成员观测不完整、无效或过期 |
| `XF_DIAG_INVALID_INPUT` | 无法产生有效控制输出 |
| `XF_DIAG_STOPPED` | 已停止 |
| `XF_DIAG_NOT_READY` | 未准备好或本次调用未完成有效规划 |

除 OK 之外的诊断输出零速。`step` 可以返回 `XF_OK` 同时带 `INCOMPLETE_TELEMETRY`：必须同时检查函数返回值和每机诊断。

提供至少 local 成员数的容量，最大 128。容量不足返回 `XF_BUFFER_TOO_SMALL`，`count` 给出需要数量；此时不能读取超过 capacity 的元素。调用发生其他错误时也不得继续发送上一帧非零指令。建议直接使用 `xf_command out[XF_MAX_MEMBERS]`。

控制参考时间应递增。重复/回退时间或大于 1 秒的周期中断会返回 `XF_STALE` 并给出零速；恢复时持续提交新的有效观测。数据不完整期间领航不继续前进。

`stop` 只影响 SDK 后续输出，不会自动把停止报文发给飞控。宿主必须串行管理最终指令提交和停止意图，防止停止后发送之前缓存的非零结果。

## 5. EOTS 对接

创建时提供厂商命令回调：

```c
static xf_result XF_CALL on_command(void* user, uint32_t action, uint32_t mode) {
    /* 根据 action/mode 调用实际厂商 SDK，真实受理成功才返回 XF_OK。 */
    return XF_VENDOR_ERROR; /* 未接入厂商时显式失败。 */
}

xf_eots* eots = NULL;
xf_result r = xf_eots_create(on_command, user_context, &eots);
/* 检查 r == XF_OK 后才能使用 eots。 */
```

| 接口 | 用法 |
|---|---|
| `xf_eots_abi_version()` | 检查 ABI 版本 |
| `xf_eots_create(handler, user, &handle)` | 创建载荷实例；handler 不得为空 |
| `xf_eots_command(handle, action, mode)` | 向厂商转发命令 |
| `xf_eots_feed(handle, &observation)` | 在厂商帧回调中提交结果 |
| `xf_eots_read(handle, &snapshot)` | 获取可广播的载荷状态 |
| `xf_eots_destroy(handle)` | 销毁；调用前必须结束厂商回调 |

### 5.1 命令

action 为 `XF_EOTS_TRACK`、`XF_EOTS_CLEAR`、`XF_EOTS_STOP`；mode 为 `XF_EOTS_SOT` 或 `XF_EOTS_MOT`。分别映射到厂商启动跟踪、清除主目标并保持搜索、停止跟踪。

handler 在命令调用线程同步执行，返回受理结果，不等待目标锁定。失败映射为 `XF_VENDOR_ERROR`；成功受理后再接收正常帧。SDK 不会替厂商启动相机线程。

### 5.2 观测

`xf_eots_observation` 包含：

- `event`：APPEARED、UPDATED、LOCKED、LOST、RECOVERED、RESET、OVERRIDDEN 对应的 `XF_EOTS_*` 常量。
- `has_primary`：是否携带主目标。只有 LOST 和 RESET 可以为 0。
- `primary`：目标 ID、类别、1～31 字节且以 NUL 结尾的 label、置信度 `[0,1]`、原始图像中的像素框、归一化中心偏移及像素/帧速度。
- `frame`：原始图像 width/height、非负 capture_ms 和非负递增 seq。
- `fps`：有限且 > 0 的实测帧率。

框宽高必须 > 0，整个框须落在原始图像内。模型执行 resize、crop 或 letterbox 后，应先在厂商适配器中还原坐标。中心偏移范围 `[-1,1]`；目标像素速度不是无人机速度。

seq 必须严格递增，capture_ms 不能回退。同一帧的多个事件先在宿主汇总，再调用一次 feed。capture_ms 保留图像时钟原值；厂商适配器应主动丢弃内部积压的旧帧。

若新的相机会话从 0 重新编号，应停止并结束旧回调，销毁旧句柄后重新 create。不要把旧会话延迟回调送入新句柄。

### 5.3 状态读取

状态为 IDLE、SEARCHING、TRACKING、LOST、RECOVERING、OVERRIDDEN 对应的 `XF_EOTS_STATE_*`。

`snapshot` 使用 `has_primary/has_frame/has_fps` 指示字段是否有效。`xf_eots_read` 返回 `XF_NOT_READY` 时，不应继续发布上一帧 TRACKING。目标丢失时会撤销锁定并带出 LOST 状态；持续停流超过 1 秒也会撤销锁定。读取应由一个广播线程按固定周期执行。

当前快照只携带一个主目标。MOT 命令可转发，但没有完整多目标列表。库不创建 socket，宿主负责 JSON 编码或网络传输。

## 6. 两个 SDK 的连接关系

- 飞机自身或成员机的位姿/速度送入 `xf_swarm_push_state`。
- 被观察物体的像素框送入 `xf_eots_feed`，不能冒充某架无人机的位置。
- 若业务要求编队跟随目标，宿主须先把目标定位到共同 ENU 世界坐标，并按距离、高度和权限规则生成领航目标，再调用 `xf_swarm_set_goal`。
- 目标丢失、目标切换、接管时的任务策略由宿主决定；EOTS STOP 不会自动调用编队 STOP。

## 7. 线程、内存和错误处理

接口同步复制输入数据，不保留输入数组指针。输出由调用方分配；库内对象只能通过对应 destroy 释放。禁止复制/解引用句柄或调用已销毁句柄。

编队实例内部序列化状态更新和控制周期，建议一个控制线程负责配置、step、stop、reset，遥测回调负责 push。destroy 不得与任何接口并发。EOTS handler 的 user 对象必须比句柄及所有回调活得更久；handler 不得重入命令或销毁同一实例。

关闭 EOTS 时先停止/注销并 join 厂商回调，再 destroy。C 回调不要向 ABI 边界抛出异常。

| 函数返回值 | 含义 |
|---|---|
| `XF_OK` | 调用完成，仍需检查业务诊断或状态 |
| `XF_INVALID_ARGUMENT` | 参数、观测或当前 feed 条件不满足 |
| `XF_NOT_READY` | 尚未初始化完成或暂无可用快照 |
| `XF_BUFFER_TOO_SMALL` | 输出缓冲区不足 |
| `XF_STALE` | 乱序、重复或不连续时间 |
| `XF_CAPACITY` | 身份缓存容量已满 |
| `XF_STOPPED` | 编队处于停止状态 |
| `XF_VENDOR_ERROR` | 厂商命令未成功受理 |
| `XF_NO_MEMORY` | 资源分配失败 |
| `XF_INTERNAL_ERROR` | 库内部未完成本次调用 |

所有输入数值须有限，布尔标志使用 0/1。任一调用失败都应由宿主采取明确处理，不得继续使用过时控制结果。

## 8. 发布验证

本包已完成 Windows/Linux 的纯 C ABI 调用验证、示例编译运行，以及导出表和调试符号检查。编队库仅导出 11 个公开函数，EOTS 库仅导出 6 个公开函数。使用前可按 `SHA256SUMS` 核对二进制。

未执行 MSVC、ARM 或具体视觉厂商/飞控真机联调。不同平台、ABI 版本或新增能力需要更新二进制交付包，使用方不能在本仓库重新编译算法。
