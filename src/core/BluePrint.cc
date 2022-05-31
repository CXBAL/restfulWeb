#include "BluePrint.h"
#include "HttpMsg.h"

using namespace wfrest;

void BluePrint::ROUTE(const char *route, const Handler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler](const HttpReq *req,
                            HttpResp *resp,
                            SeriesWork *) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    asp->before(req, resp);
                }

                handler(req, resp);     // handler 函数的调用

                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto asp : global_aspect->aspect_list)
                        {
                            asp->after(req, resp);
                        }
                    });
                }
                return nullptr;
            };

    router_.handle(route, -1, wrap_handler, verb);
}

void BluePrint::ROUTE(const char *route, int compute_queue_id, const Handler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id](HttpReq *req,
                                        HttpResp *resp,
                                        SeriesWork *) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    asp->before(req, resp);
                }
                WFGoTask *go_task = WFTaskFactory::create_go_task(
                        "wfrest" + std::to_string(compute_queue_id),
                        handler,
                        req,
                        resp);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto asp : global_aspect->aspect_list)
                        {
                            asp->after(req, resp);
                        }
                    });
                }
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, verb);
}

void BluePrint::ROUTE(const char *route, const Handler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, handler, str_to_verb(method));
    }
}

void BluePrint::ROUTE(const char *route, int compute_queue_id, const Handler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, compute_queue_id, handler, str_to_verb(method));
    }
}

void BluePrint::GET(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::GET);
}

void BluePrint::GET(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::GET);
}

void BluePrint::POST(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::POST);
}

void BluePrint::POST(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::POST);
}

void BluePrint::DELETE(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::DELETE);
}

void BluePrint::DELETE(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::DELETE);
}

void BluePrint::PATCH(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::PATCH);
}

void BluePrint::PATCH(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PATCH);
}

void BluePrint::PUT(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::PUT);
}

void BluePrint::PUT(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PUT);
}

void BluePrint::HEAD(const char *route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::HEAD);
}

void BluePrint::HEAD(const char *route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::HEAD);
}

void BluePrint::ROUTE(const char *route, const SeriesHandler &handler, Verb verb)
{
    // lambda 表达式
    WrapHandler wrap_handler =
            [handler, this](const HttpReq *req,
                            HttpResp *resp,
                            SeriesWork *series) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    asp->before(req, resp);
                }
                handler(req, resp, series);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *) 
                    {
                        for(auto asp : global_aspect->aspect_list)
                        {
                            asp->after(req, resp);
                        }
                    });
                }
                return nullptr;
            };

    router_.handle(route, -1, wrap_handler, verb);
}

void BluePrint::ROUTE(const char *route, int compute_queue_id, const SeriesHandler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id, this](HttpReq *req,
                                        HttpResp *resp,
                                        SeriesWork *series) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    asp->before(req, resp);
                }
                WFGoTask *go_task = WFTaskFactory::create_go_task(
                        "wfrest" + std::to_string(compute_queue_id),
                        handler,
                        req,
                        resp,
                        series);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *) 
                    {
                        for(auto asp : global_aspect->aspect_list)
                        {
                            asp->after(req, resp);
                        }
                    });
                }
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, verb);
}

void BluePrint::ROUTE(const char *route, const SeriesHandler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, handler, str_to_verb(method));
    }
}

void BluePrint::ROUTE(const char *route, int compute_queue_id, 
            const SeriesHandler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, compute_queue_id, handler, str_to_verb(method));
    } 
}

void BluePrint::GET(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::GET);
}

void BluePrint::GET(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::GET);
}

void BluePrint::POST(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::POST);
}

void BluePrint::POST(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::POST);
}

void BluePrint::DELETE(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::DELETE);
}

void BluePrint::DELETE(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::DELETE);
}

void BluePrint::PATCH(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::PATCH);
}

void BluePrint::PATCH(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PATCH);
}

void BluePrint::PUT(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::PUT);
}

void BluePrint::PUT(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PUT);
}

void BluePrint::HEAD(const char *route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::HEAD);
}

void BluePrint::HEAD(const char *route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::HEAD);
}

void BluePrint::add_blueprint(const BluePrint &bp, const std::string &url_prefix)
{
    bp.router_.routes_map_.all_routes([this, &url_prefix]
    (const std::string &sub_prefix, VerbHandler verb_handler)
    {
        if (!url_prefix.empty() && url_prefix.back() == '/')
            verb_handler.path = url_prefix + sub_prefix;
        else
            verb_handler.path = url_prefix + "/" + sub_prefix;
        
        VerbHandler &vh = this->router_.routes_map_.find_or_create(verb_handler.path.c_str());
        vh = verb_handler;
    });
}

