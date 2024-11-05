/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod
 *
 * Description:
 *
 * 11/10/2004 Stephane Fillod
 *	- QT translation
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <QTranslator>
#include <QThread>
#include <QApplication>
#include <QMessageBox>

#ifdef _WIN32
# include <windows.h>
#else
# include <csignal>
#endif
#include <iostream>

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "../util/Settings.h"
# include "fdrmdialog.h"
# include "TransmDlg.h"
# include "DialogUtil.h"

#include <pthread.h>

#include <unistd.h>
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#ifdef USE_OPENSL
# include <SLES/OpenSLES.h>
SLObjectItf engineObject = nullptr;
#endif

#include "../main-Qt/crx.h"
#include "../main-Qt/ctx.h"


//消息队列
//#include "../messagequeue.h"

#include "../MDI/managementsource.h"
#include "../MDI/cmaindrmreceiver.h"


#include "../csubdrmreceiver.h"

#include "../receivermanager.h" // 管理类


int main(int argc, char **argv)
{

    QApplication app(argc, argv);


#ifdef USE_OPENSL
    (void)slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    (void)(*engineObject)->Realize(engineObject, SLbool_false);
#endif
#if defined(__unix__) && !defined(__APPLE__)
    /* Prevent signal interaction with popen */
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGPIPE);
    sigaddset(&sigset, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
#endif


#if defined(__APPLE__)
    /* find plugins on MacOs when deployed in a bundle */
    app.addLibraryPath(app.applicationDirPath()+"../PlugIns");
#endif
#ifdef _WIN32
    WSADATA wsaData;
    (void)WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    /* Load and install multi-language support (if available) */
    QTranslator translator(nullptr);
    if (translator.load("dreamtr"))
        app.installTranslator(&translator);

    CSettings Settings;
    /* Parse arguments and load settings from init-file */
    Settings.Load(argc, argv);  //

    // home/linux/code_wode/001zhou/build-dream-Desktop_Qt_5_12_8_GCC_64bit-Debug
    //qDebug() << "Current working directory:" << QDir::currentPath();


    RingBuffer ringBuffer;          // RingBuffer 开辟空间
    ringBuffer.ring_buffer_init();

    cout<<"new  project!"<<endl;
    try
    {

        string mode = Settings.Get("command", "mode", string());

        // 管理类 (程序只有一个实例)
        ReceiverManager receiverManager;

        // 实例化 发射机
        CDRMTransmitter DRMTransmitter(&Settings);
        DRMTransmitter.GetTransData()->m_Loopback.pRingbuff = &ringBuffer;  //环形缓冲区
        DRMTransmitter.pReceiverManager_m = &receiverManager;           // 源管理类


        // 添加到管理类中
        receiverManager.addSubReceiver("192.168.0.193", // UDP IP
                                       "192.168.0.193", // local IP
                                       50000,     // 端口号
                                       1, // 索引号
                                       Settings);




        // 获取 管理类中的管理的子类（ 根据索引 ）
        CSubDRMReceiver* SubDRMReceiver1 = receiverManager.getSubReceiver(1);
        if(SubDRMReceiver1 == nullptr)
        {
            cerr<<"获取 管理类中的管理的子类 error;"<< endl;
        }
        else
        {
            // 打印实例的地址
            // SubDRMReceiver1 地址: 0x55c21c358e60 (指针 地址都是一样的)
            SubDRMReceiver1->LoadSettings();
            // 启动子线程
            SubDRMReceiver1->pthread_ReadData_start();

            {
                // wait子线程先起来
                std::unique_lock<std::mutex> lock(SubDRMReceiver1->m_mutex);
                SubDRMReceiver1->m_condition.wait(lock,
                                                  [&] { return SubDRMReceiver1->m_ready; });  // 等待子线程完成启动
            }

        }

        if (mode == "receive")
        {
            // CDRMReceiver DRMReceiver(&Settings); // 源代码

            CMainDRMReceiver DRMReceiver(&Settings);
            DRMReceiver.pDRMTransmitter = &DRMTransmitter;  // 把 发射机的指针 给接收机成员变量
            DRMReceiver.m_LoopBack.pRingbuff = &ringBuffer; //  ring 给 接收机

            /* First, initialize the working thread. This should be done in an extra
               routine since we cannot 100% assume that the working thread is
               ready before the GUI thread */

#ifdef HAVE_LIBHAMLIB
            CRig rig(DRMReceiver.GetParameters());
            rig.LoadSettings(Settings); // must be before DRMReceiver for G313
#endif
            CRx rx( DRMReceiver);  //SubDRMReceiver1  DRMReceiver

#ifdef HAVE_LIBHAMLIB
            DRMReceiver.SetRig(&rig);

            if(DRMReceiver.GetDownstreamRSCIOutEnabled())
            {
                rig.subscribe();
            }
            FDRMDialog *pMainDlg = new FDRMDialog(&rx, Settings, rig);
#else
            // 接收机 主页面
            FDRMDialog *pMainDlg = new FDRMDialog(&rx, Settings);
#endif
            rx.LoadSettings();  // load settings after GUI initialised so LoadSettings signals get captured

            /* Start working thread */
            rx.start();

            pMainDlg->show();
            /* Set main window */

#if 1
            // 发射机 主界面
            DRMTransmitter.LoadSettings();

            CTx tx(DRMTransmitter);

            TransmDialog* pTrMainDlg = new TransmDialog(tx);
            pTrMainDlg->pReceiverManager = &receiverManager;
            if(pTrMainDlg->pReceiverManager == nullptr)
            {
                cerr<<"警告错误: pTrMainDlg->pReceiverManager == nullptr"<<endl;
            }
            if(0)
            {
                //现在不用了
                //            QObject::connect(pTrMainDlg->pushButton_updata_menu, &QPushButton::clicked,
                //                             pMainDlg, &FDRMDialog::receiveParameters);

                //            QObject::connect(pMainDlg , &FDRMDialog::parametersReceived,
                //                             pTrMainDlg, &TransmDialog::onParametersReceived);

            }

            /* ---------------------  刷新源 按钮   --------------------------- */
            QObject::connect(pTrMainDlg->pushButton_updata_menu, &QPushButton::clicked,
                             pTrMainDlg, &TransmDialog::onParametersReceived);

            tx.LoadSettings();

            pTrMainDlg->show();


            app.exec();//

            rx.SaveSettings();
            tx.SaveSettings();

#ifdef HAVE_LIBHAMLIB
            //源码
            if(DRMReceiver.GetDownstreamRSCIOutEnabled())
                pTrMainDlg->onParametersReceived(rx);
            {
                rig.unsubscribe();
            }
            rig.SaveSettings(Settings);

#endif

#endif


        }
        else if(mode == "transmit")
        {
            CDRMTransmitter DRMTransmitter(&Settings);
            CTx tx(DRMTransmitter);
            // 发射机 主界面
            TransmDialog* pMainDlg = new TransmDialog(tx);

            tx.LoadSettings(); // load settings after GUI initialised so LoadSettings signals get captured
            /* Show dialog */
            pMainDlg->show();
            // tx.start();
            app.exec();
            tx.SaveSettings();
        }
        else
        {
            CHelpUsage HelpUsage(Settings.UsageArguments(), argv[0]);
            app.exec();
            exit(0);
        }
    }
    catch(CGenErr GenErr)
    {
        qDebug("%s", GenErr.strError.c_str());
    }
    catch(string strError)
    {
        qDebug("%s", strError.c_str());
    }
    catch(char *Error)
    {
        qDebug("%s", Error);
    }

    /* Save settings to init-file */
    Settings.Save();

    return 0;
}
