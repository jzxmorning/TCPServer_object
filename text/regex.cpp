#include<iostream>
#include<regex>
int main()
{
    std::string str="get /baidu/login?user=xiaoming&pass=123123 HTTP/1.1\r\n";
    std::smatch matches;
    std::regex res("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP\\/1\\.[01])(?:\n|\r\n)?",std::regex::icase);
    bool ret = std::regex_match(str,matches,res);
    if (ret == false) {
        return -1;
    }
    std::string method = matches[1];
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    std::cout << method << std::endl;
    for (int i = 0; i < matches.size(); i++) {
        std::cout << i << " : ";
        std::cout << matches[i] << std::endl;
    }
    return 0;
}