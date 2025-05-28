DataStream 类 / DataStream Class
-------

#### 0 概述 / Overview
- `DataStream` 类可将C++基本数据类型和自定义类型转换为二进制数据，便于网络传输和存储。相比JSON/XML等通用格式，具有以下特点：
  - 转换速度更快
  - 支持类型更丰富
  - 内存占用更小

- The `DataStream` class can serialize C++ fundamental types and custom types into binary data for network transmission and storage. Compared to generic formats like JSON or XML, it offers:
  - Faster conversion
  - Broader type support
  - Reduced memory footprint

#### 1 主要改进 / Key Improvements

- 源码参考OpenDIS项目[OpenDIS](https://github.com/open-dis/open-dis-cpp/blob/master/src/dis6/utils/DataStream.h)，并做了如下改进：
  - 改为模板类，buffer类型作为模板参数
  - 增加对枚举类的支持
  - 实现buffer移动输出和基于buf的构造，降低数据转移成本
  
- Originally adapted from OpenDIS project [OpenDIS](https://github.com/open-dis/open-dis-cpp/blob/master/src/dis6/utils/DataStream.h). Key improvements is below:
  - Converted to template class with buffer type as template parameter
  - Added support for `enum class` types
  - Implemented buffer move semantics and buffer-based construction to reduce data transfer costs

#### 2 待优化项 / More To Do

- 你可以通过修改代码实现对一些C++标准容器的支持，如std::vector<int32_t>等
  
- You can implement support for some C++standard containers by modifying the code, such as std:: vector<int32_t>, etc
