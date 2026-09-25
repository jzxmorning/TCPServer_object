#include <iostream>
#include "server.hpp"
#include <string>
int main()
{
 Socket client;
 client.CreateClient(8080,"127.0.0.1");
std::string str ="hello";
 client.Send(str.c_str(),str.size()+1);
    return 0;
}