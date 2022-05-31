#ifndef WFREST_HTTPMSG_H_
#define WFREST_HTTPMSG_H_

#include "workflow/HttpMessage.h"
#include "workflow/WFTaskFactory.h"

#include <fcntl.h>
#include <unordered_map>
#include <memory>

#include "StringPiece.h"
#include "HttpDef.h"
#include "HttpContent.h"
#include "Compress.h"
#include "json_fwd.hpp"
#include "StrUtil.h"
#include "HttpCookie.h"
#include "Noncopyable.h"

namespace protocol
{
class MySQLResultCursor;

}  // namespace protocol


namespace wfrest
{

using Json = nlohmann::json;

struct ReqData;
class MySQL;

// request 类
class HttpReq : public protocol::HttpRequest, public Noncopyable
{
public:
    std::string &body() const;

    // post body
    std::map<std::string, std::string> &form_kv() const;

    Form &form() const;

    Json &json() const;

    http_content_type content_type() const
    { return content_type_; }

    // 获取 请求头 中的字段的值
    const std::string &header(const std::string &key) const;
    // 判断 指定 header 的字段是否存在
    bool has_header(const std::string &key) const;

    const std::string &param(const std::string &key) const;

    template<typename T>
    T param(const std::string &key) const;

    bool has_param(const std::string &key) const;

    const std::string &query(const std::string &key) const;

    const std::string &default_query(const std::string &key,
                                     const std::string &default_val) const;

    // 返回 请求中的参数的 map 集合
    const std::map<std::string, std::string> &query_list() const
    { return query_params_; }

    bool has_query(const std::string &key) const;

    // 返回 匹配的路由
    const std::string &match_path() const
    { return route_match_path_; }

    // handler define path
    const std::string &full_path() const
    { return route_full_path_; }

    // 返回当前路由
    std::string current_path() const
    { return parsed_uri_.path; }

    const std::map<std::string, std::string> &cookies() const;  // 获取 cookies 信息
    
    const std::string &cookie(const std::string &key) const;    // 查询 指定 cookie 的值
public:
    void fill_content_type();   // 保存 请求头 中 content_type 部分数据

    void fill_header_map();     // 将请求头信息中的字段 保存到 headers_ 中

    // 设置 路由参数
    // /{name}/{id} params in route
    void set_route_params(std::map<std::string, std::string> &&params)
    { route_params_ = std::move(params); }

    // /match*  
    // /match123 -> match123
    void set_route_match_path(const std::string &match_path)
    { route_match_path_ = match_path; }

    void set_full_path(const std::string &route_full_path)
    { route_full_path_ = route_full_path; }

    // 保存 请求中的参数
    void set_query_params(std::map<std::string, std::string> &&query_params)
    { query_params_ = std::move(query_params); }

    // 保存 解析到的 uri
    void set_parsed_uri(ParsedURI &&parsed_uri)
    { parsed_uri_ = std::move(parsed_uri); }

public:
    HttpReq();

    HttpReq(HttpRequest &&base_req) 
        : HttpRequest(std::move(base_req))
    {}

    ~HttpReq();

    HttpReq(HttpReq&& other);

    HttpReq &operator=(HttpReq&& other);

private:
    using HeaderMap = std::map<std::string, std::vector<std::string>, MapStringCaseLess>;
    
    http_content_type content_type_;    // 保存 content_type 字段
    ReqData *req_data_;                 // 请求的数据 结构体

    // 下面三个变量的设置 是在 Router.cc 文件的 call() 函数设置的
    std::string route_match_path_;                      // 路由中匹配到的路径
    std::string route_full_path_;                       // 路由中匹配到的完整路径
    std::map<std::string, std::string> route_params_;   // 存储路由中的参数


    std::map<std::string, std::string> query_params_;   // 存储路由中要查询的参数
    mutable std::map<std::string, std::string> cookies_;    // 存储 cookie 信息

    MultiPartForm multi_part_;  // 表单格式的数据
    HeaderMap headers_;         // 保存 请求头 字段的键值

    ParsedURI parsed_uri_;      // 解析到的 uri
};

// 获取 路由中的参数
template<>
inline int HttpReq::param<int>(const std::string &key) const
{
    if (route_params_.count(key))
        return std::stoi(route_params_.at(key));
    else
        return 0;
}

// 获取 路由中的参数
template<>
inline size_t HttpReq::param<size_t>(const std::string &key) const
{
    if (route_params_.count(key))
        return static_cast<size_t>(std::stoul(route_params_.at(key)));
    else
        return 0;
}

template<>
inline double HttpReq::param<double>(const std::string &key) const
{
    if (route_params_.count(key))
        return std::stod(route_params_.at(key));
    else
        return 0.0;
}


// response 类
class HttpResp : public protocol::HttpResponse, public Noncopyable
{
public:
    using MySQLJsonFunc = std::function<void(Json *json)>;

    using MySQLFunc = std::function<void(protocol::MySQLResultCursor *cursor)>;

    using RedisFunc = std::function<void(Json *json)>;
public:
    // send string
    void String(const std::string &str);

    void String(std::string &&str);

    // file
    void File(const std::string &path);

    void File(const std::string &path, size_t start);

    void File(const std::string &path, size_t start, size_t end);

    // save file
    void Save(const std::string &file_dst, const std::string &content);

    void Save(const std::string &file_dst, std::string &&content);

    // json
    void Json(const Json &json);

    void Json(const std::string &str);

    void set_status(int status_code);
    
    // Compress
    void set_compress(const Compress &compress);

    // cookie
    void add_cookie(HttpCookie &&cookie)
    { cookies_.emplace_back(std::move(cookie)); }

    void add_cookie(const HttpCookie &cookie)
    { cookies_.push_back(cookie); }

    const std::vector<HttpCookie> &cookies() const
    { return cookies_; }

    int get_state() const; 

    int get_error() const;

    // proxy
    void Http(const std::string &url, int redirect_max, size_t size_limit);

    void Http(const std::string &url, int redirect_max)
    { this->Http(url, redirect_max, 200 * 1024 * 1024); }
    
    void Http(const std::string &url)
    { this->Http(url, 0, 200 * 1024 * 1024); }
    
    // MySQL
    void MySQL(const std::string &url, const std::string &sql);

    void MySQL(const std::string &url, const std::string &sql, const MySQLJsonFunc &func);

    void MySQL(const std::string &url, const std::string &sql, const MySQLFunc &func);

    // Redis
    void Redis(const std::string &url, const std::string &command,
            const std::vector<std::string>& params);

    void Redis(const std::string &url, const std::string &command,
            const std::vector<std::string>& params, const RedisFunc &func);

    template<class FUNC, class... ARGS>
    void Compute(int compute_queue_id, FUNC&& func, ARGS&&... args)
    {
        WFGoTask *go_task = WFTaskFactory::create_go_task(
                "wfrest" + std::to_string(compute_queue_id),
                std::forward<FUNC>(func),
                std::forward<ARGS>(args)...);
        this->add_task(go_task);
    }

    void Error(int error_code);

    void Error(int error_code, const std::string &errmsg);

private:
    int compress(const std::string * const data, std::string *compress_data);

    void add_task(SubTask *task);

public:
    HttpResp() = default;

    HttpResp(HttpResponse && base_resp) 
        : HttpResponse(std::move(base_resp))
    {}

    ~HttpResp() = default;

    HttpResp(HttpResp&& other);

    HttpResp &operator=(HttpResp&& other);
    
public:
    std::map<std::string, std::string, MapStringCaseLess> headers;
    void *user_data;

private:
    std::vector<HttpCookie> cookies_;
};

using HttpTask = WFNetworkTask<HttpReq, HttpResp>;

} // namespace wfrest


#endif // WFREST_HTTPMSG_H_