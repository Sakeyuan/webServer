#pragma once
namespace Sake
{
    // http请求方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT
    };

    // http主状态机解析状态
    enum CHECK_STATE
    {
        // 正在解析请求行
        CHECK_STATE_REQUESTLINE = 0,

        // 正在解析请求头
        CHECK_STATE_HEADER,

        // 正在解析请求体
        CHECK_STATE_CONTENT
    };

    // http从状态机解析状态
    enum LINE_STATUS
    {
        // 读取到一个完整的行
        LINE_OK = 0,

        // 行出错LINE_BAD
        LINE_BAD,

        // 行数据不完整
        LINE_IMPERFECT
    };

    // http状态响应
    enum HTTP_CODE
    {
        // 报文不完整，需要继续读取客户数据
        NO_REQUEST,

        // 获取了完整请求
        GET_REQUEST,

        // 请求报文语法错误
        BAD_REQUEST,

        // 服务器没有资源
        NO_RESOURSE,

        // 禁止访问资源
        FORBIDDEN_REQUST,

        // 文件请求成功
        FILE_REQUEST,

        // 服务器内部错误
        INTERNAL_ERROR,

        // 客户端已关闭连接
        CLOSE_CONNECT,

        //登录响应和注册响应
        LOG_REG_RESPONSE,

    };
}
