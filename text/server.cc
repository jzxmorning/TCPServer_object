#include <iostream>
#include "server.hpp"
#include <string>
int main()
{
    //     Buffer test;
    //     std::string str = "hello";
    //    test.WriteStringAndPush(str);
    //    std::string temp=test.ReadAsStringAndPop(5);
    //    std::cout<<test.ReadAbleSize()<<std::endl;
    //    std::cout<<temp<<std::endl;
    Socket server;
    server.CreateServer(8080);
    int newfd = server.Accept();
    Socket temp(newfd);
    char str[1024] = {0};
    ssize_t n = temp.Recv(str, sizeof(str) - 1);
    if (n > 0)
    {
        str[n] = '\0';
    }
    std::cout << str << std::endl;
    return 0;
}