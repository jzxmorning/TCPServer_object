#include<iostream>
#include"server.hpp"
#include<string>
int main()
{
    Buffer test;
    std::string str = "hello";
   test.WriteStringAndPush(str);
   std::string temp=test.ReadAsStringAndPop(5);
   std::cout<<test.ReadAbleSize()<<std::endl;
   std::cout<<temp<<std::endl;
    return 0;
}