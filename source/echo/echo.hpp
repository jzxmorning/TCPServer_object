#include"../server.hpp"
class Echo
{
    private:
    TcpServer _server;
    private:
    void OnConnected(const PtrConnection& conn)
    {
        LOG(LogLevel::DEBUG)<<"OnConnected %p"<<conn.get();
    }

     void OnClose(const PtrConnection& conn)
    {
        LOG(LogLevel::DEBUG)<<"OnClose %p"<<conn.get();
    }

    void OnMessage(const PtrConnection& conn, Buffer *buf)
    {
        conn->Send(buf->ReadPosition(),buf->ReadAbleSize());
        buf->MoveReadOffset(buf->ReadAbleSize());
        conn->Shutdown();
    }

    public:
    Echo(int port)
    :_server(port)
    {
        _server.SetThreadCount(2);
        _server.EnableInactiveRelease(10);
        _server.SetConnectedCallback(std::bind(&Echo::OnConnected,this,std::placeholders::_1));
        _server.SetClosedCallback(std::bind(&Echo::OnClose,this,std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&Echo::OnMessage,this,std::placeholders::_1,std::placeholders::_2));
    }
    void Start()
    {
        _server.Start();
    }
};
