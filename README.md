# 现代 C++ 后端实战教程

适合：学过 C/C++ 基础，尚未独立做过完整项目。

**先读 [HTML 教材](index.html) 或 [Markdown 教材](教程.md)。** 全书 30 章（0～29），从对象生命周期、RAII、标准库讲到 CMake、HTTP、SQLite、测试和部署。不要一次读完再开始动手，按每章练习推进。

在 GitHub 中可以直接阅读 Markdown 教材。HTML 阅读版请克隆或下载仓库后，用浏览器打开 index.html；GitHub 的代码页面不会直接运行 HTML。

```powershell
git clone https://github.com/kurunomochi/cpp-readme.git
Set-Location cpp-readme
```

- [完整源码导读](源码导读.md)：按学习顺序收录自有源码。
- [验证记录](验证记录.md)：已运行的检查与未验证的环境。
- [第三方来源与版本](third_party/NOTICE.md)：版本、许可证与 SHA-256 文件摘要。

## 第一次运行（Windows PowerShell）

进入本 README 所在目录。要求 64 位 C++20 编译器、CMake 3.24+、Ninja；HTTP 测试另需 Python 3.9+。如果本机工具已可用，不必重复安装。依赖源码已经附带，不需要联网下载库。

```powershell
g++ --version
cmake --version
ninja --version
python --version
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
.\build\debug\lab01_hello.exe
.\build\debug\task_server.exe
```

最后一条保持运行。在另一个终端：

```powershell
curl.exe -i http://127.0.0.1:8080/healthz
$body = [System.Text.Encoding]::UTF8.GetBytes('{"title":"Learn CMake"}')
Invoke-RestMethod -Method Post -Uri 'http://127.0.0.1:8080/api/tasks' -ContentType 'application/json' -Body $body
Invoke-RestMethod 'http://127.0.0.1:8080/api/tasks'
```

healthz 应返回 200 和 `{"status":"ok"}`。在服务器终端按 Ctrl+C 停止。数据库默认写到当前工作目录的 tasks.db，换目录启动可能打开另一份数据库。

如果 GCC 在 PATH 中找不到，而你使用 MSYS2 UCRT64，可在当前终端添加自己的工具链目录：

```powershell
$env:Path = 'C:\msys64\ucrt64\bin;' + $env:Path
```

上面使用 MSYS2 的默认安装路径；如果安装在其他目录，请相应替换。切换编译器时新建 build 目录。

## Linux

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build python3 curl
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/task_server
```

Linux、Docker、systemd 与 HTTPS 的部署步骤见教材 23～24 章，需在目标环境实际验收。

## 可运行学习阶段

| target | 源码 | 内容 |
|---|---|---|
| lab01_hello | examples/01_hello.cpp | 编译与输出 |
| lab02_modern | examples/02_modern.cpp | RAII、移动、ranges、span、concepts、jthread |
| lab03_cli | examples/03_cli.cpp | 内存版命令行任务管理 |
| lab04_http | examples/04_http.cpp | 8081 端口最小 HTTP 服务 |
| task_server | src/ | 8080 端口完整 API |

例如：`cmake --build build/debug --target lab03_cli`，再运行对应程序。

## API 与配置

`GET /healthz`；`GET/POST /api/tasks`；`GET/PATCH/DELETE /api/tasks/:id`。

创建 body：`{"title":"Learn C++"}`；修改 body：`{"title":"New title","completed":true}`；list 可带 limit/offset。

环境变量：APP_HOST 默认 127.0.0.1，APP_PORT 默认 8080，APP_DB 默认 tasks.db，APP_TOKEN 默认空。设置 token 后 API 需要 `Authorization: Bearer <token>`；healthz 保持公开。非回环监听要求至少 32 字节 token，建议由随机生成器产生。

这是单用户教学基线，完整用户账号、用户间权限隔离与集群能力属于后续扩展。本项目未自动部署到任何外部服务器。

## 辅助脚本

```powershell
python scripts/backup_db.py tasks.db backups/tasks-copy.db
python scripts/load_test.py --requests 200 --workers 8
```

priority 迁移脚本仅用于第 26 章扩展练习，先备份并停止使用该数据库的服务；当前 C++ API 仍需自己扩展 priority 字段。

本包的自有示例代码允许你学习、修改和用于自己的项目；第三方代码遵循各自许可证。HTML 教材无需联网即可阅读，外部官方文档链接需要联网。
