#include <muduo/net/TcpServer.h>
#include <functional>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include"json.hpp"
using namespace muduo;
using namespace muduo::net;
using namespace std;
using namespace placeholders;
using namespace std;
using json=nlohmann::json;
// 基于muduo网络库服务器程序
// 1.组合TcpServer对象
// 2.创建EventLoop事件循环对象的指针
// 3.明确TcpServer构造对象需要什么参数，输出ChatServer的构造函数
// 4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
// 5.设置合适的服务器端线程数量，muduo库会自己分配I/O线程和workker线程

// epoll +线程池
// 好处：能够吧网络I/O的代码和业务代码区分开
// 用户的连接和断开  用户的可读写事件
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+Port
               const string &nameArg)         // 服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 绑定this指针用于访问成员变量
        // 给服务器注册用户读写时间回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // 设置服务器端的线程数量 1个I/O线程，3个worker线程
        _server.setThreadNum(4);
    }
    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // TcpServer绑定的回调函数，当有新连接或连接中断时调用
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state:online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state:offline" << endl;
            //客户端断开链接
            conn->shutdown(); // close(fd)
            //_loop->quit()
        }
    }
    // TcpServer绑定的回调函数，当有新数据时调用
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buffer,
                   muduo::Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        //数据的反序列化
        json js=json::parse(buf);
        //通过js["msgid"]
        //达到的目的：完全解耦网络模块的代码和业务模块的代码
        cout << "recv data: " << buf << " time: " << time.toString() << endl;
        conn->send(buf);
    }

private:
    TcpServer _server; // #1
    EventLoop *_loop;  // #2 epoll
};

int main()
{
    EventLoop loop; // epoll
    InetAddress addr("192.168, 100, 126", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop(); // epoll wait以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;
}