#include<iostream>



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
    void SetRelease(ReleaseFun &cb)
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
        if(it!=_timeer.end())
        {
            _timeer.erase(it);
        }
    }
    public:
    void addtimewhell(uint64_t id,uint32_t timeout,const TaskFun& fun)
    {
        TaskPtr p(new Task(id,timeout,fun));
        p->SetRelease(std::bind(&TimeWhell::RvmTimer,this,id));
        _timeer[id]=WeakPtr(p);
        int pos = (ticket+timeout)%capacity;
        _Timewhell[pos].push_back(p);
    }
    void flush(uint64_t id)
    {
        auto it=_timer.find(id);
        if(it!=_timeer.end())
        {
           TaskPtr p1=it->second.lock();
           int pos =(ticket+p1->timeout)%capacity;
           _timeer[pos].push_back(p1);
        }
        return;
    }
    void cancel(uint64_t id)
    {
        auto it =_timeer.find(id);
        if(it!=_timeer.end())
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
    private:
    using TaskPtr = std::shared_ptr<Task>;
    using WeakPtr = std::weak_ptr<Task>;
    int ticket;//指针;
    int capacity;//大小
    std::vactor<std::vector<TaskPtr>> _Timewhell;
    std::unordered_map<int,WeakPtr> _timer;
};