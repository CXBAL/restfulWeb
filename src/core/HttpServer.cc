#include "workflow/HttpMessage.h"

#include <utility>

#include "HttpServer.h"
#include "HttpServerTask.h"
#include "UriUtil.h"
#include "HttpFile.h"
#include "PathUtil.h"
#include "Macro.h"
#include "Router.h"
#include "ErrorCode.h"

using namespace wfrest;

// 该函数是获取请求过来的参数
void HttpServer::process(HttpTask *task)
{
    auto *server_task = static_cast<HttpServerTask *>(task);

    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();
    
    req->fill_header_map();     // 将请求头信息中的字段 保存到 headers_ 中
    req->fill_content_type();   // 保存 请求头 中 content_type 部分数据

    const std::string &host = req->header("Host");  // 获取 请求头 中的字段的值
    
    if (host.empty())
    {
        //header Host not found
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string request_uri = "http://" + host + req->get_request_uri();  // or can't parse URI
    ParsedURI uri;
    // uri 解析
    if (URIParser::parse(request_uri, uri) < 0)
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string route;

    if (uri.path && uri.path[0])
        route = uri.path;
    else
        route = "/";

    if (uri.query)
    {
        StringPiece query(uri.query);
        req->set_query_params(UriUtil::split_query(query)); // 保存 请求中的参数
    }

    req->set_parsed_uri(std::move(uri));    // 保存 解析到的 uri
    std::string verb = req->get_method();   // 保存 http 请求 的 动词
    // call() 函数中设置了 路由的完整路径、路由中的参数、路由中匹配到的路径 等信息
    // 即：route_full_path_ 、route_params_ 、route_match_path_ 
    int ret = blue_print_.router().call(str_to_verb(verb), route, server_task); 
    if(ret != StatusOK)
    {
        resp->Error(ret, verb + " " + route);
    }
    if(track_func_)
    {
        server_task->add_callback(track_func_);
    }
}

// new_session 产生一次交互，其实就是产生 server_task
CommSession *HttpServer::new_session(long long seq, CommConnection *conn)
{
    HttpTask *task = new HttpServerTask(this, this->WFServer<HttpReq, HttpResp>::process);
    task->set_keep_alive(this->params.keep_alive_timeout);
    task->set_receive_timeout(this->params.receive_timeout);
    task->get_req()->set_size_limit(this->params.request_size_limit);

    return task;
}

// 列出 路由
void HttpServer::list_routes()
{
    blue_print_.router().print_routes();
}

void HttpServer::register_blueprint(const BluePrint &bp, const std::string& url_prefix)
{
    blue_print_.add_blueprint(bp, url_prefix);
}

// /static : /www/file/
void HttpServer::Static(const char *relative_path, const char *root)
{
    BluePrint bp;
    int ret = serve_static(root, OUT bp);
    if(ret != StatusOK)
    {
        fprintf(stderr, "[WFREST] Error : %s dose not exists\n", root);
        return;
    }
    blue_print_.add_blueprint(std::move(bp), relative_path);
}

int HttpServer::serve_static(const char* path, OUT BluePrint &bp)
{
    std::string path_str(path);
    bool is_file = true;
    if (PathUtil::is_dir(path_str))
    {
        is_file = false;
    } else if(!PathUtil::is_file(path_str))
    {
        return StatusNotFound;
    }    
    bp.GET("/*", [path_str, is_file](const HttpReq *req, HttpResp *resp) {
        std::string match_path = req->match_path();
        if(is_file && match_path.empty())
        {
            resp->File(path_str);
        } else 
        {
            resp->File(path_str + "/" + match_path);
        }
    });
    return StatusOK;
}

// 追踪
HttpServer &HttpServer::track()
{
    track_func_ = [](HttpTask *server_task) {
        HttpResp *resp = server_task->get_resp();
        HttpReq *req = server_task->get_req();
        HttpServerTask *task = static_cast<HttpServerTask *>(server_task);
        Timestamp current_time = Timestamp::now();
        std::string fmt_time = current_time.to_format_str();
        // time | http status code | peer ip address | verb | route path
        fprintf(stderr, "[WFREST] %s | %s | %s | %s | \"%s\" | -- \n", 
                    fmt_time.c_str(),
                    resp->get_status_code(),
                    task->get_peer_addr_str().c_str(), 
                    req->get_method(),
                    req->current_path().c_str());
    };
    return *this;
}

HttpServer &HttpServer::track(const TrackFunc &track_func)
{
    track_func_ = track_func;
    return *this;
}

HttpServer &HttpServer::track(TrackFunc &&track_func)
{
    track_func_ = std::move(track_func);
    return *this;
}

