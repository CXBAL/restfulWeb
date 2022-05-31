#include "workflow/HttpUtil.h"

#include "Router.h"
#include "HttpServerTask.h"
#include "HttpMsg.h"
#include "ErrorCode.h"

using namespace wfrest;

// 处理请求路由
void Router::handle(const char *route, int compute_queue_id, const WrapHandler &handler, Verb verb)
{
    VerbHandler &vh = routes_map_.find_or_create(route);
    if(vh.verb_handler_map.find(verb) != vh.verb_handler_map.end()) 
    {
        fprintf(stderr, "duplicate verb\n");
        return;
    }
    vh.verb_handler_map.insert({verb, handler});
    vh.path = route;
    vh.compute_queue_id = compute_queue_id;
}

int Router::call(Verb verb, const std::string &route, HttpServerTask *server_task) const
{
    HttpReq *req = server_task->get_req();
    HttpResp *resp = server_task->get_resp();

    // skip the last / of the url. Except for /
    // /hello ==  /hello/
    // / not change
    StringPiece route2(route);
    if (route2.size() > 1 and route2[static_cast<int>(route2.size()) - 1] == '/')
        route2.remove_suffix(1);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;
    auto it = routes_map_.find(route2, route_params, route_match_path);

    int error_code = StatusOK;
    if (it != routes_map_.end())   // has route
    {
        // match verb
        // it == <StringPiece : path, VerbHandler>
        std::map<Verb, WrapHandler> &verb_handler_map = it->second.verb_handler_map;
        bool has_verb = verb_handler_map.find(verb) != verb_handler_map.end() ? true : false;
        if(verb_handler_map.find(Verb::ANY) != verb_handler_map.end() or has_verb)
        {
            req->set_full_path(it->second.path);                    // 设置路由的完整路径
            req->set_route_params(std::move(route_params));         // 设置路由的参数
            req->set_route_match_path(std::move(route_match_path)); // 设置路由的匹配路径
            WFGoTask * go_task;
            if(has_verb) 
            {
                go_task = it->second.verb_handler_map[verb](req, resp, series_of(server_task)); // WrapHandler 处理函数 调用
            } else 
            {
                go_task = it->second.verb_handler_map[Verb::ANY](req, resp, series_of(server_task));    // WrapHandler 处理函数 调用
            }
            if(go_task)
                **server_task << go_task;
        } else
        {
            error_code = StatusRouteVerbNotImplment;
        }
    } else
    {
        error_code = StatusRouteNotFound;
    }
    return error_code;
}

// 打印路由
void Router::print_routes() const
{
    routes_map_.all_routes([](const std::string &prefix, const VerbHandler &verb_handler)
                        {
                            if(prefix == "/")
                            {
                                for(auto& vh : verb_handler.verb_handler_map)
                                {
                                    fprintf(stderr, "[WFREST] %s\t%s\n", verb_to_str(vh.first), prefix.c_str());
                                }
                            }
                            else 
                            {
                                for(auto& vh : verb_handler.verb_handler_map)
                                {
                                    fprintf(stderr, "[WFREST] %s\t/%s\n", verb_to_str(vh.first), prefix.c_str());
                                }
                            }
                        });
}

std::vector<std::pair<std::string, std::string> > Router::all_routes() const
{
    std::vector<std::pair<std::string, std::string> > res;
    routes_map_.all_routes([&res](const std::string &prefix, const VerbHandler &verb_handler)
                        {
                            for(auto& vh : verb_handler.verb_handler_map)
                            {
                                res.emplace_back(verb_to_str(vh.first), prefix.c_str());
                            }
                        });
    return res;
}
