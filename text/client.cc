#include "../source/server.hpp"
int main()
{
    Socket cli_sock;
    cli_sock.CreateClient(8080, "127.0.0.1");
    for (int i = 0; i < 5; i++) {
        std::string str = "hello";
        cli_sock.Send(str.c_str(), str.size());
        char buf[1024] = {0};
        cli_sock.Recv(buf, 1023);
        LOG(LogLevel::DEBUG)<<buf;
        sleep(1);
    }
    while(1) sleep(1);
    return 0;
}