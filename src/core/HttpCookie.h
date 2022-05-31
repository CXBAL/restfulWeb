#ifndef WFREST_HTTPCOOKIE_H_
#define WFREST_HTTPCOOKIE_H_

#include <string>
#include <map>
#include "Timestamp.h"
#include "StringPiece.h"
#include "Copyable.h"
namespace wfrest
{

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cookie
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Cookies

enum class SameSite
{
    DEFAULT, STRICT, LAX, NONE
};

inline const char *same_site_to_str(SameSite same_site)
{
    switch (same_site)
    {
        case SameSite::STRICT:
            return "Strict";
        case SameSite::LAX:
            return "Lax";
        case SameSite::NONE:
            return "None";
        default:
            return "";
    }
}

class HttpCookie : public Copyable
{
public:
    // Check if the cookie is empty
    // 检查 cookie 是否为空
    explicit operator bool() const
    { return (!key_.empty()) && (!value_.empty()); }

    std::string dump() const;

    static std::map<std::string, std::string> split(const StringPiece &cookie_piece);   // 将 cookie 切分成 键值对的形式

public:
    // 设置 cookie 的键
    HttpCookie &set_key(const std::string &key)
    {
        key_ = key;
        return *this;
    }

    HttpCookie &set_key(std::string &&key)
    {
        key_ = std::move(key);
        return *this;
    }

    // 设置 cookie 的值
    HttpCookie &set_value(const std::string &value)
    {
        value_ = value;
        return *this;
    }

    HttpCookie &set_value(std::string &&value)
    {
        value_ = std::move(value);
        return *this;
    }

    // 设置域（domain），指定 cookie 在哪个域下可以被接受
    // 如果不指定 domain 则默认是当前源（origin），但不包括子域名。如果指定了 Domain 则一般包含子域名（二级域名、三级域名）
    HttpCookie &set_domain(const std::string &domain)
    {
        domain_ = domain;
        return *this;
    }

    HttpCookie &set_domain(std::string &&domain)
    {
        domain_ = std::move(domain);
        return *this;
    }

    // 设置路径（path），指定 cookie 在当前主机下哪些路径可以接受 Cookie
    // 设置在 /login 下接受 Cookie。(只要是/login开头的都能接受,如：/login/user 等),此时 ‘/’ 下是没有 Cookie 的
    HttpCookie &set_path(const std::string &path)
    {
        path_ = path;
        return *this;
    }

    HttpCookie &set_path(std::string &&path)
    {
        path_ = std::move(path);
        return *this;
    }

    // 过期时间设置（Expires 字段）
    // 设置 Cookie 30s 后过期，则 30s 后浏览器 cookie 自动清空
    HttpCookie &set_expires(const Timestamp &expires)
    {
        expires_ = expires;
        return *this;
    }

    // 设置一段时间过期。（Max-Age）
    // 设置 Cookie 20s 后过期
    HttpCookie &set_max_age(int max_age)
    {
        max_age_ = max_age;
        return *this;
    }

    // 设置 secure 字段
    // 标记为 secure 的 Cookie 只应通过被 Https 协议加密过的请求发送给服务端。
    //（通过 https 创建的 Cookie 只能通过 Https 请求将 Cookie 携带到服务器，通过 http 无法拿到 Cookie）
    HttpCookie &set_secure(bool secure)
    {
        secure_ = secure;
        return *this;
    }

    // 设置不能通过 javascript 访问 Cookie。（HttpOnly字段）
    // 不能通过 document.cookie 访问
    HttpCookie &set_http_only(bool http_only)
    {
        http_only_ = http_only;
        return *this;
    }

    // 设置 sameSite ，它有三个可选值 None、strict、Lax
    // sameSite: None: 浏览器在同站请求、跨站请求下都会发送 Cookies
    // sameSite: Strict: 浏览器只会在相同站点下发送 Cookies
    // sameSite: Lax: 与 strict 类似，不同的是它可以从外站通过链接导航到该站。
    HttpCookie &set_same_site(SameSite same_site)
    {
        same_site_ = same_site;
        return *this;
    }

public:
    const std::string &key() const
    { return key_; }

    const std::string &value() const
    { return value_; }

    const std::string &domain() const
    { return domain_; }

    const std::string &path() const
    { return path_; }

    Timestamp expires() const
    { return expires_; }

    int max_age() const
    { return max_age_; }

    bool is_secure() const
    { return secure_; }

    bool is_http_only() const
    { return http_only_; }

    SameSite same_site() const
    { return same_site_; }

public:
    HttpCookie(const std::string &key, const std::string &value)
            : key_(key), value_(value)
    {}

    HttpCookie(std::string &&key, std::string &&value)
            : key_(std::move(key)), value_(std::move(value))
    {}

    HttpCookie() = default;

private:
    std::string key_;
    std::string value_;
    std::string domain_;
    std::string path_;

    Timestamp expires_;
    int max_age_ = 0;
    bool secure_ = false;
    bool http_only_ = false;

    SameSite same_site_ = SameSite::DEFAULT;
};

} // namespace wfrest




#endif // WFREST_HTTPCOOKIE_H_