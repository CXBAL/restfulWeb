#ifndef WFREST_HTTPCONTENT_H_
#define WFREST_HTTPCONTENT_H_

#include <string>
#include <map>
#include <vector>
#include "MultiPartParser.h"
#include "Macro.h"
#include "Noncopyable.h"

namespace wfrest
{

class StringPiece;

// Urlencode 格式数据
class Urlencode
{
public:
    // 对请求的表单进行解析
    static std::map<std::string, std::string> parse_post_kv(const StringPiece &body);
};

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
// <name ,<filename, body>>

// Form 表单 的存储形式
using Form = std::map<std::string, std::pair<std::string, std::string>>;

// Modified From libhv
// MultiPartForm 格式数据
class MultiPartForm 
{
public:
    Form parse_multipart(const StringPiece &body) const;    // 解析表单格式的数据

    void set_boundary(std::string &&boundary)
    { boundary_ = std::move(boundary); }

    void set_boundary(const std::string &boundary)
    { boundary_ = boundary; }

public:
    static const std::string k_default_boundary;

    MultiPartForm();
private:
    static int header_field_cb(multipart_parser *parser, const char *buf, size_t len);

    static int header_value_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_begin_cb(multipart_parser *parser);

    static int headers_complete_cb(multipart_parser *parser);

    static int part_data_end_cb(multipart_parser *parser);

    static int body_end_cb(multipart_parser *parser);

private:
    std::string boundary_;

    multipart_parser_settings settings_;
};

class MultiPartEncoder 
{
public:
    MultiPartEncoder();

    std::string &encode();

    void add_param(const std::string &name, const std::string &value);

    void add_file(const std::string &file_name, const std::string &file_path);

    const std::string &boundary();

    void set_boundary(const std::string &boundary);

    void set_boundary(std::string &&boundary);

private:
    std::string boundary_;
    std::string content_;
    std::vector<std::pair<std::string, std::string>> params_;
    std::vector<std::pair<std::string, std::string>> files_;
};

}  // wfrest

#endif // WFREST_HTTPCONTENT_H_
