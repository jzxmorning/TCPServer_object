#include"echo.hpp"
int main()
{
    Echo _server(8080);
    _server.Start();

    return 0;
}