# ✨ wfrest: C++ Web Framework REST API

wfrest是一个快速🚀, 高效⌛️, 简单易用的💥 c++ 异步web框架.

wfrest基于[✨**C++ Workflow**✨](https://github.com/sogou/workflow)开发. [**C++ Workflow**](https://github.com/sogou/workflow) 是一个设计轻盈优雅的企业级程序引擎.

你可以用来：

- 快速搭建http服务器：

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });

    if (svr.start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

## 文档

- [API 示例](#🎆-api-examples)
    - [路由中的参数](./docs/cn/param_in_path.md)
    - [url请求参数](./docs/cn/query_param.md)
    - [表单数据](./docs/cn/post_form.md)
    - [发送文件](./docs/cn/send_file.md)
    - [保存文件](./docs/cn/save_file.md)
    - [上传文件](./docs/cn/upload_file.md)
    - [Json](./docs/cn/json.md)


## 编译 && 安装

### 需求

* workflow, 版本大于等于 v0.9.9 
* Linux , 比如ubuntu 18.04 或者更新版本
* Cmake
* zlib1g-dev
* libssl-dev
* libgtest-dev
* gcc 和 g++ 或者 llvm + clang

如果你在ubuntu 20.04，你可以用以下命令安装

```bash
apt-get install build-essential cmake zlib1g-dev libssl-dev libgtest-dev -y
```

### cmake

```
git clone --recursive https://github.com/wfrest/wfrest
cd wfrest
make
sudo make install
```

编译例子:

```
make example
```

测试:

```
make check
```
