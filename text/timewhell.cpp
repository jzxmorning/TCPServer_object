#include<iostream>
#include<vector>
#include<unordered_map>
#include <functional>
#include <memory>
#include <unistd.h>
#include <cstdint>


using TaskFun =std::function<void()>;
using ReleaseFun =std::function<void()>;
class Task
{
    public:
    Task(uint64_t id,uint32_t timeout,const TaskFun& fun)
    :_id(id)
    ,_timeout(timeout)
    ,_func(fun)
    ,_iscancel(false)
    {
    }
    ~Task()
    {
        if(_iscancel==false)
        {
            _func();
        }
        _release();
    }
    void Setcancel()
    {
        _iscancel=true;
    }
    void SetRelease(const ReleaseFun &cb)
    {
        _release=cb;
    }
    uint32_t Timeout()
    {
        return _timeout;
    }
    private:
    uint64_t _id; //任务对象的ID；
    uint32_t _timeout; //时间超限的设置；
    bool _iscancel; //是否取消任务的执行；
    TaskFun _func; //任务的回调函数；
    ReleaseFun _release; //清除在时间轮中的信息；
};

class TimeWhell
{
    public:

    void RvmTimer(uint64_t id)
    {
        auto it=_timer.find(id);
        if(it!=_timer.end())
        {
            _timer.erase(it);
        }
    }
    public:
    void addtimewhell(uint64_t id,uint32_t timeout,const TaskFun& fun)
    {
        TaskPtr p(new Task(id,timeout,fun));
        p->SetRelease(std::bind(&TimeWhell::RvmTimer,this,id));
        _timer[id]=WeakPtr(p);
        int pos = (ticket+timeout)%capacity;
        _Timewhell[pos].push_back(p);
    }
    void flush(uint64_t id)
    {
        auto it=_timer.find(id);
        if(it!=_timer.end())
        {
           TaskPtr p1=it->second.lock();
           if(p1)
           {
            int pos =(ticket+p1->Timeout())%capacity;
           _Timewhell[pos].push_back(p1);
           }
        }
        return;
    }
    void cancel(uint64_t id)
    {
        auto it =_timer.find(id);
        if(it!=_timer.end())
        {
            TaskPtr p=it->second.lock();
            if(p)
            {
                p->Setcancel();
            }
        }
        return;
    }
    void Run()
    {
        ticket=(ticket+1)%capacity;
        _Timewhell[ticket].clear();
    }
    TimeWhell()
    :ticket(0)
    ,capacity(60)
    ,_Timewhell(capacity)
    {
    }
    private:
    using TaskPtr = std::shared_ptr<Task>;
    using WeakPtr = std::weak_ptr<Task>;
    int ticket;//指针;
    int capacity;//大小
    std::vector<std::vector<TaskPtr>> _Timewhell;
    std::unordered_map<uint64_t,WeakPtr> _timer;
};

class T
{
    public:
    T()
    {
        std::cout<<"构造函数"<<std::endl;
    }
    ~T()
    {
        std::cout<<"析构函数"<<std::endl;
    }
};


void task(T* text)
{
    delete text;
}
int main()
{
    TimeWhell t;
    T * text=new T;
    t.addtimewhell(1,5,std::bind(task,text));
    int cnt=3;
    while(cnt--)
    {
        t.Run();
        t.flush(1);
        std::cout<<"1 号任务推迟"<<std::endl;
        sleep(1);
    }
    while(1)
    {
        t.Run();
        std::cout<<"-----------------------"<<std::endl;
         sleep(1);
    }
    return 0;
}