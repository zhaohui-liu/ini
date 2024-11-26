Simple C++ ini reader and writer
========

## 一个快速的 ini 文件解析库
* 支持解析ini文件
* 支持修改、保存ini文件
* 支持设置多个注释符，默认为“#”和';'

## 例子
```
#include <print>
#include <cstdlib>
#include "ini.hpp"

int main(int argc, char *argv[]) {
    ini ini;
    ini.section("server")->option("host")->set_string("127.0.0.1");
    ini.section("server")->option("port")->set_int32(8080);
    ini.section("server")->option("port")->set_comment("port number");
    std::println(ini.to_string());
    return EXIT_SUCCESS;
}
```

## 输出
```
[server]
host = 127.0.0.1
port = 8080 ;port number
```
