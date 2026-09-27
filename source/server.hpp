#pragma once
#include "Log.hpp"
#include <vector>
#include <iostream>
#include <functional>
#include <assert.h>
#include <mutex>
#include <cstring>
#include <string>
#include <algorithm>
#include <sys/eventfd.h>
#include <unordered_map>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#define DEFAULT_BUFFER_SIZE 1024
using namespace LogMoudle;
class Buffer
{
private:
    std::vector<char> _buf; // 读写的容器；
    uint64_t _read_index;   // 读指针；
    uint64_t _write_index;  // 写指针；
public:
    Buffer()
        : _buf(DEFAULT_BUFFER_SIZE), _read_index(0), _write_index(0)
    {
    }
    char *Begin()
    {
        return &(*_buf.begin());
    }
    char *WritePosition()
    {
        return Begin() + _write_index;
    }
    char *ReadPosition()
    {
        return Begin() + _read_index;
    }
    uint64_t HeadIdleSize()
    {
        return _read_index;
    }
    uint64_t TailIdleSize()
    {
        return _buf.size() - _write_index;
    }
    uint64_t ReadAbleSize()
    {
        return _write_index - _read_index;
    }
    void MoveReadOffset(uint64_t len)
    {
        if (len == 0)
        {
            return;
        }
        assert(len <= ReadAbleSize());
        _read_index += len;
    }
    void MoveWriteOffset(uint64_t len)
    {
        assert(TailIdleSize() >= len);
        _write_index += len;
    }
    void EnsureWriteSpace(uint64_t len)
    {
        // 后面的内存本来就足够。
        if (len <= TailIdleSize())
        {
            return;
        }
        // 前面的空间加上后面的空间足够。
        if (len <= HeadIdleSize() + TailIdleSize())
        {
            uint64_t read = ReadAbleSize();
            std::copy(ReadPosition(), ReadPosition() + read, Begin());
            _read_index = 0;
            _write_index = read;
        }
        else
        {
            // 空间的确是不够了，需要我们进行扩容。
            _buf.resize(_write_index + len);
        }
    }
    void Write(const void *data, uint64_t len)
    {
        EnsureWriteSpace(len);
        const char *str = (const char *)data;
        std::copy(str, str + len, WritePosition());
    }
    void WriteAndPush(const void *data, uint64_t len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }
    void WriteString(const std::string &data)
    {
        const char *d = data.c_str();
        int len = data.size();
        Write(d, len);
    }
    void WriteStringAndPush(const std::string &data)
    {
        WriteString(data);
        MoveWriteOffset(data.size());
    }
    void WriteBuffer(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
    }
    void WriteBufferAndPush(Buffer &data)
    {
        WriteBuffer(data);
        MoveWriteOffset(data.ReadAbleSize());
    }
    void Read(void *buf, uint64_t len)
    {
        assert(ReadAbleSize() >= len);
        std::copy(ReadPosition(), ReadPosition() + len, (char *)buf);
    }
    void ReadAndPop(void *buf, uint64_t len)
    {
        Read(buf, len);
        MoveReadOffset(len);
    }
    std::string ReadAsString(uint64_t len)
    {
        assert(ReadAbleSize() >= len);
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }
    std::string ReadAsStringAndPop(uint64_t len)
    {
        assert(len <= ReadAbleSize());
        std::string str = ReadAsString(len);
        MoveReadOffset(len);
        return str;
    }
    char *FindCRLF()
    {
        char *ret = (char *)memchr(ReadPosition(), '\n', ReadAbleSize());
        return ret;
    }
    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (pos == nullptr)
        {
            return "";
        }
        else
        {
            return ReadAsString(pos - ReadPosition() + 1);
        }
    }
    std::string GetLineAndPop()
    {
        char *pos = FindCRLF();
        if (pos == nullptr)
        {
            return "";
        }
        else
        {
            std::string str = ReadAsString(pos - ReadPosition() + 1);
            MoveReadOffset(pos - ReadPosition() + 1);
            return str;
        }
    }
    void Clear()
    {
        _read_index = 0;
        _write_index = 0;
    }
};

#define MAX_LISTEN 1024
class Socket
{
private:
    int _sockfd;

public:
    Socket()
        : _sockfd(-1)
    {
    }
    Socket(int fd)
        : _sockfd(fd)
    {
    }
    int Fd()
    {
        return _sockfd;
    }
    ~Socket()
    {
        Close();
    }
    bool Create()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "socket create error";
            return false;
        }
        return true;
    }
    bool Bind(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(addr);
        int ret = bind(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            LOG(LogLevel::ERROR) << "bind error";
            return false;
        }
        return true;
    }
    bool Listen(int backlog = MAX_LISTEN)
    {
        int ret = listen(_sockfd, backlog);
        if (ret < 0)
        {
            LOG(LogLevel::ERROR) << "listen error";
            return false;
        }
        return true;
    }
    int Accept()
    {
        int fd = accept(_sockfd, nullptr, nullptr);
        if (fd < 0)
        {
            LOG(LogLevel::ERROR) << "accept error";
            return -1;
        }
        return fd;
    }
    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(addr);
        int ret = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            LOG(LogLevel::ERROR) << "connect error";
            return false;
        }
        return true;
    }
    ssize_t Recv(void *buf, size_t len, int flag = 0)
    {
        ssize_t n = recv(_sockfd, buf, len, flag);
        if (n <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                // 读到了数据的结尾，或者被信号中断读取。
                return 0;
            }
            else
            {
                LOG(LogLevel::ERROR) << "recv error";
                return -1;
            }
        }
        return n;
    }
    ssize_t NoBlockRcv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }
    ssize_t Send(const void *buf, size_t len, int flag = 0)
    {
        ssize_t ret = send(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            else
            {
                LOG(LogLevel::ERROR) << "send error";
                return -1;
            }
        }
        return ret;
    }
    ssize_t NoBlockSend(const void *buf, size_t len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }
    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    bool CreateServer(uint16_t port, const std::string &ip = "0.0.0.0", bool block_flag = false)
    {
        if (Create() == false)
        {
            return false;
        }
        if (block_flag)
            NonBlock();
        if (Bind(ip, port) == false)
        {
            return false;
        }
        if (Listen() == false)
        {
            return false;
        }
        ReuseAddress();
        return true;
    }
    bool CreateClient(uint16_t port, const std::string &ip)
    {
        if (Create() == false)
        {
            return false;
        }
        if (Connect(ip, port) == false)
        {
            return false;
        }
        return true;
    }
    // 设置套接字选项---开启地址端口重用
    void ReuseAddress()
    {
        // int setsockopt(int fd, int leve, int optname, void *val, int vallen)
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
    }
    void NonBlock()
    {
        // int fcntl(int fd, int cmd, ... /* arg */ );
        int flag = fcntl(_sockfd, F_GETFL, 0);
        fcntl(_sockfd, F_SETFL, flag | O_NONBLOCK);
    }
};

class Channel
{
public:
    Channel(int fd)
        : _fd(fd), _events(0), _revents(0)
    {
    }
    int Fd()
    {
        return _fd;
    }
    uint32_t Event()
    {
        return _events;
    }
    void SetRevent(uint32_t event)
    {
        _revents = event;
    }
    bool EventReadable()
    {
        return (_events & EPOLLIN);
    }
    bool EventWriteable()
    {
        return (_events & EPOLLOUT);
    }
    void SetReadEvent()
    {
        _events |= EPOLLIN;
        Update();
    }
    void SetWriteEvent()
    {
        _events |= EPOLLOUT;
        Update();
    }
    void RevReadEvent()
    {
        _events = _events | (~EPOLLIN);
        Update();
    }
    void RevWriteEvent()
    {
        _events = _events | (~EPOLLOUT);
        Update();
    }
    void RevAll()
    {
        _events = 0;
        Update();
    }
    void Update();
    void Rev();

private:
    int _fd;
    uint32_t _events;  // 需要监控的事件；
    uint32_t _revents; // 就绪的事件；
    using EventCallBack = std::function<void()>;
    EventCallBack ReadCallBack;   // 读事件就绪回调函数；
    EventCallBack WriteCallBack;  // 写事件就绪回调函数
    EventCallBack ErrorCallBack;  // 错误事件就绪回调函数；
    EventCallBack CloseCallBack;  // 关闭事件就绪回调函数；
    EventCallBack _EventCallBack; // 任意事件就绪回调函数；

public:
    void SetReadCallBack(const EventCallBack &cb)
    {
        ReadCallBack = cb;
    }
    void SetWriteCallBack(const EventCallBack &cb)
    {
        WriteCallBack = cb;
    }
    void SetErrorCallBack(const EventCallBack &cb)
    {
        ErrorCallBack = cb;
    }
    void SetCloseCallBack(const EventCallBack &cb)
    {
        CloseCallBack = cb;
    }
    void SetEvevtCallBack(const EventCallBack &cb)
    {
        _EventCallBack = cb;
    }
    void HandleEvent()
    {
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {
            if (ReadCallBack)
            {
                ReadCallBack();
            }
        }
        // 因为可能会释放连接所以只能执行一个。
        if (_revents & EPOLLOUT)
        {
            if (WriteCallBack)
            {
                WriteCallBack();
            }
        }
        else if (_revents & EPOLLERR)
        {
            if (ErrorCallBack)
            {
                ErrorCallBack(); // 在这样可能会直接释放连接；
            }
        }
        else if (_revents & EPOLLHUP)
        {
            if (CloseCallBack)
            {
                CloseCallBack();
            }
        }
        // 都要进行的回调函数。
        if (_EventCallBack)
        {
            _EventCallBack();
        }
    }
};

#define MAX_EPOLLEVENT 1024
class Poller
{
public:
    Poller()
    {
        _epfd = epoll_create1(MAX_EPOLLEVENT);
        if (_epfd < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_create error";
            abort();
        }
    }
    void Update(Channel *channel, int op)
    {
        int fd = channel->Fd();
        struct epoll_event env;
        env.data.fd = fd;
        env.events = channel->Event();
        int ret = epoll_ctl(_epfd, op, fd, &env);
        if (ret < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_ctrl error";
        }
        return;
    }
    bool HasChannel(Channel *target)
    {
        int fd = target->Fd();
        auto it = _channel.find(fd);
        if (it == _channel.end())
        {
            return false;
        }
        return true;
    }
    void UpdateEvent(Channel *channel)
    {
        bool ret = HasChannel(channel);
        if (ret == true)
        {
            // 修改事件监控
            return Update(channel, EPOLL_CTL_MOD);
        }
        else
        {
            _channel.insert({channel->Fd(), channel});
            return Update(channel, EPOLL_CTL_ADD);
        }
    }
    void RevEvent(Channel *channel)
    {
        auto it = _channel.find(channel->Fd());
        if (it != _channel.end())
        {
            _channel.erase(it);
        }
        Update(channel, EPOLL_CTL_DEL);
    }
    void Poll(std::vector<Channel *> *active)
    {
        int nfd = epoll_wait(_epfd, _env, MAX_EPOLLEVENT, -1);
        if (nfd < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_wait error";
            abort();
        }
        for (int i = 0; i < nfd; i++)
        {
            auto it = _channel.find(_env[i].data.fd);
            assert(it != _channel.end());
            it->second->SetRevent(_env[i].events);
            active->push_back(it->second);
        }
        return;
    }

private:
    int _epfd;
    struct epoll_event _env[MAX_EPOLLEVENT];
    std::unordered_map<int, Channel *> _channel;
};

using TaskFun = std::function<void()>;
using ReleaseFun = std::function<void()>;
class Task
{
public:
    Task(uint64_t id, uint32_t timeout, const TaskFun &fun)
        : _id(id), _timeout(timeout), _func(fun), _iscancel(false)
    {
    }
    ~Task()
    {
        if (_iscancel == false)
        {
            _func();
        }
        _release();
    }
    void Setcancel()
    {
        _iscancel = true;
    }
    void SetRelease(const ReleaseFun &cb)
    {
        _release = cb;
    }
    uint32_t Timeout()
    {
        return _timeout;
    }

private:
    uint64_t _id;        // 任务对象的ID；
    uint32_t _timeout;   // 时间超限的设置；
    bool _iscancel;      // 是否取消任务的执行；
    TaskFun _func;       // 任务的回调函数；
    ReleaseFun _release; // 清除在时间轮中的信息；
};

class TimeWhell
{
public:
    void RvmTimer(uint64_t id)
    {
        auto it = _timer.find(id);
        if (it != _timer.end())
        {
            _timer.erase(it);
        }
    }

public:
    void addtimewhell(uint64_t id, uint32_t timeout, const TaskFun &fun)
    {
        TaskPtr p(new Task(id, timeout, fun));
        p->SetRelease(std::bind(&TimeWhell::RvmTimer, this, id));
        _timer[id] = WeakPtr(p);
        int pos = (ticket + timeout) % capacity;
        _Timewhell[pos].push_back(p);
    }
    void flush(uint64_t id)
    {
        auto it = _timer.find(id);
        if (it != _timer.end())
        {
            TaskPtr p1 = it->second.lock();
            if (p1)
            {
                int pos = (ticket + p1->Timeout()) % capacity;
                _Timewhell[pos].push_back(p1);
            }
        }
        return;
    }
    void cancel(uint64_t id)
    {
        auto it = _timer.find(id);
        if (it != _timer.end())
        {
            TaskPtr p = it->second.lock();
            if (p)
            {
                p->Setcancel();
            }
        }
        return;
    }
    void Run()
    {
        ticket = (ticket + 1) % capacity;
        _Timewhell[ticket].clear();
    }
    TimeWhell()
        : ticket(0), capacity(60), _Timewhell(capacity)
    {
    }

private:
    using TaskPtr = std::shared_ptr<Task>;
    using WeakPtr = std::weak_ptr<Task>;
    int ticket;   // 指针;
    int capacity; // 大小
    std::vector<std::vector<TaskPtr>> _Timewhell;
    std::unordered_map<uint64_t, WeakPtr> _timer;
};

class EventLoop
{
public:
    void RunTask()
    {
        std::vector<Functor> func;
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            std::swap(func, _runv);
        }
        for (auto &V : func)
        {
            V();
        }
        return;
    }
    void ReadEvent()
    {
        uint64_t val;
        int ret = read(_event_fd, &val, sizeof(val));
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            LOG(LogLevel::ERROR) << "readevent error";
            abort();
        }
        return;
    }
    void WeakUpRun()
    {
        uint64_t val = 1;
        int ret = write(_event_fd, &val, sizeof(val));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            LOG(LogLevel::ERROR) << "weakup error";
            abort();
        }
        return;
    }
    void Start()
    {
        while (1)
        {
            std::vector<Channel *> active;
            _poll.Poll(&active);
            for (auto &V : active)
            {
                V->HandleEvent();
            }
            RunTask();
        }
    }

private:
    using Functor = std::function<void()>;
    std::thread::id _thread_id;
    int _event_fd;
    Channel _event_channel;
    std::vector<Functor> _runv;
    Poller _poll;
    std::mutex _mutex;

public:
    void QueueInLoop(const Functor &cb)
    {
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _runv.push_back(cb);
        }
        WeakUpRun();
    }
    bool IsInLoop()
    {
        return (_thread_id==std::this_thread::get_id());
    }
    void AssertInLoop()
    {
        assert(IsInLoop());
    }
    void RunInLoop(const Functor & cb)
    {
        if(IsInLoop())
        {
            cb();
        }else
        {
            QueueInLoop(cb);
        }
    }
    static int CreateEventFd()
    {
        int efd= eventfd(0,EFD_CLOEXEC | EFD_NONBLOCK);
        if(efd < 0 )
        {
             if (errno == EINTR || errno == EAGAIN) {
                    return;
            LOG(LogLevel::ERROR)<<"eventfd error";
            abort();
        }
        return efd;
    }
    }
    EventLoop()
    :_thread_id(std::this_thread::get_id())
    ,_event_fd(CreateEventFd())
    ,_event_channel(_event_fd)
    {
        _event_channel.SetReadCallBack(std::bind(&EventLoop::ReadEvent,this));
        _event_channel.SetReadEvent();
    }
    void EventUpdate_eventloop(Channel * channel)
    {
        _poll.UpdateEvent(channel);
    }
    void EventRev_eventloop(Channel* channel)
    {
        _poll.RevEvent(channel);
    }
};