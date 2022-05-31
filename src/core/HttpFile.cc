#include "workflow/WFTaskFactory.h"

#include <sys/stat.h>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"
#include "HttpServerTask.h"
#include "FileUtil.h"
#include "ErrorCode.h"

using namespace wfrest;

namespace
{
/*
我们不占用任何线程来读取文件，而是生成一个异步文件读取任务 并 在阅读完成后回复请求。

在回复信息之前，我们需要将全部数据读入内存。因此，它不适合传输太大的文件。

todo:有没有更好的传输大文件的方法？
*/
// 异步读的回调函数
void pread_callback(WFFileIOTask *pread_task)
{
    FileIOArgs *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *resp = static_cast<HttpResp *>(pread_task->user_data);

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileReadError);
    } else
    {
        resp->append_output_body_nocopy(args->buf, ret);
    }
}

// 异步写的回调函数
void pwrite_callback(WFFileIOTask *pwrite_task)
{
    long ret = pwrite_task->get_retval();
    HttpServerTask *server_task = task_of(pwrite_task);
    HttpResp *resp = server_task->get_resp();
    delete static_cast<std::string *>(pwrite_task->user_data);

    if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileWriteError);
    } else
    {
        resp->append_output_body_nocopy("Save File success\n", 18);
    }
}

}  // namespace

// 服务器 给 客户端 发送文件
// note : [start, end)
int HttpFile::send_file(const std::string &path, size_t file_start, size_t file_end, HttpResp *resp)
{
    if(!PathUtil::is_file(path))
    {
        return StatusNotFound;
    }
    int start = file_start;
    int end = file_end;
    if (end == -1 || start < 0)
    {
        size_t file_size;
        int ret = FileUtil::size(path, OUT &file_size);

        if (ret != StatusOK)
        {
            return ret;
        }
        if (end == -1) end = file_size;
        if (start < 0) start = file_size + start;
    }

    if (end <= start)
    {
        return StatusFileRangeInvalid;
    }

    http_content_type content_type = CONTENT_TYPE_NONE;
    std::string suffix = PathUtil::suffix(path);
    if(!suffix.empty())
    {
        content_type = ContentType::to_enum_by_suffix(suffix);
    }
    if (content_type == CONTENT_TYPE_NONE || content_type == CONTENT_TYPE_UNDEFINED) {
        content_type = APPLICATION_OCTET_STREAM;
    }
    resp->headers["Content-Type"] = ContentType::to_str(content_type);

    size_t size = end - start;
    void *buf = malloc(size);

    HttpServerTask *server_task = task_of(resp);
    server_task->add_callback([buf](HttpTask *server_task)
                              {
                                  free(buf);
                              });
    // https://datatracker.ietf.org/doc/html/rfc7233#section-4.2
    // Content-Range: bytes 42-1233/1234
    resp->headers["Content-Range"] = "bytes " + std::to_string(start)
                                            + "-" + std::to_string(end)
                                            + "/" + std::to_string(size);

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(path,
                                                                buf,
                                                                size,
                                                                static_cast<off_t>(start),
                                                                pread_callback);
    pread_task->user_data = resp;  
    **server_task << pread_task;
    return StatusOK;
}

// 服务器接收文件
// content 参数：左值引用形式
void HttpFile::save_file(const std::string &dst_path, const std::string &content, HttpResp *resp)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_content = new std::string;
    *save_content = content;

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_content->c_str()),
                                                                  save_content->size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    pwrite_task->user_data = save_content;
}

// 服务器接收文件
// content 参数：右值引用形式
void HttpFile::save_file(const std::string &dst_path, std::string &&content, HttpResp *resp)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_content = new std::string;
    *save_content = std::move(content);

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_content->c_str()),
                                                                  save_content->size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    pwrite_task->user_data = save_content;
}




