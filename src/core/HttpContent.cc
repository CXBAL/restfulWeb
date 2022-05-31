#include "workflow/StringUtil.h"
#include "workflow/WFFacilities.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "StrUtil.h"
#include "HttpContent.h"
#include "StringPiece.h"
#include "PathUtil.h"
#include "HttpDef.h"

using namespace wfrest;

const std::string MultiPartForm::k_default_boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

// 对请求体的数据进行切分
std::map<std::string, std::string> Urlencode::parse_post_kv(const StringPiece &body)
{
    std::map<std::string, std::string> map;

    if (body.empty())
        return map;

    // 对请求表单中的数据按 '&' 进行切分
    std::vector<StringPiece> arr = StrUtil::split_piece<StringPiece>(body, '&');

    if (arr.empty())
        return map;

    for (const auto &ele: arr)
    {
        if (ele.empty())
            continue;

        // 对数据 按 '=' 进行切分，划分出数据的 键和值
        std::vector<std::string> kv = StrUtil::split_piece<std::string>(ele, '=');
        size_t kv_size = kv.size();
        std::string &key = kv[0];

        if (key.empty() || map.count(key) > 0)
            continue;

        if (kv_size == 1)
        {
            map.emplace(std::move(key), "");
            continue;
        }

        std::string &val = kv[1];

        if (val.empty())
            map.emplace(std::move(key), "");
        else
            map.emplace(std::move(key), std::move(val));
    }
    return map;
}

// multipart 解析器的状态
enum multipart_parser_state_e
{
    MP_START,           // 解析开始
    MP_PART_DATA_BEGIN,
    MP_HEADER_FIELD,
    MP_HEADER_VALUE,
    MP_HEADERS_COMPLETE,
    MP_PART_DATA,
    MP_PART_DATA_END,
    MP_BODY_END
};

// multipart 解析器 的用户数据的结构
struct multipart_parser_userdata
{
    Form *mp;                       // 表单形式数据
    multipart_parser_state_e state; // multipart 解析器的状态
    std::string header_field;       // 数据中该部分数据描述的 字段
    std::string header_value;       // 数据中该部分数据描述的 对应的值
    std::string part_data;          // 数据中的具体数据
    std::string name;               // multipart 数据的名字
    std::string filename;           // multipart 数据的文件名（文件路径）

    void handle_header();           // 处理数据中该部分数据描述

    void handle_data();             // 处理数据
};

// 处理数据中该部分数据描述
void multipart_parser_userdata::handle_header()
{
    if (header_field.empty() || header_value.empty()) return;
    if (strcasecmp(header_field.c_str(), "Content-Disposition") == 0)
    {
        // Content-Disposition: attachment
        // Content-Disposition: attachment; filename="filename.jpg"
        // Content-Disposition: form-data; name="avatar"; filename="user.jpg"
        // 对 数据的描述字段的值进行切分    
        StringPiece header_val_piece(header_value);
        std::vector<StringPiece> dispo_list = StrUtil::split_piece<StringPiece>(header_val_piece, ';');

        for (auto &dispo: dispo_list)
        {
            auto kv = StrUtil::split_piece<StringPiece>(StrUtil::trim(dispo), '=');
            if (kv.size() == 2)
            {
                // name="file"
                // kv[0] is key(name)
                // kv[1] is value("file")
                StringPiece value = StrUtil::trim_pairs(kv[1], R"(""'')");
                if (kv[0].starts_with(StringPiece("name")))
                {
                    name = value.as_string();
                } else if (kv[0].starts_with(StringPiece("filename")))
                {
                    filename = value.as_string();
                }
            }
        }
    }
    header_field.clear();
    header_value.clear();
}

// 处理数据
void multipart_parser_userdata::handle_data()
{
    if (!name.empty())
    {
        std::pair<std::string, std::string> formdata;
        formdata.first = filename;      // 文件名
        formdata.second = part_data;    // 文件数据
        (*mp)[name] = formdata;
    }
    name.clear();
    filename.clear();
    part_data.clear();
}

// MultiPartForm 构造函数
MultiPartForm::MultiPartForm()
{
    settings_ = {
            .on_header_field = header_field_cb,
            .on_header_value = header_value_cb,
            .on_part_data = part_data_cb,
            .on_part_data_begin = part_data_begin_cb,
            .on_headers_complete = headers_complete_cb,
            .on_part_data_end = part_data_end_cb,
            .on_body_end = body_end_cb
    };
}

int MultiPartForm::header_field_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->handle_header();
    userdata->state = MP_HEADER_FIELD;
    userdata->header_field.append(buf, len);
    return 0;
}

int MultiPartForm::header_value_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_HEADER_VALUE;
    userdata->header_value.append(buf, len);
    return 0;
}

int MultiPartForm::part_data_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_PART_DATA;
    userdata->part_data.append(buf, len);
    return 0;
}

int MultiPartForm::part_data_begin_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_PART_DATA_BEGIN;
    return 0;
}

// 该函数里 handle_header() 函数执行
int MultiPartForm::headers_complete_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->handle_header();
    userdata->state = MP_HEADERS_COMPLETE;
    return 0;
}

// 该函数里 handle_data() 函数执行
int MultiPartForm::part_data_end_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_PART_DATA_END;
    userdata->handle_data();
    return 0;
}

int MultiPartForm::body_end_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_BODY_END;
    return 0;
}

// 解析表单格式的数据
Form MultiPartForm::parse_multipart(const StringPiece &body) const
{
    Form form;
    std::string boundary = "--" + boundary_;
    multipart_parser *parser = multipart_parser_init(boundary.c_str(), &settings_); // multipart 解析器 初始化，返回解析器类型的指针
    multipart_parser_userdata userdata;         // multipart 的用户数据
    userdata.state = MP_START;                  // 用户数据的 状态
    userdata.mp = &form;
    multipart_parser_set_data(parser, &userdata);       // 将 用户数据（multipart 解析器 的用户数据的结构） 传给 multipart 解析器，让 解析器进行解析
    size_t num_parse = multipart_parser_execute(parser, body.data(), body.size());  // multipart 解析器 具体执行，返回解析数据的长度
    multipart_parser_free(parser);              // multipart 解析器 释放
    return form;                                // 返回解析的数据
}



MultiPartEncoder::MultiPartEncoder()
    : boundary_(MultiPartForm::k_default_boundary)
{
}

void MultiPartEncoder::add_param(const std::string &name, const std::string &value) 
{
    params_.push_back({name, value});
}

void MultiPartEncoder::add_file(const std::string &file_name, const std::string &file_path)
{
    files_.push_back({file_name, file_path});
}

const std::string& MultiPartEncoder::boundary() 
{
    return boundary_;
}

void MultiPartEncoder::set_boundary(const std::string &boundary)
{
    boundary_ = boundary;
}

void MultiPartEncoder::set_boundary(std::string &&boundary) 
{
    boundary_ = std::move(boundary);
}

std::string &MultiPartEncoder::encode()
{
    content_.clear();

    std::vector<std::pair<void *, WFFuture<ssize_t> *>> buf_fr_list;
    for(auto &file : files_) 
    {
        int fd = open(file.second.c_str(), O_RDONLY);
        if (fd >= 0)
        {
            size_t size = lseek(fd, 0, SEEK_END);
            void *buf = malloc(size);   
            WFFuture<ssize_t> fr = WFFacilities::async_pread(fd, buf, size, 0);
            buf_fr_list.push_back({buf, &fr});        
        } else 
        {
            buf_fr_list.push_back({nullptr, nullptr});
        }
    }
    for(auto &param : params_)
    {
        content_ += "\r\n--";
        content_ += boundary_;
        content_ += "\r\nContent-Disposition: form-data; name=\"";
        content_ += param.first;
        content_ += "\"\r\n\r\n";
        content_ += param.second;
    }
    for(int i = 0; i < files_.size(); i++)
    {
        ssize_t len = buf_fr_list[i].second->get();
        if(!buf_fr_list[i].first || !len) 
        {
            continue;
        }
        char *file_content = static_cast<char *>(buf_fr_list[i].first);
        std::string file_suffix = PathUtil::suffix(files_[i].second);
        std::string file_type = ContentType::to_str_by_suffix(file_suffix);
        content_ += "\r\n--";
        content_ += boundary_;
        content_ += "\r\nContent-Disposition: form-data; name=\"";
        content_ += files_[i].first;
        content_ += "\"; filename=\"";
        content_ += PathUtil::base(files_[i].second);
        content_ += "\"\r\nContent-Type: ";
        content_ += file_type;
        content_ += "\r\n\r\n";
        content_ += std::string(file_content);
    }
    content_ += "\r\n--";
    content_ += boundary_;
    content_ += "--\r\n";
    for(auto &buf_fr : buf_fr_list)
    {
        free(buf_fr.first);
    }
    return content_;
}
