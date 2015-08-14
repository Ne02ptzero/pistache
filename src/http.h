/* http.h
   Mathieu Stefani, 13 August 2015
   
   Http Layer
*/

#pragma once

#include <type_traits>
#include <stdexcept>
#include "listener.h"
#include "net.h"
#include "http_headers.h"

namespace Net {

namespace Http {

#define HTTP_METHODS \
    METHOD(Options, "OPTIONS") \
    METHOD(Get, "GET") \
    METHOD(Post, "POST") \
    METHOD(Head, "HEAD") \
    METHOD(Put, "PUT") \
    METHOD(Delete, "DELETE") \
    METHOD(Trace, "TRACE") \
    METHOD(Connect, "CONNECT")

// 10. Status Code Definitions
#define STATUS_CODES \
    CODE(100, Continue, "Continue") \
    CODE(101, Switching_Protocols, "Switching Protocols") \
    CODE(200, Ok, "OK") \
    CODE(201, Created, "Created") \
    CODE(202, Accepted, "Accepted") \
    CODE(203, NonAuthoritative_Information, "Non-Authoritative Information") \
    CODE(204, No_Content, "No Content") \
    CODE(205, Reset_Content, "Reset Content") \
    CODE(206, Partial_Content, "Partial Content") \
    CODE(300, Multiple_Choices, "Multiple Choices") \
    CODE(301, Moved_Permanently, "Moved Permanently") \
    CODE(302, Found, "Found") \
    CODE(303, See_Other, "See Other") \
    CODE(304, Not_Modified, "Not Modified") \
    CODE(305, Use_Proxy, "Use Proxy") \
    CODE(307, Temporary_Redirect, "Temporary Redirect") \
    CODE(400, Bad_Request, "Bad Request") \
    CODE(401, Unauthorized, "Unauthorized") \
    CODE(402, Payment_Required, "Payment Required") \
    CODE(403, Forbidden, "Forbidden") \
    CODE(404, Not_Found, "Not Found") \
    CODE(405, Method_Not_Allowed, "Method Not Allowed") \
    CODE(406, Not_Acceptable, "Not Acceptable") \
    CODE(407, Proxy_Authentication_Required, "Proxy Authentication Required") \
    CODE(408, Request_Timeout, "Request Timeout") \
    CODE(409, Conflict, "Conflict") \
    CODE(410, Gone, "Gone") \
    CODE(411, Length_Required, "Length Required") \
    CODE(412, Precondition_Failed, "Precondition Failed") \
    CODE(413, Request_Entity_Too_Large, "Request Entity Too Large") \
    CODE(414, RequestURI_Too_Long, "Request-URI Too Long") \
    CODE(415, Unsupported_Media_Type, "Unsupported Media Type") \
    CODE(416, Requested_Range_Not_Satisfiable, "Requested Range Not Satisfiable") \
    CODE(417, Expectation_Failed, "Expectation Failed") \
    CODE(500, Internal_Server_Error, "Internal Server Error") \
    CODE(501, Not_Implemented, "Not Implemented") \
    CODE(502, Bad_Gateway, "Bad Gateway") \
    CODE(503, Service_Unavailable, "Service Unavailable") \
    CODE(504, Gateway_Timeout, "Gateway Timeout")


enum class Method {
#define METHOD(m, _) m, 
    HTTP_METHODS
#undef METHOD
};

enum class Code {
#define CODE(value, name, _) name = value,
    STATUS_CODES
#undef CODE
};

enum class Version {
    Http10, // HTTP/1.0
    Http11 // HTTP/1.1
};

const char* methodString(Method method);
const char* codeString(Code code);

// 4. HTTP Message
class Message {
public:
    Message();
    Version version;

    Headers headers;
    std::string body;
};

// 5. Request
class Request : public Message {
public:
    Request();

    Method method;
    std::string resource;
};

// 6. Response
class Response : public Message {
public:
    Response(int code, std::string body);
    Response(Code code, std::string body);

    void writeTo(Tcp::Peer& peer);
    std::string mimeType;

private:
    int code_;
};

namespace Private {

    struct ParsingError : public std::runtime_error {
        ParsingError(const char* msg) : std::runtime_error(msg) { }
    };

    struct Parser {

        Parser(const char* buffer, size_t len)
            : buffer(buffer)
            , len(len)
            , cursor(0)
            , contentLength(-1)
        { }

        Request expectRequest();
        void expectRequestLine();
        void expectHeaders();
        void expectBody();

        void advance(size_t count);
        bool eol() const;

        const char* buffer;
        size_t len;
        size_t cursor;

        char next() const;
    private:
        void raise(const char* msg) const;
        ssize_t contentLength;
        Request request;
    };

    struct Writer {
        Writer(char* buffer, size_t len) 
            : buf(buffer)
            , len(len)
        { }

        ssize_t writeRaw(const void* data, size_t len);
        ssize_t writeString(const char* str);

        template<typename T>
        typename std::enable_if<
                     std::is_integral<T>::value, ssize_t
                  >::type
        writeInt(T value) {
            auto str = std::to_string(value);
            return writeRaw(str.c_str(), str.size());
        }

        ssize_t writeChar(char c) {
            *buf++ = c;
            return 0;
        }

        ssize_t writeHeader(const char* name, const char* value);

        template<typename T>
        typename std::enable_if<
                    std::is_arithmetic<T>::value, ssize_t
                 >::type
        writeHeader(const char* name, T value) {
            auto str = std::to_string(value);
            return writeHeader(name, str.c_str());
        }

        char *cursor() const { return buf; }

    private:
        char* buf;
        size_t len;
    };

}

class Handler : public Net::Tcp::Handler {
public:
    void onInput(const char* buffer, size_t len, Tcp::Peer& peer);
    void onOutput();

    virtual void onRequest(const Request& request, Tcp::Peer& peer) = 0;
};

class Server {
public:
    Server();
    Server(const Net::Address& addr);

    void setHandler(const std::shared_ptr<Handler>& handler);
    void serve();

private:
    std::shared_ptr<Handler> handler_;
    Net::Tcp::Listener listener;
};

} // namespace Http

} // namespace Net


