#pragma once
#include "Log.hpp"
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
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
    :_sockfd(fd)
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
         if (block_flag) NonBlock();
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