#include "ctrx.h"
#include "../GlobalDefinitions.h"

CTRx::CTRx(QThread *parent) : QThread(parent)
{

}


/*在 Qt 的信号槽机制中，
 * !!发射信号时传递的参数会被复制或移动 !!，
 * 因此局部变量可以作为信号的参数发送过去。
 * 只要信号参数类型和槽函数的参数类型匹配，
 * 并且在信号发射时数据没有丢失或未定义行为，使用局部变量是安全的。
*/
void CTRx::SetOutputDevice_(QString dev)
{
    DEBUG_COUT("CTRx::SetOutputDevice");
    std::string output_dev = dev.toStdString();

    //const std::string&
    emit OutputDeviceChanged(dev);
    emit OutputDeviceChanged(output_dev);

    DEBUG_COUT("***emit OutputDeviceChanged(QString s)****");
    DEBUG_COUT("***emit OutputDeviceChanged(string s)****");
}

