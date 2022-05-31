#ifndef WFREST_ROUTER_H_
#define WFREST_ROUTER_H_

#include <functional>
#include "RouteTable.h"
#include "Noncopyable.h"

namespace wfrest
{

class HttpServerTask;

class Router : public Noncopyable
{
public:
    // 处理路由
    void handle(const char *route, int compute_queue_id, const WrapHandler &handler, Verb verb);

    int call(Verb verb, const std::string &route, HttpServerTask *server_task) const;

    // 打印路由信息，
    void print_routes() const;   // for logging

    std::vector<std::pair<std::string, std::string>> all_routes() const;   // for test 

private:
    RouteTable routes_map_;     // 路由表 存储

    friend class BluePrint;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
