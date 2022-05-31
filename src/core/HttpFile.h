#ifndef WFREST_HTTPFILE_H_
#define WFREST_HTTPFILE_H_

#include <string>
#include <vector>

namespace wfrest
{
class HttpResp;

class HttpFile
{
public:
    // 服务器 给 客户端 发送文件
    static int send_file(const std::string &path, size_t start, size_t end, HttpResp *resp);

    // 服务器接收文件
    // content 参数：左值引用形式
    static void save_file(const std::string &dst_path, const std::string &content, HttpResp *resp);

    // 服务器接收文件
    // content 参数：右值引用形式
    static void save_file(const std::string &dst_path, std::string&& content, HttpResp *resp);
};

}  // namespace wfrest

#endif // WFREST_HTTPFILE_H_
