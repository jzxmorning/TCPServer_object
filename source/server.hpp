#pragma once
#include "Log.hpp"
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstring>
#include<string>
#include <algorithm>
#define DEFAULT_BUFFER_SIZE 1024
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
        char * ret = (char*)memchr(ReadPosition(),'\n',ReadAbleSize());
        return ret;
    }
    std::string GetLine()
    {
        char * pos = FindCRLF();
        if(pos==nullptr)
        {
            return "";
        }else
        {
            return ReadAsString(pos-ReadPosition()+1);
        }
    }
    std::string GetLineAndPop()
    {   
        char * pos = FindCRLF();
        if(pos==nullptr)
        {
            return "";
        }else
        {
            std::string str=ReadAsString(pos-ReadPosition()+1);
            MoveReadOffset(pos-ReadPosition()+1);
            return str;
        }
    }
    void Clear()
    {
        _read_index=0;
        _write_index=0;
    }
};

