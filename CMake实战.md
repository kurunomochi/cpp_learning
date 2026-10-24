# CMake 手把手实战：从第一条命令到可被复用的工程

这不是要求你记住所有命令的手册，而是一条能够从空目录复现的练习路线。先理解“谁调用谁、路径相对哪里、要求传给谁”，再把这些关系写进配置。每个实验都附带完整文件；可以先原样运行一次，再在自己新建的目录重写。

本篇以 CMake **3.24 及以上**、C++20、Ninja 为基础，避免依赖最新文档才有的语法。Windows 主线为 PowerShell + MSYS2 UCRT64 GCC。四个实验在 `lessons/cmake/01_hello`、`02_library`、`03_project`、`04_consumer`，都是独立 CMake 工程，不需要先构建 HTTP 服务器。

除明确写出 Push-Location 的预设实验外，命令都在仓库根目录执行。`build/cmake01` 等是建议的新构建目录；本地若已有不兼容缓存，请选择另一个名字。实验代码不下载依赖，C07 的远程依赖方案另行注明。

## C01. CMake 工作流：配置、生成、构建、测试、运行

### 五件不同的事

| 阶段 | 典型命令 | 具体发生什么 |
|---|---|---|
| 配置与生成 | `cmake -S 源码目录 -B 构建目录 -G Ninja` | 解释 CMakeLists、检测编译器、建立目标关系并生成 Ninja 文件 |
| 构建 | `cmake --build 构建目录` | 调用 Ninja，让它运行编译与链接命令 |
| 测试 | `ctest --test-dir 构建目录` | 启动登记的测试程序，检查退出结果 |
| 运行 | `路径/程序.exe` | 启动你的程序，读取运行期配置 |
| 安装 | `cmake --install 构建目录 --prefix 目录` | 按 install 规则复制已构建的产物和配套文件 |

一次 `cmake -S ... -B ...` 通常包含 configure 和 generate 两个阶段。configure 时执行 CMake 语言、创建 targets；generate 时根据完整依赖关系生成构建规则。build 才执行真正的编译工作。项目变更后 Ninja 可能自动触发重新配置，但初学时改完配置主动再运行一次 cmake 更容易看懂。

`ctest` 一般不会替你先编译新代码；运行前先 build。`cmake --install` 也不等于自动构建；先 build 再 install。启动服务器后修改 CPP，旧进程不会热更新。

### 先确认自己调用了哪套工具

```powershell
Get-Command cmake,g++,ninja
cmake --version
g++ --version
ninja --version
cmake --help
```

`cmake --help` 的末尾列出本机可用生成器。Ninja 是构建工具；`-G Ninja` 是告诉 CMake 要生成它能读的规则。Visual Studio 和 Ninja Multi-Config 生成器使用不同目录布局和配置选择方式，C06 再讲。

如果系统里有多套 GCC、Python、CMake，打印位置比只看“我已经安装了”更可靠。在 MSYS2 主线中保持 UCRT64 编译器、运行库和依赖一致。编辑器显示的编译器不一定等于终端 PATH 里最先找到的编译器。

### 三种目录，画出来再运行

源码目录保存 CMakeLists 和 cpp；构建目录保存 cache、目标文件和程序；当前工作目录决定相对命令路径和运行期相对数据路径。`-S` 指 source，`-B` 指 binary/build，它们与终端当前目录可以不同。

```text
仓库根目录/                     ← 本篇默认终端位置
  lessons/cmake/01_hello/        ← -S 指向这里，里面必须有 CMakeLists.txt
  build/cmake01/                ← -B 指向这里，由 CMake 创建
```

不要把产物写进源码根目录再到处找垃圾文件；不要手工修改生成的 build.ninja，它下次生成会被覆盖。先学会描述工程，让工具维护派生产物。

**实验**：先只执行 C02 的 configure，观察 build 下出现 cache 和 Ninja 文件，但还没有 hello.exe；执行 build 后再找可执行文件。这样你就亲眼区分了“CMake 成功”和“程序编译成功”。

## C02. 五行配置逐项解释，并观察一次增量构建

第一阶段只有两个文件：

文件：`lessons/cmake/01_hello/main.cpp`

```cpp
#include <iostream>
int main() { std::cout << "Hello, CMake!\n"; }
```

文件：`lessons/cmake/01_hello/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(hello_cmake VERSION 1.0.0 LANGUAGES CXX)
add_executable(hello main.cpp)
target_compile_features(hello PRIVATE cxx_std_20)
set_target_properties(hello PROPERTIES CXX_EXTENSIONS OFF)
```

第一行声明最低 CMake 版本并设置相应策略基线，版本不是 C++ 标准版本。第二行命名项目、设置版本、启用 CXX 语言；project 会触发编译器检测。第三行创建逻辑目标 hello，main.cpp 是相对此 CMakeLists 所在源码目录的源文件。第四行为这个目标提出至少 C++20 的编译能力要求。第五行关闭此目标的 C++ 扩展模式，避免不经意依赖 gnu++ 扩展。

`hello_cmake` 是项目名，`hello` 是 target 名，`hello.exe` 是本平台产物名；三者没有必须相同的规定。target 不只是一个文件名，它带有源码、头文件路径、编译特性和依赖关系。

```powershell
cmake -S lessons/cmake/01_hello -B build/cmake01 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake01 --parallel 2
.\build\cmake01\hello.exe
cmake --build build/cmake01 --verbose
```

预期程序输出 `Hello, CMake!`。最后一次没有更改文件，Ninja 通常报告 no work to do。修改 main.cpp 的文字，再 build，只需重新编译受影响的目标并链接。`--verbose` 在确实有构建工作时显示实际命令，不保证没有改动时也重打历史命令。

### 如果新增一个源文件

```cmake
# 替换原来的 add_executable 行，extra.cpp 需真实存在
add_executable(hello main.cpp extra.cpp)
# 或在已创建 hello 之后追加：target_sources(hello PRIVATE extra.cpp)
```

两种方式任选一种，不要把相同实验当成必须同时执行。头文件可以列在 sources 中改善 IDE 展示，但这并不替代 include 路径配置，也不会把普通头文件当独立 cpp 编译。不要 `#include "extra.cpp"` 来绕过链接问题。

### 为什么课程不用 GLOB 自动搜所有 cpp

明确列出源文件能让新增、删除在版本差异中直接出现。GLOB 可用于特定场景，但“新文件是否触发重新配置”和生成器行为需要理解；不要因为少写两行而把重要构建输入变隐含。文件很多时按模块拆 CMakeLists，C05 会做。

**练习及答案**：给 main.cpp 使用的函数创建声明和定义，把定义放 extra.cpp。若忘记把 extra.cpp 加入 target，通常是链接错误；若头文件位置不对，是编译错误。将两种故障各制造一次，保留第一条实质诊断，不必把整个红色日志都背下来。

## C03. CMake 也是一种语言：参数、变量、列表、作用域

### 不把 C++ 和 CMake 语法混在一起

命令形式是 `命令名(参数...)`，不需要分号结束语句；`#` 开始注释。变量名常区分大小写，命令名不区分大小写。一般保持命令小写、自定义项目开关大写，帮助阅读。`set(name value)` 写变量，`${name}` 在参数中展开它。

文件：`lessons/cmake/variables.cmake`

```cmake
set(label "outer")
set(parts alpha beta gamma)
function(show_scope)
  set(label "inner")
  message(STATUS "function label=${label}")
endfunction()
show_scope()
message(STATUS "outside label=${label}")
foreach(part IN LISTS parts)
  message(STATUS "part=${part}")
endforeach()
```

运行这个脚本不需要 project，也不会编译 C++：

```powershell
cmake -P lessons/cmake/variables.cmake
```

预期先看到 `function label=inner`，再看到 `outside label=outer`，随后依次 alpha、beta、gamma。function 建立变量作用域，内部同名变量不会直接改掉外面；`PARENT_SCOPE` 可以明确向上一层传值，但不要把它当全局变量通道乱用。macro 不建立相同的函数变量作用域，初学封装优先用 function。

`set(parts alpha beta gamma)` 存成分号分隔的列表。`foreach(part IN LISTS parts)` 遍历各项；`message("${parts}")` 看到一个整体字符串。路径里有空格时给展开加引号，例如 `"${CMAKE_CURRENT_SOURCE_DIR}/include"`。带分号的特殊路径仍有列表语义风险，学习工程优先选普通目录名。

### 认识几类相似但不同的变量

| 名称 | 从哪里来 | 用在哪里 |
|---|---|---|
| `${MY_VALUE}` | 普通变量或 cache 查找 | CMake 执行期间 |
| `$ENV{PATH}` | 当前 CMake 进程环境 | 找工具、读取外部配置 |
| `-DMY_OPTION=ON` | 命令行写入 cache | 跨 configure 保存用户选择 |
| `MY_DEFINE=1` | target_compile_definitions | 传给 C/C++ 预处理器 |
| `$<CONFIG:Debug>` | 生成器表达式 | 生成/构建配置相关的目标属性 |

`set(APP_PORT 8080)` 不会自动让 C++ 代码有 APP_PORT 宏，也不会设置程序运行时环境变量。要生成编译期常量可以 configure_file；最终服务器的 APP_PORT 则是在运行时用 getenv 读取，这是另外一层。

### 条件与目录作用域

```cmake
# 放在启用了 CXX 的 CMake 工程里，而不是单纯 -P 脚本中使用
option(MY_ENABLE_LOGS "Enable extra compile-time logging" OFF)
if(MY_ENABLE_LOGS)
  message(STATUS "Extra logs enabled")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Using GCC")
endif()
```

`option` 创建 BOOL cache 选项；ON/OFF 是 CMake 的真假约定，和 C++ bool 不是同一个对象。STREQUAL 做字符串相等比较，MATCHES 使用正则表达式。include 在当前作用域执行文件；add_subdirectory 处理另一个目录，有自己的目录变量作用域并继承上层设置。

常见路径变量：CMAKE_CURRENT_SOURCE_DIR 是正在处理目录的源码路径；CMAKE_CURRENT_BINARY_DIR 是其对应构建路径；PROJECT_SOURCE_DIR 是最近 project 的源码根；CMAKE_SOURCE_DIR 是最外层源码根。可复用子项目如果到处使用最外层根路径，嵌入别人的工程时容易失效。

**练习**：把脚本函数里的 label 改成不同值，预测外部输出；再增加 `set(result "done" PARENT_SCOPE)`，观察 result。**参考**：label 不变，新增 result 被带回调用处；不应为了修改 target 属性使用 PARENT_SCOPE，target_compile_options 等直接修改对应目标即可。

## C04. 拆成静态库：PUBLIC、PRIVATE、INTERFACE 的真实用法

第二阶段把函数放到库，让应用消费它。目录有意让 app 和 include 分开，避免编译器因恰好同目录而掩盖错误。

文件：`lessons/cmake/02_library/include/greeting/greeting.hpp`

```cpp
#pragma once
#include <string>
#include <string_view>
namespace greeting {
std::string greet(std::string_view name);
}
```

文件：`lessons/cmake/02_library/src/greeting.cpp`

```cpp
#include "greeting/greeting.hpp"
namespace greeting {
std::string greet(std::string_view name) { return "Hello, " + std::string{name} + "!"; }
}
```

文件：`lessons/cmake/02_library/app/main.cpp`

```cpp
#include "greeting/greeting.hpp"
#include <iostream>
int main() {
    const auto message = greeting::greet("CMake");
    std::cout << message << '\n';
    return message == "Hello, CMake!" ? 0 : 1;
}
```

文件：`lessons/cmake/02_library/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(greeting_demo LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(greeting STATIC src/greeting.cpp)
target_include_directories(greeting PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(greeting PUBLIC cxx_std_20)
add_executable(greeting_app app/main.cpp)
target_link_libraries(greeting_app PRIVATE greeting)
include(CTest)
if(BUILD_TESTING)
  add_test(NAME greeting_output COMMAND greeting_app)
endif()
```

```powershell
cmake -S lessons/cmake/02_library -B build/cmake02 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake02 --verbose
ctest --test-dir build/cmake02 --output-on-failure --no-tests=error
.\build\cmake02\greeting_app.exe
```

输出 `Hello, CMake!`。构建先把 greeting.cpp 编译成目标文件，再归档成静态库；应用编译 main.cpp，再把必要的库代码链接进程序。静态库归档不是一个普通可运行进程，也通常不是最终解析所有外部符号的步骤。

### include 路径从哪里来

源代码写 `#include "greeting/greeting.hpp"`，因此搜索根应是 include，而不是 include/greeting。库自己的 cpp 需要此搜索根，应用在包含公开头时也需要，于是标为 PUBLIC。

| 关键字 | 当前 target 使用要求 | 消费当前 target 的目标得到要求 |
|---|---|---|
| PRIVATE | 有 | 不传播该编译使用要求 |
| PUBLIC | 有 | 有 |
| INTERFACE | 本目标不用 | 有 |

这张表用于理解 include、编译特性等使用要求。静态库的 PRIVATE 链接依赖可能仍需要传递到最终链接步骤，不能推导为“PRIVATE 的库绝不会出现在程序链接命令中”。编译使用要求与最终链接需要分别分析，详见 [target_link_libraries 官方说明](https://cmake.org/cmake/help/v3.24/command/target_link_libraries.html)。

应用 `target_link_libraries(greeting_app PRIVATE greeting)` 获得 greeting 的使用要求，包括 include 和 C++20；PRIVATE 的意思是应用不再将这条依赖作为自己的公开接口向下传播，不是拒绝继承 greeting 的 PUBLIC 配置。

### 一个能真正观察差异的故障实验

在你复制的实验目录中，把 greeting 的 include 改 PRIVATE。库自己仍能编译，但 app 找不到 greeting/greeting.hpp，说明要求未传播。恢复 PUBLIC；重新 configure/build 应通过。不要为了救急给全局 include_directories 加所有路径，这会掩盖真实依赖。

把库改为 INTERFACE 不能直接“省略 cpp 编译”：普通有实现的 cpp 必须编译。header-only 库才可用 `add_library(name INTERFACE)` 包装 include 和特性。INTERFACE target 本身通常没有 .a/.lib 产物。

STATIC、SHARED、OBJECT 分别表示静态库、共享库、目标文件集合。共享库还涉及导出符号和运行时加载路径，尤其 Windows DLL；不能假设把 STATIC 改为 SHARED 后所有平台交付自然完整。本课程先把静态库链路做对。

**练习**：增加 goodbye 声明和实现并从应用调用。只改库 cpp 时，应用 cpp 通常不用重编译，但应用需要重新链接；改公开头可能导致库和应用都重新编译。用 --verbose 对照你的预测。

## C05. 多目录工程：配置文件如何协作，生成头文件放哪里

第三阶段叫 TaskKit，是一个独立的小型标题校验库。它提供小应用和测试，最后安装供第四阶段消费。此处的标题校验只用于构建教学，规则不完全等同于后端的控制字符及 UTF-8 校验。

```text
03_project/
  CMakeLists.txt
  CMakePresets.json
  include/taskkit/title.hpp
  src/CMakeLists.txt、title.cpp
  app/CMakeLists.txt、main.cpp
  tests/CMakeLists.txt、title_test.cpp
  cmake/Warnings.cmake、config.hpp.in、TaskKitConfig.cmake.in
```

完整根配置如下。第一次先关注 option/include/configure_file/add_subdirectory；后半段 install/export 留到 C09 再拆解。这个文件是完整可构建版本，不需要读者猜测省略的部分。

文件：`lessons/cmake/03_project/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(TaskKit VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(TASKKIT_BUILD_APP "Build the demo executable" ON)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/Warnings.cmake)

configure_file(cmake/config.hpp.in generated/taskkit/config.hpp @ONLY)
add_subdirectory(src)
if(TASKKIT_BUILD_APP)
  add_subdirectory(app)
endif()
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS taskkit EXPORT TaskKitTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(TASKKIT_BUILD_APP)
  install(TARGETS taskkit_app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT TaskKitTargets NAMESPACE TaskKit::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
configure_package_config_file(cmake/TaskKitConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
```

根文件管理全局选择和模块顺序；模块内部管理自己的 target。set(CMAKE_CXX_EXTENSIONS OFF) 为之后创建的目标初始化默认属性，而不是神奇地回头改掉所有已经创建的目标。使用项目级默认值要注意出现位置。

文件：`lessons/cmake/03_project/cmake/Warnings.cmake`

```cmake
function(taskkit_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
endfunction()
```

这里没有把警告选项公开传播给别人的代码，每个本项目目标显式调用 taskkit_warnings。MSVC 使用 /W4 等参数，GCC/Clang 使用 -Wall 等参数，不能把某家编译器参数无条件复制给另一家。

文件：`lessons/cmake/03_project/src/CMakeLists.txt`

```cmake
add_library(taskkit STATIC title.cpp)
add_library(TaskKit::taskkit ALIAS taskkit)
target_compile_features(taskkit PUBLIC cxx_std_20)
target_include_directories(taskkit
  PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "${PROJECT_BINARY_DIR}/generated")
taskkit_warnings(taskkit)
```

TaskKit::taskkit 是 taskkit 的别名，不会再编译一次。带 `::` 的名字清楚表达一个有命名空间的 target，拼错时也更容易得到明确诊断。

BUILD_INTERFACE 给源码树内消费者使用 include 路径；INSTALL_INTERFACE 给安装后消费者使用安装前缀下的 include 路径。generated 头只被 title.cpp 使用，因此是 PRIVATE。如果某个公开头也包含它，就必须重新设计安装与可见性，而不能靠碰巧存在的 build 目录。

文件：`lessons/cmake/03_project/include/taskkit/title.hpp`

```cpp
#pragma once
#include <string>
#include <string_view>
namespace taskkit {
bool valid_title(std::string_view text);
std::string version();
}
```

文件：`lessons/cmake/03_project/src/title.cpp`

```cpp
#include "taskkit/title.hpp"
#include "taskkit/config.hpp"
namespace taskkit {
bool valid_title(std::string_view text) {
    if (text.empty() || text.size() > 200) return false;
    // 本实验只识别 ASCII 的六种空白，不是完整 Unicode 空白规则。
    return text.find_first_not_of(" \t\n\r\f\v") != std::string_view::npos;
}
std::string version() { return TASKKIT_VERSION; }
}
```

文件：`lessons/cmake/03_project/cmake/config.hpp.in`

```cpp
#pragma once
#define TASKKIT_VERSION "@PROJECT_VERSION@"
```

根文件的 configure_file 把模板里的 PROJECT_VERSION 替换成 1.0.0，输出到**构建树**的 generated/taskkit/config.hpp。不要改生成文件，应该改模板或版本，再重新配置。`@ONLY` 让它只展开 @VAR@ 形式，避免误处理希望保留的 `${...}` 文本。相关语义见 [configure_file](https://cmake.org/cmake/help/v3.24/command/configure_file.html)。

文件：`lessons/cmake/03_project/app/CMakeLists.txt`

```cmake
add_executable(taskkit_app main.cpp)
target_link_libraries(taskkit_app PRIVATE TaskKit::taskkit)
taskkit_warnings(taskkit_app)
```

文件：`lessons/cmake/03_project/app/main.cpp`

```cpp
#include "taskkit/title.hpp"
#include <iostream>
int main() {
    std::cout << "TaskKit " << taskkit::version() << '\n'
              << std::boolalpha << taskkit::valid_title("learn") << '\n';
}
```

```powershell
cmake -S lessons/cmake/03_project -B build/cmake03 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake03 --parallel 2
.\build\cmake03\app\taskkit_app.exe
```

预期两行：`TaskKit 1.0.0`、`true`。多目录示例产物在 app 子目录，别把主后端项目的平铺路径机械照搬过来。`cmake --build build/cmake03 --target taskkit` 只构建库及它的必要依赖，不构建所有应用和测试。

**练习**：用 `-DTASKKIT_BUILD_APP=OFF` 配置新的 build/cmake03-libonly，确认还能构建库和测试。解释为什么 src 必须在 app/tests 之前可用。已有同名 executable 残留不会自动代表当前配置仍构建它，因此实验优先使用新 build 目录。

## C06. Cache、Presets、Debug/Release 与工具链选择

### Cache 是用户配置的持久化记录

CMakeCache.txt 保存编译器路径、生成器、选项、包查找结果等。普通变量在一次配置的作用域里存在，cache 项保存在构建树内，下次配置可以继续使用。`option(X ... ON)` 是默认值，不应强行覆盖用户上次选定的 OFF。

```powershell
cmake -LAH -N build/cmake03
cmake -S lessons/cmake/03_project -B build/cmake03 -DTASKKIT_BUILD_APP=OFF
cmake -S lessons/cmake/03_project -B build/cmake03 -DTASKKIT_BUILD_APP=ON
```

`-N` 只查看而不执行通常的配置/生成，`-LAH` 展示非高级/高级 cache 值及帮助说明。不要把 cache 当项目源文件提交。更换编译器、体系结构或生成器时使用新构建目录最清楚。

3.24 提供 `--fresh` 可重建配置缓存，但它不等于保证清空所有旧产物，更不是对运行中数据的清理工具。初学排错建议直接用新的 build/cmake03-retry 目录，保留旧日志方便比较。

### 单配置与多配置

Ninja、Unix Makefiles 常用单配置：configure 时用 CMAKE_BUILD_TYPE 选择 Debug/Release，一个构建树主要对应一个配置。Ninja Multi-Config、Visual Studio 是多配置：构建时 `--config Debug`，测试 `-C Debug`，安装 `--config Debug`。不要用 CMAKE_BUILD_TYPE 来控制后者。

```powershell
# 独立可选实验：同一 build 树保留多种配置
cmake -S lessons/cmake/03_project -B build/cmake03-multi -G "Ninja Multi-Config"
cmake --build build/cmake03-multi --config Debug
ctest --test-dir build/cmake03-multi -C Debug --output-on-failure --no-tests=error
.\build\cmake03-multi\app\Debug\taskkit_app.exe
```

Debug 通常更方便断点与局部变量观察；Release 通常启用优化，但不是“无需测试的正式版”。优化、符号、NDEBUG 等具体参数应看实际构建命令，RelWithDebInfo 可同时提供一定优化与调试信息。不把 assert 当全部测试，因为常见 Release 配置会禁用它。

### Presets 记录团队常用组合

文件：`lessons/cmake/03_project/CMakePresets.json`

```json
{
  "version": 5,
  "cmakeMinimumRequired": {"major": 3, "minor": 24, "patch": 0},
  "configurePresets": [
    {"name": "debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build/debug",
     "cacheVariables": {"CMAKE_BUILD_TYPE": "Debug", "BUILD_TESTING": "ON"}},
    {"name": "release", "inherits": "debug", "binaryDir": "${sourceDir}/build/release",
     "cacheVariables": {"CMAKE_BUILD_TYPE": "Release"}}
  ],
  "buildPresets": [
    {"name": "debug", "configurePreset": "debug", "jobs": 2},
    {"name": "release", "configurePreset": "release", "jobs": 2}
  ],
  "testPresets": [
    {"name": "debug", "configurePreset": "debug", "output": {"outputOnFailure": true},
     "execution": {"noTestsAction": "error"}},
    {"name": "release", "configurePreset": "release", "output": {"outputOnFailure": true},
     "execution": {"noTestsAction": "error"}}
  ]
}
```

version 5 是预设文件格式版本，不是项目版本。configurePresets 记录生成器、构建目录与 cache；buildPresets 关联 configurePreset 并设置并行数；testPresets 设置输出和无测试时的失败行为。release 继承 debug 的公共项，再覆盖 build type 与 binaryDir。

```powershell
Push-Location lessons/cmake/03_project
cmake --list-presets
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
cmake --preset release
cmake --build --preset release
ctest --preset release
Pop-Location
```

Push-Location 进入这个**独立工程**，否则在仓库根目录执行 --preset debug 会配置主后端工程。`${sourceDir}` 指预设所在项目的源码根。不能把 JSON 里的宏语法照搬到 PowerShell。

CMakePresets.json 提交给团队；个人机器路径放 CMakeUserPresets.json 并忽略提交。个人预设可继承公共 debug，指定自己的工具链；不要把 `D:/某人电脑/编译器` 写入团队默认。官方对共享和本地预设的区别见 [Presets 说明](https://cmake.org/cmake/help/v3.24/manual/cmake-presets.7.html)。

### 工具链必须在首次语言检测前确定

```powershell
# 可选：把路径换成本机真实存在的编译器，使用全新 build 目录
cmake -S lessons/cmake/03_project -B build/cmake03-gcc -G Ninja '-DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe'
```

设置编译器路径不保证 Ninja、链接器、运行时 DLL 自动就绪；终端 PATH 仍要合理。也可在第一次 configure 前设置 CXX 环境变量，已有 cache 时它通常不会覆盖原选择。不要在 project() 之后突然换编译器。

交叉编译工具链文件会描述目标系统、处理器、编译器和 sysroot；那时编译产物不一定能在主机直接执行，CTest 需要目标机或模拟器。此篇不假装一个通用 toolchain 模板能适配所有嵌入式平台，先掌握本机的可复现链路。

**练习与答案**：用两个 build 目录分别构建 Debug/Release，查看是否各有 cache；给 library-only 配置关闭 BUILD_TESTING，`ctest -N` 应找不到本项目测试，不能把这一结果当测试通过。

## C07. 第三方依赖：源码、find_package、FetchContent 与包管理器

### 先识别你手中拿到的到底是什么

只有头文件的库常用 INTERFACE target；有 cpp/c 源码的库需要编译；预编译库需要与当前平台、架构、编译器 ABI 和运行库兼容。头文件能被找到不代表链接就会成功；链接成功也不代表运行时 DLL 一定能找到。

最终后端采用 vendored 源码：third_party 随项目附带，版本、许可证与摘要记录在 NOTICE 中。优点是离线和固定输入，代价是仓库变大、升级和漏洞跟踪由维护者负责。新学习项目不要一开始同时混用多个来源的同一个库。

```cmake
# 概念片段：已准备相应路径时才能使用
add_library(json_vendor INTERFACE)
target_include_directories(json_vendor SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json")
target_link_libraries(my_app PRIVATE json_vendor)
```

SYSTEM 通常把该 include 作为系统头目录处理以减少外部警告，但不表示“安全可信”；my_app 必须已创建。INTERFACE 目标也通过 target_link_libraries 使用，因为这个命令表达的不只是 .lib/.a 链接，还包括使用要求。

### find_package 搜索已提供的包配置

```cmake
# C09 的真实实验会安装并查找到这个包
find_package(TaskKit 1 CONFIG REQUIRED)
target_link_libraries(consumer PRIVATE TaskKit::taskkit)
```

CONFIG 模式找 TaskKitConfig.cmake / taskkit-config.cmake。Module 模式找 FindTaskKit.cmake，模块可能来自 CMake 自带或你设置的 CMAKE_MODULE_PATH。CMAKE_PREFIX_PATH 提供安装前缀搜索提示；TaskKit_DIR 则可指向具体 Config 文件所在目录。不要把前者指成 include 子目录后期待自动解决所有事情。

find_package 通常负责查找和加载使用信息，不等于默认联网安装软件。依赖提供器/包管理器集成可能改变查找流程，所以看清项目实际配置。优先消费库给出的 imported target，它能携带头路径、链接产物和依赖，不要手写几十个绝对 .a 路径。

### FetchContent 在配置时取得源码

下面是独立联网方案的示意，**没有加入本教程默认构建，也未在本次离线实验中执行**：

```cmake
include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)
# 在 add_executable(my_app ...) 之后
target_link_libraries(my_app PRIVATE nlohmann_json::nlohmann_json)
```

第一次配置需要网络和 Git；受限网络失败不是 C++ 语法错误。课程用固定版本标签演示，严格复现发布更应固定不可变提交或带校验和的归档，记录依赖更新原因。不能把分支 main 当永远不变的输入。与构建时才下载的 ExternalProject 相比，这里需要配置阶段拿到内容，见 [FetchContent 官方文档](https://cmake.org/cmake/help/v3.24/module/FetchContent.html)。

### vcpkg / Conan 的定位

包管理器解决依赖取得、版本与构建组合；CMake 继续描述自己工程的 target。以 vcpkg 为例，常在首次 configure 提供它的 toolchain 文件，并用 manifest 记录依赖；Conan 常产生可供 CMake 查找的配置和工具链。具体命令随版本变化，使用时查各自官方文档，不把一个历史安装脚本当普适真理。

在 Windows 上不要把 MSVC 编译的 C++ 库随意塞给 MinGW GCC，也不要混搭 32/64 位、不同 CRT 或不兼容标准库。静态/动态链接、运行库选择和许可证均要了解。C 接口也需要确认调用约定和 ABI，不代表所有二进制随便互换。

**练习**：从最终项目指出 httplib、json、sqlite 分别是哪种包装 target，找出哪个实际编译了 .c。**参考**：httplib/json 是 INTERFACE，sqlite3_vendor 是 STATIC；SQLite 源码使用 C 编译，因此 project 启用了 C 和 CXX。

## C08. 测试与调试：让构建结果有可检查的行为

文件：`lessons/cmake/03_project/tests/CMakeLists.txt`

```cmake
add_executable(title_test title_test.cpp)
target_link_libraries(title_test PRIVATE TaskKit::taskkit)
taskkit_warnings(title_test)
add_test(NAME title_rules COMMAND title_test)
```

文件：`lessons/cmake/03_project/tests/title_test.cpp`

```cpp
#include "taskkit/title.hpp"
#include <iostream>
#include <string>
int main() {
    int failures{};
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
    };
    check(taskkit::valid_title("learn"), "ordinary title");
    check(!taskkit::valid_title(""), "empty title");
    check(!taskkit::valid_title(" \t"), "whitespace title");
    check(taskkit::valid_title(std::string(200, 'a')), "200-byte boundary");
    check(!taskkit::valid_title(std::string(201, 'a')), "201-byte boundary");
    check(taskkit::version() == "1.0.0", "configured version");
    return failures == 0 ? 0 : 1;
}
```

根目录 include(CTest) 提供 BUILD_TESTING 选项并在启用时打开测试支持。tests 子目录只有选项开启时才加入；add_test 注册一个测试，COMMAND title_test 引用可执行目标，让 CMake 处理平台后缀和配置路径。

测试程序有六个检查：普通标题、空串、纯 ASCII 空白、200 字节、201 字节和配置版本。失败时输出具体检查名字并返回非零。一个 CTest 条目可以包含多个内部检查，因此“1 个测试目标”与“6 项断言条件”不是同一统计。

```powershell
cmake --build build/cmake03 --target title_test
ctest --test-dir build/cmake03 -N
ctest --test-dir build/cmake03 -R title_rules --output-on-failure --no-tests=error
ctest --test-dir build/cmake03 -R title_rules -V
```

`-N` 列出但不运行；`-R` 按正则筛选；--output-on-failure 保留失败输出；-V 展示更详细的测试命令和输出。`--no-tests=error` 避免因为名字写错或没有注册测试而空跑成功。

### 做一个真能抓住错误的实验

在你复制的实验中把长度上限从 200 改成 199，重新 build/test。200-byte boundary 应失败；恢复后再通过。只运行 CTest 而不 build，可能还在测试旧程序，这正是本章要求你主动观察的陷阱。

Release 中测试仍应有效。本例没有把所有断言写成 assert，而是普通运行时判断；因此不会随 NDEBUG 消失。编译成功是语法和链接层的检查，测试检查行为，两者不能互相替代。

### compile_commands.json 与编辑器

CMAKE_EXPORT_COMPILE_COMMANDS 在支持的生成器上输出每个源文件的真实编译命令，可供 clangd 和分析工具使用。编辑器只显示红线但命令行构建成功时，先看它是否读取了正确构建树、标准版本和 include 路径。不要为消除编辑器红线把项目逻辑改坏。

Ninja 主线可通过 CMake Tools 选择项目与 preset，或让 clangd 指向相应 build 目录。不同时启用多个互相争夺补全和诊断的 C++ 语言服务器；具体设置按所用扩展文档选择。本教程提供命令行作为独立可验证的基准。

### 调试时先保证程序和源码对应

```powershell
gdb .\build\cmake03\tests\title_test.exe
```

在 GDB 内输入 `break taskkit::valid_title`、`run`、`next`、`print text`、`bt`、`continue`、`quit`。不同标准库的漂亮打印支持影响 string_view 的显示，可先查看字段或更简单局部变量。若优化后变量消失，先改 Debug 并重新构建，而不是断言调试器坏了。

ASan/UBSan 等检测需编译和链接两侧配置，且依赖工具链与平台支持；主后端的 TASK_ENABLE_SANITIZERS 明确只为 Linux GCC/Clang 开启。不要把 `-fsanitize` 无条件复制到 MSVC/当前 Windows 教学路线。

**验收**：能让测试主动失败并说明是哪条规则被破坏；能指出测试启动的是哪个 executable；能确认 Release 的检查依旧执行。

## C09. install 与 export：让另一个项目只依赖安装结果

### 安装可执行文件和交付可复用库是两种需求

主后端 install 只安装 task_server 到 bin，用于运行程序。TaskKit 还需要公开头、静态库、导出的 target 信息和包配置，另一个 CMake 工程才能 `find_package`。不能只复制头文件就声称库交付完成。

根配置里 `install(TARGETS taskkit EXPORT TaskKitTargets ...)` 把库加入导出集合；`install(EXPORT ... NAMESPACE TaskKit:: ...)` 写出 target 定义；`install(DIRECTORY include/ ...)` 安装公开头，注意 include/ 的结尾斜杠表示复制其内容。

`GNUInstallDirs` 提供 bin、lib、include 的惯常变量，避免处处硬写平台目录。静态库使用 ARCHIVE，动态库常还涉及 LIBRARY/RUNTIME 等类别；Windows 的运行时 DLL 与导入库也需要分别考虑。本实验限定静态库。

文件：`lessons/cmake/03_project/cmake/TaskKitConfig.cmake.in`

```cmake
@PACKAGE_INIT@
include("${CMAKE_CURRENT_LIST_DIR}/TaskKitTargets.cmake")
check_required_components(TaskKit)
```

configure_package_config_file 生成包入口并处理安装前缀相关内容，入口通过自身文件所在目录加载 Targets；write_basic_package_version_file 生成版本匹配信息。SameMajorVersion 表达同主版本的兼容策略，实际 API/ABI 是否兼容仍由作者维护，配置声明不是自动证明。

### 完整安装及消费命令

先确认 C05 已完成构建，再运行：

```powershell
$taskkitPrefix = Join-Path $PWD 'install/taskkit'
cmake --install build/cmake03 --prefix "$taskkitPrefix"
cmake -S lessons/cmake/04_consumer -B build/cmake04 -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$taskkitPrefix"
cmake --build build/cmake04 --parallel 2
ctest --test-dir build/cmake04 --output-on-failure --no-tests=error
.\build\cmake04\consumer.exe
```

第四阶段的完整文件：

文件：`lessons/cmake/04_consumer/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(taskkit_consumer LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(TaskKit 1 CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE TaskKit::taskkit)
include(CTest)
if(BUILD_TESTING)
  add_test(NAME installed_package COMMAND consumer)
endif()
```

文件：`lessons/cmake/04_consumer/main.cpp`

```cpp
#include <taskkit/title.hpp>
#include <iostream>
int main() {
    std::cout << "Imported TaskKit " << taskkit::version() << '\n';
    return taskkit::valid_title("external app") && !taskkit::valid_title("") ? 0 : 1;
}
```

预期 `Imported TaskKit 1.0.0`。consumer 没有把第三阶段源码 add_subdirectory 进来，也没有硬写其 build 路径。它通过安装前缀取得 include、库和 C++ 标准使用要求。

### 安装后复用的四种常见错误

一是只装 .a 不装头，编译阶段找不到声明；二是装了头但没写 Config/Targets，find_package 找不到包；三是导出接口仍包含开发者电脑上的源码绝对路径，换目录就坏；四是包被找到但 ABI 不兼容，编译/链接/运行仍可能失败。

本例 BUILD_INTERFACE/INSTALL_INTERFACE 把开发路径和安装路径分开；generated 头只用于构建库，所以不需要安装。如果公开接口依赖其他库，包配置可能需要 `find_dependency` 查找传递依赖，不能只把自己的 Target 文件扔给消费者。

### 可移动安装位置实验

把 install/taskkit 复制到另一个新的目录，用新的 build/cmake04-relocated 配置消费者，CMAKE_PREFIX_PATH 指向复制位置。检查生成的编译命令不再引用旧源码目录。使用全新 build 防止 cache 继续找到旧包。

这验证的是包路径可移动，不证明不同编译器、不同操作系统之间二进制兼容。动态运行时依赖也不一定被 install 自动收齐，Windows 在开发终端运行成功可能依赖 PATH 中的 GCC DLL，部署到干净机器仍需检查。

**过关标准**：能解释 Config、Targets、Version 三类文件的职责，并在新的 consumer 构建树中运行成功。导入导出整体机制可对照 [CMake 官方指南](https://cmake.org/cmake/help/v3.24/guide/importing-exporting/index.html)。

## C10. 生成器表达式、生成文件与构建依赖：理解执行时间

### 不同写法发生在不同时间

`set`、if 和 configure_file 在配置阶段工作；`$<...>` 生成器表达式在支持它的属性上下文中稍后求值；编译器、代码生成器和链接器由构建工具按依赖触发。不要在 configure 阶段读取一个只有 build 才产生的文件，然后疑惑它为什么不存在。

```cmake
# 片段：app 必须已经存在
target_compile_definitions(app PRIVATE "$<$<CONFIG:Debug>:APP_DEBUG=1>")
```

外层条件表达式在 Debug 配置生成 APP_DEBUG=1，其他配置为空，适用于多配置生成器。`if(CMAKE_BUILD_TYPE STREQUAL "Debug")` 只是配置时判断变量，对多配置工程无法表达同一个构建树的多种配置。

`$<TARGET_FILE:task_server>` 指向当前配置下的实际可执行文件路径，主后端的 Python HTTP 测试用它启动服务器。硬写 `build/Debug/task_server.exe` 会破坏 Linux、Ninja 和用户自定义输出路径。

### configure_file 与构建时生成命令

版本号来自 CMake 变量时使用 configure_file 已够用。若需要根据 schema 调用生成器产出源码，应把输入、输出、命令和依赖纳入构建图。下面是**进阶结构片段**，`tools/generate.py` 和 `schema.txt` 是需要你自己实现的扩展输入，不是默认工程内已有文件：

```cmake
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(generated_header "${CMAKE_CURRENT_BINARY_DIR}/generated/schema.hpp")
add_custom_command(
  OUTPUT "${generated_header}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/schema.txt" "${generated_header}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/schema.txt"
  VERBATIM)
add_custom_target(generate_schema DEPENDS "${generated_header}")
add_dependencies(app generate_schema)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
```

OUTPUT 是实际产物；DEPENDS 指定输入变化时要重跑；VERBATIM 帮助正确转义命令参数，不是自动执行任意 shell 语法。生成脚本必须失败返回非零并且真的写出目标文件。漏掉 DEPENDS 会导致输入已改、产物却没更新；多个独立规则写同一个输出会造成并行竞争。

add_custom_target 本身通常总被认为需要构建，但本例的文件规则由 OUTPUT 的时间戳决定是否重跑。add_dependencies 建立目标顺序；target_link_libraries 还表达链接与使用要求，二者不能相互替代。语义可查 [add_custom_command 官方文档](https://cmake.org/cmake/help/v3.24/command/add_custom_command.html)。

### 公共头改变为什么会触发重新编译

编译器会给 Ninja 提供头文件依赖信息，实际包含某个头的源文件会受它变化影响。CMake 描述工程层依赖，编译器参与发现文本包含依赖；不是每次改头都全量编译所有项目，也不是不把头列在 sources 就永远检测不到。

不要在配置脚本中随意执行网络下载、改系统环境或修改源码树的副作用。一个可复现工程应尽量让源码是输入、构建树是输出；同一份源码可以对应 Debug、Release、不同工具链的多个构建树。

**练习与参考**：把 TaskKit 的版本改成 1.0.1，configure 后观察 generated 头，再 build。测试的版本契约也需相应更新。只改生成头会在下次配置时被模板覆盖，说明正确修改位置是 project VERSION 和对应测试预期。

## C11. 回到真实后端：逐段读懂完整 CMakeLists

到这里不要再把根配置当成一整段魔法。下面是配套服务当前的完整配置，随后逐块说明为什么存在：

文件：`CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(task_server VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(TASK_ENABLE_SANITIZERS "Enable ASan and UBSan (Linux GCC/Clang)" OFF)
include(CTest)
find_package(Threads REQUIRED)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)
if(MSVC)
  target_compile_options(project_options INTERFACE /W4 /permissive- /utf-8 /EHsc)
else()
  target_compile_options(project_options INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
if(TASK_ENABLE_SANITIZERS)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(project_options INTERFACE -fsanitize=address,undefined)
  else()
    message(FATAL_ERROR "This sanitizer preset is configured for Linux GCC/Clang only")
  endif()
endif()

add_library(sqlite3_vendor STATIC third_party/sqlite/sqlite3.c)
target_include_directories(sqlite3_vendor SYSTEM PUBLIC third_party/sqlite)
target_compile_definitions(sqlite3_vendor PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION)
target_link_libraries(sqlite3_vendor PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX)
  target_link_libraries(sqlite3_vendor PUBLIC m)
endif()

add_library(json_vendor INTERFACE)
target_include_directories(json_vendor SYSTEM INTERFACE third_party/json)
add_library(httplib_vendor INTERFACE)
target_include_directories(httplib_vendor SYSTEM INTERFACE third_party/httplib)
target_link_libraries(httplib_vendor INTERFACE Threads::Threads)
if(WIN32)
  target_link_libraries(httplib_vendor INTERFACE ws2_32 crypt32)
endif()

add_library(task_core STATIC src/domain.cpp src/repository.cpp)
target_include_directories(task_core PUBLIC include)
target_compile_features(task_core PUBLIC cxx_std_20)
target_link_libraries(task_core PRIVATE sqlite3_vendor project_options)

add_executable(task_server src/main.cpp src/http_api.cpp)
target_link_libraries(task_server PRIVATE task_core httplib_vendor json_vendor project_options)

add_executable(lab01_hello examples/01_hello.cpp)
add_executable(lab02_modern examples/02_modern.cpp)
add_executable(lab03_cli examples/03_cli.cpp)
add_executable(lab04_http examples/04_http.cpp)
target_link_libraries(lab01_hello PRIVATE project_options)
target_link_libraries(lab02_modern PRIVATE project_options Threads::Threads)
target_link_libraries(lab03_cli PRIVATE task_core project_options)
target_link_libraries(lab04_http PRIVATE httplib_vendor json_vendor project_options)

if(BUILD_TESTING)
  add_executable(core_tests tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE task_core project_options Threads::Threads)
  add_test(NAME core COMMAND core_tests)
  add_test(NAME modern_examples COMMAND lab02_modern)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME http_integration
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/http_test.py $<TARGET_FILE:task_server>)
    set_tests_properties(http_integration PROPERTIES TIMEOUT 60)
  else()
    message(WARNING "Python3 missing: HTTP integration test not registered")
  endif()
endif()

install(TARGETS task_server RUNTIME DESTINATION bin)
```

### 从 project 到 project_options

`LANGUAGES C CXX` 为 SQLite 的 C 源码和业务的 C++ 源码分别检测编译器。CMAKE_CXX_EXTENSIONS 控制 C++ 扩展默认值，COMPILE_COMMANDS 提供编辑器/工具输入。TASK_ENABLE_SANITIZERS 是用户可选诊断开关，include(CTest) 决定测试选项，Threads 查找平台线程使用要求。

project_options 是本项目代码复用的 INTERFACE 配置包。应用/库 PRIVATE 依赖它得到 C++20 和警告；第三方源码不无条件继承我们的严格警告。sanitizer 同时加入 compile 和 link 选项，并限制在已经明确配置的 Linux GCC/Clang 路线上。

### 三个第三方 target

sqlite3_vendor 编译 sqlite3.c，PUBLIC 提供其头路径，PRIVATE 的 SQLITE_* 宏配置 SQLite 自身构建。Threads::Threads 是跨平台线程依赖，CMAKE_DL_LIBS 表达平台动态加载库需求，UNIX 上的 m 是数学库需求；不要把这些 Unix 参数复制进所有 Windows target。

json_vendor 和 httplib_vendor 为 header-only 依赖提供 include。httplib 的模板/内联实现由包含它的使用者编译，因此相关线程和 Windows socket 库需要通过 INTERFACE 传递。Windows 分支加入 ws2_32、crypt32，具体 SSL/TLS 能力并不因出现 crypt32 就全部开启。

### 业务库与应用

task_core 收纳 domain.cpp 与 repository.cpp，公开 include/task，公开 C++20 特性。SQLite 只出现在实现里，公开头通过前置声明 sqlite3 和自定义删除器减少直接依赖；因此 PRIVATE 链接 sqlite3_vendor。最终可执行文件仍必须满足 SQLite 链接需要，这与“头文件搜索要求不公开”并不矛盾。

task_server 把入口和 HTTP 适配层连到核心库、HTTP/JSON 包装和项目选项。几个 lab 是并列可执行目标，共用需要的依赖；不存在“必须先运行 lab 再启动服务”的构建顺序。

### 测试与安装

core_tests 直接测领域和仓库；modern_examples 运行语言示例；http_integration 用 Python 启动真实服务、发送请求和检查行为。Python 未找到时明确警告，CTest 条目数会变少，所以要确认不是空跑或漏测。

安装只将 task_server 放进 bin。它没有承诺打包 Windows GCC 运行时 DLL、创建 systemd 用户、迁移线上数据库或部署 HTTPS。构建系统、运行环境和发布操作需要相互配合，主教材后面分别讲述。

### 本配置的学习型取舍

核心库显式 STATIC，依赖固定在仓库中，没有复杂包管理器初始化；线程模型和平台分支尽量少。等你能独立复现，再考虑模块拆分、依赖升级、安装包和交叉编译。增加一个 cpp 时优先判断它属于哪个 target，而不是先寻找哪里再粘贴一串 flags。

**练习**：新建 `examples/05_config.cpp` 输出环境变量 APP_PORT（为空时输出默认值），增加独立 executable，链接 project_options，并添加一个自检。**参考路线**：仿照 lab01 增加 add_executable → target_link_libraries → 在 BUILD_TESTING 内 add_test；运行期环境变量无需变成编译宏。

## C12. 排错顺序、独立练习与完整验收

### 按阶段定位，先看第一条实质错误

| 现象 | 先检查 | 如何验证 |
|---|---|---|
| source directory does not contain CMakeLists | -S 和当前目录 | 查看指定目录是否有文件 |
| CXX compiler not found | 工具是否安装、PATH、cache | Get-Command，换新 build 后指定工具链 |
| Ninja / make program not found | -G 与构建工具 | ninja --version、cmake --help |
| generator does not match | 旧目录生成器不一致 | 用全新 build 目录 |
| 头文件找不到 | include 搜索根、PUBLIC 传播 | --verbose 看 -I 路径 |
| undefined reference / unresolved external | 定义是否编译、签名是否一致、库是否链接 | 看 target sources 与最终链接命令 |
| multiple definition | 非 inline 定义重复放头、cpp 被重复包含 | 每个普通定义只保留一次 |
| find_package 失败 | Config 是否安装、前缀是否正确 | 检查 TaskKitConfig.cmake；必要时 --debug-find |
| 找不到 exe | 构建是否成功、多配置和子目录路径 | 看链接输出，确认实际配置 |
| 缺 DLL | 运行库是否可找到 | 在正确工具链终端运行；规划部署依赖 |
| CTest 没有测试 | BUILD_TESTING、Python、-R 筛选 | ctest -N、--no-tests=error |
| 改代码无效果 | 构建了另一个目录、旧进程未重启 | 查看 exe 路径、重建、停止旧服务 |

错误排查要留证据：完整命令、当前目录、工具路径、首条实质错误、你刚修改的文件。先改一个原因再验证，不要同时切编译器、改生成器、删缓存、换依赖，让问题失去可解释性。

### 练习一：不看现成 CMakeLists 重建两文件项目

main.cpp 调用 calculator.cpp 中的 checked_add。公开声明放 include/calculator.hpp，目标 app 链接 calculator 静态库。要求 C++20，Debug，开启合理警告。

参考结构为 add_library(calculator STATIC ...) → PUBLIC include → PUBLIC cxx_std_20 → add_executable(app ...) → PRIVATE 链接 calculator。验证普通加法和超范围返回无值；不要用单个“输出 hello”假装业务检查。

### 练习二：给 TaskKit 增加端口解析模块

把 F11 的 parse_port 放到 include/taskkit/port.hpp 与 src/port.cpp。返回 optional<int>，范围 1～65535，全串消费。增加独立测试，覆盖空串、80x、0、65536、1、65535。库的 add_library 需要新源文件；公开头安装规则已经覆盖 include 目录，但你要实际重装并让 consumer 调用新函数证明完成。

参考拆解：先实现并测试库 → 更新应用或 consumer → build/test → install 到新的前缀 → 新 consumer build。保持 TaskKit 版本兼容策略合理。如果只在源码树里跑通而安装包缺文件，任务还没有完成。

### 练习三：从一台开发机走向可复现交付

在一个没有旧 build 的新目录解压源码，按 README 配置、编译、测试。确认不依赖原作者的盘符、私人头文件路径或未写入说明的环境变量。为 Debug 和 Release 都保留结果，运行期产生的数据库留在交付包外。

单机通过后再在 Linux/CI 目标环境验收，记录不同平台的结果；不要把生成了工作流 YAML 当成云端已经执行成功。更换编译器或依赖版本是一种新实验，要重新检查 ABI、警告和行为测试。

### CMake 毕业检查

你应当能解释 -S/-B/-G/-D，区分 configure/build/test/install，添加 cpp 与静态库，正确传播 include 和标准要求，读懂 cache/preset，定位编译与链接错误，运行不会因 Release 消失的测试，并让独立消费者从安装前缀找到库。

若某项仍只能复制现有配置才能完成，回到相应实验重写一次。后续学习顺序建议是依赖包管理 → 共享库与运行时加载 → 安装包/CPack → 交叉编译 → 更复杂生成器与缓存加速。先让当前工程能解释、能复现，再增加构建复杂度。

需要进一步查证时使用 [CMake 3.24 文档](https://cmake.org/cmake/help/v3.24/) 和 [官方循序教程](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)。最新版页面可能出现高于本课程最低版本的功能，复制前检查 Added in version，不为了一个示例无意提高整个工程的环境要求。
