# 第三方依赖来源

取得日期：2026-10-18。源码未做业务修改，构建使用本项目 CMake 包装。

- **cpp-httplib 0.61.0**：[cpp-httplib 官方来源](https://github.com/yhirose/cpp-httplib/releases/tag/v0.61.0)。MIT，许可证：httplib/LICENSE。
- **nlohmann/json 3.12.0**：[nlohmann/json 官方来源](https://github.com/nlohmann/json/releases/tag/v3.12.0)。MIT，许可证：json/LICENSE.MIT。
- **SQLite 3.54.1**：[SQLite 官方来源](https://sqlite.org/2026/sqlite-amalgamation-3540100.zip)。SQLite 公有领域声明见源文件与 https://sqlite.org/copyright.html。

SQLite 归档 SHA3-256 已与官方 [下载页](https://sqlite.org/download.html) 对照：

`92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30`

以下是本包实际包含文件的 SHA-256，便于检查复制或修改情况：

- `httplib/httplib.h`：`bc77114769db3b3c97e4a6d753dab797c3cadaaa48beead3806298b09702df18`
- `httplib/LICENSE`：`4b45cbe16d7b71b89ae6127e26e0d90a029198ca5e958ad8e3d0b8bbed364d8b`
- `json/LICENSE.MIT`：`46a65cffd1ea955132d95a8dd921640714a8d6b537d2e4e482d31145ae95b603`
- `json/nlohmann/json.hpp`：`aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`
- `sqlite/sqlite3.c`：`0c657666d819f261cc4007526ad57e1669de9949df7a8ea847aa284a2bf0ee52`
- `sqlite/sqlite3.h`：`d6c84557261893439b341d2e6c35187ae3ea24fbfd843ad94616f72d89242b8c`
