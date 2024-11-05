#ifndef GLOBALVARIABLES__H
#define GLOBALVARIABLES__H
#include "managementsource.h"


class GlobalVariables_
{
public:
    GlobalVariables_();
};

// 定义一个全局的 std::vector 用于存储 IPAddress 结构体
extern std::vector<IPAddress> ipList;
void printIPList();

//bool isIPAddressInList(const IPAddress& address);


#endif // GLOBALVARIABLES__H
