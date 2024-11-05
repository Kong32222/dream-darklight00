/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
 *
 * Description:
 *
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

#include "fdrmdialog.h"
#include <iostream>
#include <QFileInfo>
#include <QWhatsThis>
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QFileDialog>
#include "SlideShowViewer.h"
#include "JLViewer.h"
#ifdef QT_WEBENGINE_LIB
# include "BWSViewer.h"
#endif
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#include "../Scheduler.h"
#include "../util-QT/Util.h"
void SetAxisColor(QwtPlot *plot, const QColor &color)
{
    // x轴 y轴  QwtPlot::yLeft,QwtPlot::xBottom,
    QwtScaleWidget *scaleWidget = plot->axisWidget(QwtPlot::yLeft);
    QwtScaleWidget *scaleWidget2 = plot->axisWidget(QwtPlot::xBottom);
    if (scaleWidget)
    {
        QPalette palette = scaleWidget->palette();
        palette.setColor(QPalette::WindowText, color);
        scaleWidget->setPalette(palette);
    }
    if (scaleWidget2)
    {
        QPalette palette2 = scaleWidget2->palette();
        palette2.setColor(QPalette::WindowText, color);
        scaleWidget2->setPalette(palette2);
    }
}

void SetAxisColor(QwtPlot *plot, int axis, int axis2, const QColor &color)
{

    QwtScaleWidget *scaleWidget = plot->axisWidget(axis);
    QwtScaleWidget *scaleWidget2 = plot->axisWidget(axis2);
    if (scaleWidget)
    {
        QPalette palette = scaleWidget->palette();
        palette.setColor(QPalette::WindowText, color);
        scaleWidget->setPalette(palette);
    }
    if (scaleWidget2)
    {
        QPalette palette = scaleWidget2->palette();
        palette.setColor(QPalette::WindowText, color);
        scaleWidget2->setPalette(palette);
    }
}
// Simone's values
// static _REAL WMERSteps[] = {8.0, 12.0, 16.0, 20.0, 24.0};
// David's values
static _REAL WMERSteps[] = {6.0, 12.0, 18.0, 24.0, 30.0};

/* Implementation *************************************************************/
#ifdef HAVE_LIBHAMLIB
FDRMDialog::FDRMDialog(CTRx* pRx, CSettings& Settings, CRig& rig, QWidget* parent)
#else
FDRMDialog::FDRMDialog(CTRx* pRx,
                       CSettings& Settings,
                       QWidget* parent)
#endif
    :
      CWindow(parent, Settings, "DRM"),
      rx(*reinterpret_cast<CRx*>(pRx)),
      done(false),
      serviceLabels(4), pLogging(nullptr),
      pSysTray(nullptr), pCurrentWindow(this),
      iMultimediaServiceBit(0),
      iLastMultimediaServiceSelected(-1),

      pScheduler(nullptr),
      pScheduleTimer(nullptr)
{
    setupUi(this);
    /* Set help text for the controls */
    AddWhatsThisHelp();

    // 水平
    max_soundbar_Size = QSize(1059,123);
    min_soundbar_Size = QSize(988,113);


    // Frame syn
    Frame_syn = 0;

    //FAC SDC MSC LED
    //label_LED_FAC->setAutoFillBackground(true);  // 使背景颜色填充标签


    //自定义  检测主界面的所有 参数指标
    connect(&LED_timer, &QTimer::timeout, this, &FDRMDialog::updateLabelColor);
    LED_timer.start(400);

    /**  menu_source  输入信息源   QAction  ***/
    connect(this->action_MDI , &QAction::triggered, this,[=]()
    {
        QMessageBox::information(nullptr, "输入信息源", "成功修改输入信息源:MDI");
        this->label_Signal_source->setText(tr("当前输入信息源:MDI"));
    });

    connect(this->action_file_ , &QAction::triggered, this,
            &FDRMDialog::OnOpenFile_soure);
    connect(this->action_sound_card , &QAction::triggered, this, [=]()
    {
        QMessageBox::information(nullptr, "输入信息源", "成功修改输入信息源:声卡");
        this->label_Signal_source->setText(tr("当前输入信息源:声卡"));
    });
    connect(this->actionSDR , &QAction::triggered, this,[=]()
    {
        QMessageBox::information(nullptr, "输入信息源", "成功修改输入信息源:SDR");
        this->label_Signal_source->setText(tr("当前输入信息源:SDR "));
    });

    pSoundbar = new Soundbar(this);


    pSoundbar->moveSoundbar(min_soundbar_Size.width(), min_soundbar_Size.height());

    // 频谱 frequency   spectrum  plot FS
    iPlotStyle = getSetting("plotstyle", 0, true);
    putSetting("plotstyle", iPlotStyle, true);

    iPlotStyle2 = getSetting("plotstyle", 0, true);
    putSetting("plotstyle", iPlotStyle2, true);

    /******************刷新 不同界面的图像*************************/
    //FAC
    connect(this->checkBox_FAC,&QPushButton::clicked,this,
            &FDRMDialog::onCheckBoxToggledFAC);
    checkBox_FAC->setCheckState(Qt::CheckState::Checked);

    //SDC
    connect(this->checkBox_SDC,&QPushButton::clicked,this,
            &FDRMDialog::onCheckBoxToggledSDC);
    //MSC
    connect(this->checkBox_MSC, &QCheckBox::clicked, this,
            &FDRMDialog::onCheckBoxToggledMSC);
#if 1
    /** 这一段代码是有用的， 是控制 main主要接收机监测界面 的频谱 以及QAM，x轴以及y轴 格式的*/
    // plot_FS 创建 x轴 坐标轴
    QwtScaleWidget *BottomAxisWidget = plot_FS->axisWidget(QwtPlot::xBottom);
    BottomAxisWidget->setMargin(0); // 设置边距为0
    /* 设置自定义的刻度绘制对象 */
    BottomAxisWidget->setScaleDraw(new CustomScaleDraw());

    // plot_FS  创建 y轴 坐标轴
    QwtScaleWidget *LeftAxisWidget = plot_FS->axisWidget(QwtPlot::yLeft);
    LeftAxisWidget->setMargin(0); // 设置边距为0
    /* 设置自定义的刻度绘制对象 */
    LeftAxisWidget->setScaleDraw(new CustomScaleDraw());
    /* 设置 x,y 轴的颜色 */
    SetAxisColor(plot_FS, Qt::white);

    ///////
    // plot_xingzuo(星座图) 创建 x坐标轴
    QwtScaleWidget *BottomAxisWidget1 = plot_xingzuo->axisWidget(QwtPlot::xBottom);
    BottomAxisWidget1->setMargin(0); // 设置边距为0
    /* 设置自定义的刻度绘制对象 */
    BottomAxisWidget1->setScaleDraw(new CustomScaleDraw());

    // 创建 y坐标轴
    QwtScaleWidget *LeftAxisWidget1 = plot_xingzuo->axisWidget(QwtPlot::yLeft);
    LeftAxisWidget1->setMargin(0); // 设置边距为0
    /* 设置自定义的刻度绘制对象 */
    LeftAxisWidget1->setScaleDraw(new CustomScaleDraw());
    /* 设置 x,y 轴的颜色 */
    SetAxisColor(plot_xingzuo, Qt::white);

#endif
    /* 频谱 plot */
    pMainPlot = new CDRMPlot(nullptr, plot_FS);

    pMainPlot->SetRecObj(&rx);
    pMainPlot->SetPlotStyle(1);  // iPlotStyle

    // DRM 星座
    pConstellationPlot = new CDRMPlot(nullptr, plot_xingzuo);

    pConstellationPlot->SetRecObj(&rx);
    pConstellationPlot->SetPlotStyle(1); //iPlotStyle2

    // 默认显示
    pMainPlot->SetupChart(CDRMPlot::ECharType::INPUTSPECTRUM_NO_AV);
    pMainPlot->OnTimerChart();
    connect(&MainPlotTimer,&QTimer::timeout, this, &FDRMDialog::FS_func); ////  绘画 频谱
    this->MainPlotTimer.start(200); // 200ms


    pConstellationPlot->SetupChart(CDRMPlot::ECharType::FAC_CONSTELLATION);
    pConstellationPlot->OnTimerChart();
    //设置x,y轴 刻度颜色
    pMainPlot->SetScaleColor(Qt::white);

#if 1
    // audioInput;
    // audioDevice;
    // 设置音频格式
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // 获取默认音频 输入设备
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    std::string str = QAudioDeviceInfo::
            defaultInputDevice().deviceName().toStdString();

    //std::cout<<"Sound in dev = "<< str <<std::endl;
    str = QAudioDeviceInfo::defaultOutputDevice().deviceName().toStdString();
    // std::cout<<"Sound out dev = "<< str <<std::endl;
    if (!info.isFormatSupported(format))
    {
        qWarning() << "Default format not supported, trying to use the nearest.";
        format = info.nearestFormat(format);
    }

    // 创建音频输入
    audioInput = new QAudioInput(format, this);
    // 开始捕获音频数据
    audioDevice = audioInput->start();

    connect(audioDevice, &QIODevice::readyRead,
            this, &FDRMDialog::processAudio);
#endif


    CParameter& Parameters = *rx.GetParameters();

    pLogging = new CLogging(Parameters);
    pLogging->LoadSettings(Settings);

    /* Creation of file and sound card menu */
    pFileMenu = new CFileMenu(rx, this, menu_View);
    pSoundCardMenu = new CSoundCardSelMenu(rx, pFileMenu, this);
    menu_Settings->addMenu(pSoundCardMenu);
    connect(&rx, SIGNAL(soundFileChanged(QString)), this, SLOT(OnSoundFileChanged(QString)));
    connect(&rx, SIGNAL(finished()), this, SLOT(OnWorkingThreadFinished()));

    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(rx, Settings, pFileMenu, pSoundCardMenu);

    /* FM window */
    pFMDlg = new FMDialog(rx, Settings, pFileMenu, pSoundCardMenu);

    /* Parent list for Stations and Live Schedule window */
    QMap <QWidget*,QString> parents;
    parents[this] = "drm";
    parents[pAnalogDemDlg] = "analog";

    /* Stations window */
#ifdef HAVE_LIBHAMLIB
    pStationsDlg = new StationsDlg(rx, Settings, rig, parents);
#else
    pStationsDlg = new StationsDlg(rx, Settings, parents);
#endif

    /* Live Schedule window */
    pLiveScheduleDlg = new LiveScheduleDlg(rx, Settings, parents);

    /* MOT broadcast website viewer window */
#ifdef QT_WEBENGINE_LIB
    pBWSDlg = new BWSViewer(rx, Settings, this);
#endif

    /* Journaline viewer window */
    pJLDlg = new JLViewer(rx, Settings, this);

    /* MOT slide show window */
    pSlideShowDlg = new SlideShowViewer(rx, Settings, this);

    /* Programme Guide Window */
    pEPGDlg = new EPGDlg(rx, Settings, this);

    /* Evaluation window */
    pSysEvalDlg = new systemevalDlg(rx, Settings, this);

    /* general settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this);

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this);

    connect(action_Evaluation_Dialog, SIGNAL(triggered()), pSysEvalDlg, SLOT(show()));
    connect(action_Multimedia_Dialog, SIGNAL(triggered()), this, SLOT(OnViewMultimediaDlg()));
    connect(action_Stations_Dialog, SIGNAL(triggered()), pStationsDlg, SLOT(show()));
    connect(action_Live_Schedule_Dialog, SIGNAL(triggered()), pLiveScheduleDlg, SLOT(show()));
    connect(action_Programme_Guide_Dialog, SIGNAL(triggered()), pEPGDlg, SLOT(show()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

    action_Multimedia_Dialog->setEnabled(false);

    connect(actionMultimediaSettings, SIGNAL(triggered()), pMultSettingsDlg, SLOT(show()));
    connect(actionGeneralSettings, SIGNAL(triggered()), pGeneralSettingsDlg, SLOT(show()));

    connect(actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAM()));
    connect(actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnNewAcquisition()));

    connect(actionDisplayColor, SIGNAL(triggered()), this, SLOT(OnMenuSetDisplayColor()));

    /* Plot style settings */
    QSignalMapper* plotStyleMapper = new QSignalMapper(this);
    QActionGroup* plotStyleGroup = new QActionGroup(this);
    plotStyleGroup->addAction(actionBlueWhite);
    plotStyleGroup->addAction(actionGreenBlack);
    plotStyleGroup->addAction(actionBlackGrey);
    plotStyleMapper->setMapping(actionBlueWhite, 0);
    plotStyleMapper->setMapping(actionGreenBlack, 1);
    plotStyleMapper->setMapping(actionBlackGrey, 2);
    connect(actionBlueWhite, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(actionGreenBlack, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(actionBlackGrey, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(plotStyleMapper, SIGNAL(mapped(int)), this, SIGNAL(plotStyleChanged(int)));
    switch(getSetting("plotstyle", int(0), true))
    {
    case 0:
        actionBlueWhite->setChecked(true);
        break;
    case 1:
        actionGreenBlack->setChecked(true);
        break;
    case 2:
        actionBlackGrey->setChecked(true);
        break;
    }

    connect(actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(OnWhatsThis()));

    connect(this, SIGNAL(plotStyleChanged(int)), pSysEvalDlg, SLOT(UpdatePlotStyle(int)));
    connect(this, SIGNAL(plotStyleChanged(int)), pAnalogDemDlg, SLOT(UpdatePlotStyle(int)));

    pButtonGroup = new QButtonGroup(this);
    pButtonGroup->setExclusive(true);
    pButtonGroup->addButton(PushButtonService1, 0);
    pButtonGroup->addButton(PushButtonService2, 1);
    pButtonGroup->addButton(PushButtonService3, 2);
    pButtonGroup->addButton(PushButtonService4, 3);
    connect(pButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnSelectAudioService(int)));
    connect(pButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnSelectDataService(int)));

    connect(pFMDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));
    connect(pAnalogDemDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));

    /* Init progress bar for input signal level */
#if QWT_VERSION < 0x060100
    ProgrInputLevel->setRange(-50.0, 0.0);
    ProgrInputLevel->setOrientation(Qt::Vertical, QwtThermo::LeftScale);
#else
    //    ProgrInputLevel->setScale(-50.0, 0.0);
    //    ProgrInputLevel->setOrientation(Qt::Vertical);
    //    ProgrInputLevel->setScalePosition(QwtThermo::TrailingScale);
#endif
    //ProgrInputLevel->setAlarmLevel(-12.5);
    QColor alarmColor(QColor(255, 0, 0));
    QColor fillColor(QColor(0, 190, 0));
#if QWT_VERSION < 0x060000
    ProgrInputLevel->setAlarmColor(alarmColor);
    ProgrInputLevel->setFillColor(fillColor);
#else
    QPalette newPalette = FrameMainDisplay->palette();
    newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
    newPalette.setColor(QPalette::ButtonText, fillColor);
    newPalette.setColor(QPalette::Highlight, alarmColor);
    //ProgrInputLevel->setPalette(newPalette);
#endif

#ifdef HAVE_LIBHAMLIB
    connect(pStationsDlg, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pStationsDlg, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
    connect(&rig, SIGNAL(sigstr(double)), pStationsDlg, SLOT(OnSigStr(double)));
    connect(pLogging, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pLogging, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
#endif
    connect(pSysEvalDlg, SIGNAL(startLogging()), pLogging, SLOT(start()));
    connect(pSysEvalDlg, SIGNAL(stopLogging()), pLogging, SLOT(stop()));

    /* Update times for color LEDs */
    //    //CLED_FAC->SetUpdateTime(1500);
    //    CLED_SDC->SetUpdateTime(1500);
    //    CLED_MSC->SetUpdateTime(600);

    connect(pAnalogDemDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pAnalogDemDlg, SIGNAL(NewAMAcquisition()), this, SLOT(OnNewAcquisition()));
    connect(pAnalogDemDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(Closed()), this, SLOT(close()));

    connect(pFMDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pFMDlg, SIGNAL(Closed()), this, SLOT(close()));
    connect(pFMDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    connect(pFMDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));

    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    connect(&rx, SIGNAL(drmModeStarted()), this, SLOT(ChangeGUIModeToDRM()));
    connect(&rx, SIGNAL(amModeStarted()), this, SLOT(ChangeGUIModeToAM()));
    connect(&rx, SIGNAL(fmModeStarted()), this, SLOT(ChangeGUIModeToFM()));

    serviceLabels[0] = TextMiniService1;
    serviceLabels[1] = TextMiniService2;
    serviceLabels[2] = TextMiniService3;
    serviceLabels[3] = TextMiniService4;

    ClearDisplay();
    UpdateWindowTitle(""); // load filename from settings if set

    /* System tray setup */
    pSysTray = CSysTray::Create(
                this,
                SLOT(OnSysTrayActivated(QSystemTrayIcon::ActivationReason)),
                SLOT(OnSysTrayTimer()),
                ":/icons/MainIcon.svg");
    CSysTray::AddAction(pSysTray, tr("&New Acquisition"), this, SLOT(OnNewAcquisition()));
    CSysTray::AddSeparator(pSysTray);
    CSysTray::AddAction(pSysTray, tr("&Exit"), this, SLOT(close()));

    /* clear signal strenght */
    setBars(0);


    /* Activate real-time timers */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::setBars(int bars)
{
    (void)bars;
    //	onebar->setAutoFillBackground(bars>0);
    //	twobars->setAutoFillBackground(bars>1);
    //	threebars->setAutoFillBackground(bars>2);
    //	fourbars->setAutoFillBackground(bars>3);
    //	fivebars->setAutoFillBackground(bars>4);
}

FDRMDialog::~FDRMDialog()
{
    /* Destroying logger */
    delete pLogging;
    /* Destroying top level windows, children are automaticaly destroyed */
    delete pAnalogDemDlg;
    delete pFMDlg;
}
//  绘画 频谱
void FDRMDialog::FS_func()
{


    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    bool change = false;
    bool bAudioDecoder = ! rx.GetParameters()->audiodecoder.empty();
    bool bLastAudioDecoder;
    rx.GetAudioSpec(vecrData, vecrScale);
    if (change)
    {
        bLastAudioDecoder = bAudioDecoder;
        // SetupAudioSpec(bAudioDecoder);
    }

    /* Set data */
    if (bAudioDecoder)
    {
        // cout<<"绘画  频谱"<<endl;
        pMainPlot->SetData_public(vecrData, vecrScale);
    }

    plot_FS->replot();

}

void FDRMDialog::updateLabelColor()
{
    // pMainPlot->OnTimerChart();
    pMainPlot->OnTimerChart();
    pConstellationPlot->OnTimerChart(); //星座图的绘画

    // cout<<"主界面的所有 参数指标 "<<endl;
    CParameter& Parameters = *rx.GetParameters();

    //TODO : 根据  源 数据解码后的 CRC状态 来判断状态是否ok
    bool LED_FAC = Parameters.ReceiveStatus.FAC.GetStatus();
    bool LED_SDC = Parameters.ReceiveStatus.SDC.GetStatus();

    int iShortID = Parameters.GetCurSelAudioService();
    bool LED_MSC = Parameters.AudioComponentStatus[iShortID].GetStatus();

    //Q调色板  CMultColorLED *LEDFAC;
    QPalette FACpalette = LEDFAC->palette();
    if (LED_FAC)
    {   // 设置背景颜色为绿色
        //label_LED_FAC->setStyleSheet("background-color: green;");
        LEDFAC->setStyleSheet("background-color: green;");

    } else
    {
        // 设置背景颜色为黑色，字体颜色为白色
        //label_LED_FAC->setStyleSheet("background-color: black; color: white;");
        LEDFAC->setStyleSheet("background-color: black; color: white;");
    }
    LEDFAC->setPalette(FACpalette);
    LEDFAC->update();

    // SDC
    QPalette SDCpalette = LEDSDC->palette();
    if (LED_SDC)
    {
        LEDSDC->setStyleSheet("background-color: green;");
    }
    else
    {
        LEDSDC->setStyleSheet("background-color: black; color: white;");
    }
    LEDSDC->setPalette(SDCpalette);
    LEDSDC->update();

    //MSC
    QPalette paletteMSC = LEDMSC->palette();
    if (LED_MSC)
    {
        LEDMSC->setStyleSheet("background-color: green;");  // 设置背景颜色为绿色

    }
    else
    {
        LEDMSC->setStyleSheet("background-color: black; color: white;");
    }
    LEDMSC->setPalette(paletteMSC);
    LEDMSC->update();

    // 帧同步
    QPalette paletteFrameSync = LEDFrameSync->palette();
    int Frame_Sync = (Parameters.iFrameIDReceiv) % 3 ;
    if (LED_MSC)  //  Frame_syn - Frame_Sync <= 1
    {
        LEDFrameSync->setStyleSheet("background-color: green;");  // 设置背景颜色为绿色
    }
    else
    {
        LEDFrameSync->setStyleSheet("background-color: black; color: white;");
    }

    Frame_syn = Frame_Sync;
    LEDFrameSync->setPalette(paletteMSC);
    LEDFrameSync->update();

    // time syn
    QPalette paletteLEDTimeSync = LEDTimeSync->palette();
    bool time_syn = Parameters.ReceiveStatus.TSync.GetStatus();
    if (time_syn)
    {
        LEDTimeSync->setStyleSheet("background-color: green;");  // 设置背景颜色为绿色
    }
    else
    {
        LEDTimeSync->setStyleSheet("background-color: black; color: white;");
    }
    LEDTimeSync->setPalette(paletteMSC);
    LEDTimeSync->update();

    // my_ LEDIOInterface
    ETypeRxStatus soundCardStatusI /*in  put Status*/ = Parameters.ReceiveStatus.InterfaceI.GetStatus(); /* Input */
    ETypeRxStatus soundCardStatusO /*Out put Status*/ = Parameters.ReceiveStatus.InterfaceO.GetStatus(); /* Output */
    // LED_IO Status   DATA_ERROR == 3  ,  3
    //    std::cout<<" soundCardStatusO = "<<soundCardStatusO <<std::endl;
    //    std::cout<<" soundCardStatusI = "<<soundCardStatusI <<std::endl;


    int LED_IO = soundCardStatusO == NOT_PRESENT/*现在 not use */
            || (soundCardStatusI != NOT_PRESENT && soundCardStatusI != RX_OK) ?
                soundCardStatusI : soundCardStatusO;

    QPalette paletteLEDIOInterface = LEDIOInterface->palette();

    if (LED_IO == RX_OK)
    {
        LEDIOInterface->setStyleSheet("background-color: green;");  // 设置背景颜色为绿色
    }
    else if(LED_IO == DATA_ERROR)
    {
        LEDIOInterface->setStyleSheet("background-color: yellow;");
    }
    else
    {
        LEDIOInterface->setStyleSheet("background-color: black; color: white;");
    }

    LEDIOInterface->setPalette(paletteMSC);
    LEDIOInterface->update();


    /* Show SNR if receiver is in tracking mode 显示信噪比，如果接收机是在跟踪模式  */
    if (rx.GetAcquisitionState() == AS_WITH_SIGNAL)
    {
        /* Get a consistant snapshot */

        /* We only get SNR from a local DREAM Front-End
            我们只能从本地DREAM前端获得信噪比 */
        _REAL rSNR = Parameters.GetSNR();
        if (rSNR >= 0.0)
        {
            /* SNR */
            ValueSNR->setText("<b>" +
                              QString().setNum(rSNR, 'f', 1) + " dB</b>");
        }
        else
        {
            //如果是网络的情况  ---
            ValueSNR->setText("<b>---</b>");
        }
        /* We get MER from a local DREAM Front-End or an RSCI input but not an MDI input
            我们从本地DREAM前端 或RSCI 输入获取 MER，而不是MDI输入 */
        _REAL rMER = Parameters.rMER;
        if (rMER >= 0.0 )
        {
            ValueMERWMER->setText(QString().
                                  setNum(Parameters.rWMERMSC, 'f', 1) + " dB / "
                                  + QString().setNum(rMER, 'f', 1) + " dB");
        }
        else
        {
            //如果是网络的情况 ，请可能就是 ---
            ValueMERWMER->setText("---");
        }

        /* Doppler estimation (assuming Gaussian doppler spectrum) */
        if (Parameters.rSigmaEstimate >= 0.0)
        {
            /* Plot delay and Doppler values */
            ValueWiener->setText(
                        QString().setNum(Parameters.rSigmaEstimate, 'f', 2) + " Hz / "
                        + QString().setNum(Parameters.rMinDelay, 'f', 2) + " ms");
        }
        else
        {
            /* Plot only delay, Doppler not available */
            ValueWiener->setText("--- / "
                                 + QString().setNum(Parameters.rMinDelay, 'f', 2) + " ms");
        }

        /* Sample frequency offset estimation */
        const _REAL rCurSamROffs = Parameters.rResampleOffset;

        /* Display value in [Hz] and [ppm] (parts per million) */
        ValueSampFreqOffset->setText(
                    QString().setNum(rCurSamROffs, 'f', 2) + " Hz (" +
                    QString().setNum((int) (rCurSamROffs / Parameters.GetSigSampleRate() * 1e6))
                    + " ppm)");

    }
    else
    {
        ValueSNR->setText("<b>---</b>");
        ValueMERWMER->setText("---");
        ValueWiener->setText("--- / ---");
        ValueSampFreqOffset->setText("---");
    }

    /* DC frequency */
    ValueFreqOffset->setText(QString().
                             setNum(rx.ConvertFrequency(Parameters.GetDCFrequency()),
                                    'f', 2) + " Hz");
    /* If MDI in is enabled, do not show any synchronization parameter
             * 如果启用了MDI in，则不显示任何同步参数  */
    if (rx.inputIsRSCI())
    {
        ValueSNR->setText("<b>---</b>");
        if (Parameters.vecrRdelThresholds.GetSize() > 0)
            ValueWiener->setText(QString().setNum(Parameters.rRdop, 'f', 2) + " Hz / "
                                 + QString().setNum(Parameters.vecrRdelIntervals[0], 'f', 2) + " ms ("
                    + QString().setNum(Parameters.vecrRdelThresholds[0]) + "%)");
        else
            ValueWiener->setText(QString().setNum(Parameters.rRdop, 'f', 2) + " Hz / ---");

        ValueSampFreqOffset->setText("---");
        ValueFreqOffset->setText("---");
    }

}

void FDRMDialog::OnOpenFile_soure()
{
    // 弹出文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                    tr("All Files (*);;Text Files (*.txt)"));
    if (!fileName.isEmpty())
    {
        // 用户选择了文件，处理文件
        QMessageBox::information(this, tr("File Selected"), fileName);
    }
    else
    {
        // 用户取消了文件选择
        QMessageBox::information(this, tr("No File Selected"), tr("No file was selected."));
    }
    this->label_Signal_source->setText(tr("当前输入信息源:文件"));
}

void FDRMDialog::processAudio()
{

    QByteArray audioData = audioDevice->readAll();
    const qint16 *data = reinterpret_cast<const qint16*>(audioData.constData());
    int numSamples = audioData.size() / sizeof(qint16);
    if(numSamples == 0 )
    {
        numSamples = 1;
    }
    // 计算音量
    qint64 sum = 0;
    for (int i = 0; i < numSamples; ++i) {
        sum += qAbs(data[i]);
    }
    qint64 average = sum / numSamples;
    if(average == 0)
    {
        average = 1;
    }
#if 1
    int volume = 40* (static_cast<int>(average * 100 / 32767));  // 转换为百分比
    // std::cout<<"volume = "<<volume<<std::endl;
    pSoundbar->setVolume(volume);
#endif


}

#include "../csubdrmreceiver.h"
void FDRMDialog::receiveParameters()
{
    // 当接收到参数时，发射信号
    // DEBUG_COUT("刷新菜单按钮 emit parametersReceived(rx) "); // 打印

    // emit parametersReceived(this->pReceiverManager);
}

void FDRMDialog::OnSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger
        #if QT_VERSION < 0x050000
            || reason == QSystemTrayIcon::DoubleClick
        #endif
            )
    {
        const Qt::WindowStates ws = pCurrentWindow->windowState();
        if (ws & Qt::WindowMinimized)
            pCurrentWindow->setWindowState((ws & ~Qt::WindowMinimized) | Qt::WindowActive);
        else
            pCurrentWindow->toggleVisibility();
    }
}

void FDRMDialog::OnSysTrayTimer()
{
    QString Title, Message;
    if (rx.GetAcquisitionState() == AS_WITH_SIGNAL)
    {
        CParameter& Parameters = *rx.GetParameters();
        Parameters.Lock();
        const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
        CService audioService = Parameters.Service[iCurSelAudioServ];
        const bool bServiceIsValid = audioService.IsActive()
                && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
                && (audioService.eAudDataFlag == CService::SF_AUDIO);
        if (bServiceIsValid)
        {
            /* Service label (UTF-8 encoded string -> convert) */
            Title = QString::fromUtf8(audioService.strLabel.c_str());
            // Text message of current selected audio service (UTF-8 decoding)
            Message = QString::fromUtf8(audioService.AudioParam.strTextMessage.c_str());
        }
        if(Parameters.rWMERMSC>WMERSteps[4])
            setBars(5);
        else if(Parameters.rWMERMSC>WMERSteps[3])
            setBars(4);
        else if(Parameters.rWMERMSC>WMERSteps[2])
            setBars(3);
        else if(Parameters.rWMERMSC>WMERSteps[1])
            setBars(2);
        else if(Parameters.rWMERMSC>WMERSteps[0])
            setBars(1);
        else
            setBars(0);
        Parameters.Unlock();
    }
    else {
        Message = tr("Scanning...");
        setBars(0);
    }
    CSysTray::SetToolTip(pSysTray, Title, Message);
}

void FDRMDialog::OnWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void FDRMDialog::OnSwitchToFM()
{
    OnSwitchMode(RM_FM);
}

void FDRMDialog::OnSwitchToAM()
{
    OnSwitchMode(RM_AM);
}

void FDRMDialog::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
    switch(state)
    {
    case NOT_PRESENT:
        LED->Reset(); /* GREY */
        break;

    case CRC_ERROR:
        LED->SetLight(CMultColorLED::RL_RED);
        break;

    case DATA_ERROR:
        LED->SetLight(CMultColorLED::RL_YELLOW);
        break;

    case RX_OK:
        LED->SetLight(CMultColorLED::RL_GREEN);
        break;
    }
}

#include "DRMPlot.h"

void FDRMDialog::UpdateDRM_GUI()
{
    //一直会 自动刷新的
    bool bMultimediaServiceAvailable;
    CParameter& Parameters = *( rx.GetParameters() );

    //if (isVisible() == false)
    //    ChangeGUIModeToDRM();

    Parameters.Lock();

    //    /* Input level meter */
    //    ProgrInputLevel->setValue(Parameters.GetIFSignalLevel());
    //    SetStatus(CLED_FAC, Parameters.ReceiveStatus.FAC.GetStatus());
    //    SetStatus(CLED_SDC, Parameters.ReceiveStatus.SDC.GetStatus());
    //	int iShortID = Parameters.GetCurSelAudioService();
    //	if(Parameters.Service[iShortID].eAudDataFlag == CService::SF_AUDIO)
    //	    SetStatus(CLED_MSC, Parameters.AudioComponentStatus[iShortID].GetStatus());
    //	else
    //	    SetStatus(CLED_MSC, Parameters.DataComponentStatus[iShortID].GetStatus());

    Parameters.Unlock();

    /* Clear the multimedia service bit */
    iMultimediaServiceBit = 0;

    /* Check if receiver does receive a signal */
    if(rx.GetAcquisitionState() == AS_WITH_SIGNAL)
        UpdateDisplay();
    else
    {
        ClearDisplay();
        /* No signal then set the last
           multimedia service selected to none */
        iLastMultimediaServiceSelected = -1;
    }
    /* If multimedia service availability has changed
       then update the menu如果多媒体业务可用性发生变化
        然后更新菜单 */
    bMultimediaServiceAvailable = iMultimediaServiceBit != 0;
    if (bMultimediaServiceAvailable != action_Multimedia_Dialog->isEnabled())
        action_Multimedia_Dialog->setEnabled(bMultimediaServiceAvailable);


    //my_ 主接收机信息 FACDRMModeBWV
    /* FAC info static ------------------------------------------------------ */
    QString strFACInfo;
    strFACInfo = pSysEvalDlg->GetRobModeStr() ;
    //+ " / " +  pSysEvalDlg->GetSpecOccStr();
    FACDRMModeBWV->setText(strFACInfo);
    strFACInfo = pSysEvalDlg->GetSpecOccStr();
    FACDRMWV_2->setText(strFACInfo);
    /* Interleaver Depth #################### */
    switch (Parameters.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        strFACInfo = tr("2 s (长交织)");  //Long Interleaving
        break;

    case CParameter::SI_SHORT:
        strFACInfo = tr("400 ms (短交织)");   //Short Interleaving 短
        break;

    default:
        strFACInfo = "?";
    }

    FACInterleaverDepthV->setText(strFACInfo); /* my_ Value */

    //
    /* SDC, MSC mode #################### */
    /* SDC */
    switch (Parameters.eSDCCodingScheme)
    {
    case CS_1_SM:
        strFACInfo = "4-QAM ";
        //         strFACInfo = "4-QAM / ";
        break;

    case CS_2_SM:
        strFACInfo = "16-QAM ";
        //         strFACInfo = "16-QAM / ";
        break;

    default:
        strFACInfo = "? / ";
    }
    FACSDCMSCModeV_2->setText(strFACInfo);

    /* MSC */
    switch (Parameters.eMSCCodingScheme)
    {
    case CS_2_SM:
        //        strFACInfo += "SM 16-QAM";
        strFACInfo  = "SM 16-QAM";
        break;

    case CS_3_SM:
        //        strFACInfo += "SM 64-QAM";
        strFACInfo  = "SM 64-QAM";
        break;

    case CS_3_HMSYM:
        //        strFACInfo += "HMsym 64-QAM";
        strFACInfo  = "HMsym 64-QAM";
        break;

    case CS_3_HMMIX:
        //        strFACInfo += "HMmix 64-QAM";
        strFACInfo  = "HMmix 64-QAM";
        break;

    default:
        strFACInfo += "?";
    }

    //my  MSC  mode
    FACSDCMSCModeV_3->setText(strFACInfo);

    /* Code rates #################### */
    //比如 显示: 0 / 1
    strFACInfo = QString().setNum(Parameters.MSCPrLe.iPartB);
    strFACInfo += " / ";
    strFACInfo += QString().setNum(Parameters.MSCPrLe.iPartA);

    FACCodeRateV->setText(strFACInfo); /* Value */


    /* Number of services #################### */
    strFACInfo = tr("Audio: "); //Audio
    strFACInfo += QString().setNum(Parameters.iNumAudioService);
    strFACInfo += tr(" / Data: ");  //Data
    strFACInfo += QString().setNum(Parameters.iNumDataService);

    FACNumServicesV->setText(strFACInfo); /* Value */

    ///my刷新界面-------------------------------------------------------
    {
        CParameter& Parameters = *rx.GetParameters();
        //  std::cout << "更新图表的数据和样式" << std::endl;
        /* CHART ******************************************************************/
        CVector<_REAL>	vecrData;
        CVector<_REAL>	vecrData2;
        CVector<_COMPLEX>	veccData1, veccData2, veccData3;
        CVector<_REAL>	vecrScale;
        _REAL		rLowerBound, rHigherBound;
        _REAL		rStartGuard, rEndGuard;
        _REAL		rPDSBegin, rPDSEnd;
        _REAL		rFreqAcquVal;
        _REAL		rCenterFreq, rBandwidth;

        Parameters.Lock();
        _REAL rDCFrequency = Parameters.GetDCFrequency();
        ECodScheme eSDCCodingScheme = Parameters.eSDCCodingScheme;
        ECodScheme eMSCCodingScheme = Parameters.eMSCCodingScheme;
        bool bAudioDecoder = !Parameters.audiodecoder.empty();
        int iAudSampleRate = Parameters.GetAudSampleRate();  //TODO:本来是成员变量 ，现在是临时变量
        int iSigSampleRate = Parameters.GetSigSampleRate();  //TODO:
        int iChanMode = int(rx.getRx()->GetReceiveData()->GetInChanSel());

        Parameters.Unlock();

        /* Needed to detect sample rate change */
        const int iXoredSampleRate = iAudSampleRate ^ iSigSampleRate;

        CPlotManager& PlotManager = *(rx.getRx()->GetPlotManager());

        /* First check if plot must be set up */

        CDRMPlot::ECharType CurCharType = CDRMPlot::ECharType::FAC_CONSTELLATION ;  //TODO:   CurCharType 还没有根据界面 赋值
        switch (CurCharType)
        {
        case CDRMPlot::ECharType::FAC_CONSTELLATION:
            /* Get data vector */
            //rx.GetFACMLC()->GetVectorSpace(v);
            rx.getRx()->GetFACMLC()->GetVectorSpace(veccData1);

            //if (change) SetupFACConst();
            /* Set data */
            // CDRMPlot::SetData(veccData1);
            break;

        case CDRMPlot::ECharType::SDC_CONSTELLATION:
            /* Get data vector */
            //pDRMRec->GetSDCMLCVectorSpace(veccData1);

            //            if (change || eLastSDCCodingScheme != eSDCCodingScheme)
            //            {
            //                eLastSDCCodingScheme = eSDCCodingScheme;
            //                SetupSDCConst(eSDCCodingScheme);
            //            }
            /* Set data */
            //SetData(veccData1);
            break;

        case CDRMPlot::ECharType::MSC_CONSTELLATION:
            /* Get data vector */
            //pDRMRec->GetMSCMLCVectorSpace(veccData1);

            //            if (change || eLastMSCCodingScheme != eMSCCodingScheme)
            //            {
            //                eLastMSCCodingScheme = eMSCCodingScheme;
            //                SetupMSCConst(eMSCCodingScheme);
            //            }
            /* Set data */
            //SetData(veccData1);
            break;

        case CDRMPlot::ECharType::ALL_CONSTELLATION:
            /* Get data vectors */
//            pDRMRec->GetMSCMLCVectorSpace(veccData1);
//            pDRMRec->GetSDCMLCVectorSpace(veccData2);
//            pDRMRec->GetFACMLCVectorSpace(veccData3);

//            //            if (change) SetupAllConst();
//            /* Set data */
//            SetData(veccData1, veccData2, veccData3);
            break;
        default:
            // 处理未知或未指定的信号类型
            break;
        }


    }
    plot_FS->replot();
    plot_xingzuo->replot();

}

void FDRMDialog::startLogging()
{
    pSysEvalDlg->CheckBoxWriteLog->setChecked(true);
}

void FDRMDialog::stopLogging()
{
    pSysEvalDlg->CheckBoxWriteLog->setChecked(false);
}

void FDRMDialog::OnScheduleTimer()
{
    CScheduler::SEvent e;
    e = pScheduler->front();
    if (e.frequency != -1)
    {
        rx.SetFrequency(e.frequency);
        if(!pLogging->enabled())
        {
            startLogging();
        }
    }
    else
    {
        stopLogging();
    }
    if(pScheduler->empty())
    {
        stopLogging();
    }
    else
    {
        e = pScheduler->pop();
        time_t now = time(nullptr);
        pScheduleTimer->start(1000*(e.time-now));
    }
}

//main
void FDRMDialog::OnTimer()
{
    UpdateDRM_GUI(); // TODO move this to signal driven

    // do this here so GUI has initialised before we might pop up a message box

    if(pScheduler!=nullptr)
    {
        return;
    }
    // cout<<"pScheduler==nullptr"<<endl;  不会return


    string schedfile = Settings.Get("command", "schedule", string());
    if(schedfile != "")
    {
        bool testMode = Settings.Get("command", "test", false);
        pScheduler = new CScheduler(testMode);
        if(pScheduler->LoadSchedule(schedfile)) {
            pScheduleTimer = new QTimer(this);
            connect(pScheduleTimer, SIGNAL(timeout()), this, SLOT(OnScheduleTimer()));
            /* Setup the first timeout */
            CScheduler::SEvent e;
            if(!pScheduler->empty()) {
                e = pScheduler->front();
                time_t now = time(nullptr);
                time_t next = e.time - now;
                if(next > 0)
                {
                    pScheduleTimer->start(1000*next);
                }
                else // We are late starting
                {
                    startLogging();
                }
            }
        }
        else {
            QMessageBox::information(this, "Dream", tr("Schedule file requested but not found"));
        }
    }
    else
    {
        if(pLogging->enabled())
            startLogging();
    }
}

void FDRMDialog::showTextMessage(const QString& textMessage)
{
    /* Activate text window */
    //TextTextMessage->setEnabled(true);

    QString formattedMessage = "";
    // yizhi shi 0  , zhiyou dang shujv laide shihou  caihuixianshi wenzigeshu
    for (int i = 0; i < (int)textMessage.length(); i++)
    {
        switch (textMessage.at(i).unicode())
        {
        case 0x0A:
            /* Code 0x0A may be inserted to indicate a preferred
               line break */
        case 0x1F:
            /* Code 0x1F (hex) may be inserted to indicate a
               preferred word break. This code may be used to
                   display long words comprehensibly */
            formattedMessage += "<br>";
            break;

        case 0x0B:
            /* End of a headline */
            formattedMessage = "<b><u>"
                    + formattedMessage
                    + "</u></b></center><br><center>";
            break;

        case '<':
            formattedMessage += "&lt;";
            break;

        case '>':
            formattedMessage += "&gt;";
            break;

        default:
            formattedMessage += textMessage[int(i)];
        }
    }
    Linkify(formattedMessage);
    formattedMessage = "<center>" + formattedMessage + "</center>";

    // Source code
    //TextTextMessage->setText(formattedMessage);
    //my_
    if((int)textMessage.length())
    {
        TextTextMessage_2->setText(formattedMessage);
    }

}

void FDRMDialog::showServiceInfo(const CService& service)
{
    /* Service label (UTF-8 encoded string -> convert) */
    QString ServiceLabel(QString::fromUtf8(service.strLabel.c_str()));
    //LabelServiceLabel->setText(ServiceLabel);

    /* Service ID (plot number in hexadecimal format) */
    const long iServiceID = (long) service.iServiceID;
    (void)iServiceID;
    //    if (iServiceID != 0)
    //    {
    //        LabelServiceID->setText(QString("ID:%1").arg(iServiceID,4,16).toUpper());
    //    }
    //    else
    //        LabelServiceID->setText("");

    //    /* Codec label */
    //    LabelCodec->setText(GetCodecString(service));

    /* Type (Mono / Stereo) label */
    //LabelStereoMono->setText(GetTypeString(service));

    /* Language and program type labels (only for audio service) */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* SDC Language */
        const string strLangCode = service.strLanguageCode;

        if ((!strLangCode.empty()) && (strLangCode != "---"))
        {
            //            LabelLanguage->
            //            setText(QString(GetISOLanguageName(strLangCode).c_str()));
        }
        else
        {
            /* FAC Language */
            const int iLanguageID = service.iLanguage;

            if ((iLanguageID > 0) &&
                    (iLanguageID < LEN_TABLE_LANGUAGE_CODE))
            {
                //                LabelLanguage->setText(
                //                    strTableLanguageCode[iLanguageID].c_str());
            }
            else
            {
                //LabelLanguage->setText("");
            }
            //
        }

        /* Program type */
        const int iProgrammTypeID = service.iServiceDescr;

        if ((iProgrammTypeID > 0) &&
                (iProgrammTypeID < LEN_TABLE_PROG_TYPE_CODE))
        {
            //            LabelProgrType->setText(
            //                strTableProgTypCod[iProgrammTypeID].c_str());
        }
        else
        {
            //LabelProgrType->setText("");
        }

    }

    /* Country code */
    const string strCntryCode = service.strCountryCode;

    if ((!strCntryCode.empty()) && (strCntryCode != "--"))
    {
        /*LabelCountryCode->
        setText(QString(GetISOCountryName(strCntryCode).c_str()));*/
    }
    else{
        //LabelCountryCode->setText("");
    }

}

QString FDRMDialog::serviceSelector(CParameter& Parameters, int i)
{
    Parameters.Lock();

    CService service = Parameters.Service[i];
    const _REAL rAudioBitRate = Parameters.GetBitRateKbps(i, false);
    const _REAL rDataBitRate = Parameters.GetBitRateKbps(i, true);

    /* detect if AFS mux information is available TODO - align to service */
    bool bAFS = ((i==0) && (
                     (Parameters.AltFreqSign.vecMultiplexes.size() > 0)
                     || (Parameters.AltFreqSign.vecOtherServices.size() > 0)
                     ));

    Parameters.Unlock();

    QString text;

    /* Check, if service is used */
    if (service.IsActive())
    {
        /* Do UTF-8 to string conversion with the label strings */
        QString strLabel = QString().fromUtf8(service.strLabel.c_str());

        /* Label for service selection button (service label, codec
           and Mono / Stereo information) */
        QString strCodec = GetCodecString(service);
        QString strType = GetTypeString(service);
        text = strLabel;
        if (!strCodec.isEmpty() || !strType.isEmpty())
            text += "  | " + strCodec + " | " + strType;

        /* Bit-rate (only show if greater than 0) */
        if (rAudioBitRate > (_REAL) 0.0)
        {
            text += " (" + QString().setNum(rAudioBitRate, 'f', 2) + " kbps)";
        }

        /* Audio service */
        if (service.eAudDataFlag == CService::SF_AUDIO)
        {
            /* Report missing codec !!!! */
            if (!rx.CanDecode(service.AudioParam.eAudioCoding))
                text += tr(" [-no codec available]");

            /* Show, if a multimedia stream is connected to this service */
            if (service.DataParam.iStreamID != STREAM_ID_NOT_USED)
            {
                if (service.DataParam.iUserAppIdent == DAB_AT_EPG)
                    text += tr(" + EPG"); /* EPG service */
                else
                {
                    text += tr(" + MM"); /* other multimedia service */
                    /* Set multimedia service bit */
                    iMultimediaServiceBit |= 1 << i;
                }

                /* Bit-rate of connected data stream */
                text += " (" + QString().setNum(rDataBitRate, 'f', 2) + " kbps)";
            }
        }
        /* Data service */
        else
        {
            if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
            {
                if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
                {
                    switch (service.DataParam.iUserAppIdent)
                    {
                    case DAB_AT_BROADCASTWEBSITE:
                    case DAB_AT_JOURNALINE:
                    case DAB_AT_MOTSLIDESHOW:
                        /* Set multimedia service bit */
                        iMultimediaServiceBit |= 1 << i;
                        break;
                    }
                }
            }
        }

        if(bAFS)
        {
            text += tr(" + AFS");
        }

        switch (i)
        {
        case 0:
            PushButtonService1->setEnabled(true);
            break;

        case 1:
            PushButtonService2->setEnabled(true);
            break;

        case 2:
            PushButtonService3->setEnabled(true);
            break;

        case 3:
            PushButtonService4->setEnabled(true);
            break;
        }
    }
    return text;
}

void FDRMDialog::UpdateDisplay()
{
    CParameter& Parameters = *rx.GetParameters();

    Parameters.Lock();

    /* Receiver does receive a DRM signal ------------------------------- */
    /* First get current selected services */
    int iCurSelAudioServ = Parameters.GetCurSelAudioService();

    CService audioService = Parameters.Service[iCurSelAudioServ];
    Parameters.Unlock();

    bool bServiceIsValid = audioService.IsActive()
            && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
            && (audioService.eAudDataFlag == CService::SF_AUDIO);

    int iFirstAudioService=-1, iFirstDataService=-1;
    for(unsigned i=0; i < MAX_NUM_SERVICES; i++)
    {
        QString label = serviceSelector(Parameters, int(i));
        serviceLabels[i]->setText(label);
        pButtonGroup->button(int(i))->setEnabled(label != "");
        if (!bServiceIsValid && (iFirstAudioService == -1 || iFirstDataService == -1))
        {
            Parameters.Lock();
            audioService = Parameters.Service[i];
            Parameters.Unlock();
            /* If the current audio service is not valid
                find the first audio service available */
            if (iFirstAudioService == -1
                    && audioService.IsActive()
                    && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
                    && (audioService.eAudDataFlag == CService::SF_AUDIO))
            {
                iFirstAudioService = i;
            }
            /* If the current audio service is not valid
                find the first data service available */
            if (iFirstDataService == -1
                    && audioService.IsActive()
                    && (audioService.eAudDataFlag == CService::SF_DATA))
            {
                iFirstDataService = i;
            }
        }
    }

    /* Select a valid service, priority to audio service */
    if (iFirstAudioService != -1)
    {
        iCurSelAudioServ = iFirstAudioService;
        bServiceIsValid = true;
    }
    else
    {
        if (iFirstDataService != -1)
        {
            iCurSelAudioServ = iFirstDataService;
            bServiceIsValid = true;
        }
    }

    if(bServiceIsValid)
    {
        /* Get selected service */
        Parameters.Lock();
        audioService = Parameters.Service[iCurSelAudioServ];
        Parameters.Unlock();

        pButtonGroup->button(iCurSelAudioServ)->setChecked(true);

        /* If we have text messages */
        if (audioService.AudioParam.bTextflag)
        {
            // Text message of current selected audio service (UTF-8 decoding)
            QString TextMessage(QString::fromUtf8(audioService.AudioParam.strTextMessage.c_str()));
            showTextMessage(TextMessage);
        }
        else
        {
            /* Deactivate text window */
            //TextTextMessage->setEnabled(false);

            /* Clear Text */
            //TextTextMessage->setText("");
        }

        /* Check whether service parameters were not transmitted yet */
        if (audioService.IsActive())
        {
            showServiceInfo(audioService);

            Parameters.Lock();
            _REAL rPartABLenRat = Parameters.PartABLenRatio(iCurSelAudioServ);
            _REAL rBitRate = Parameters.GetBitRateKbps(iCurSelAudioServ, false);
            Parameters.Unlock();

            /* Bit-rate */
            QString strBitrate = QString().setNum(rBitRate, 'f', 2) + tr(" kbps");

            /* Equal or unequal error protection */
            if (int(rPartABLenRat) != 0)
            {
                /* Print out the percentage of part A length to total length */
                strBitrate += " UEP (" + QString().setNum(rPartABLenRat * 100, 'f', 1) + " %)";
            }
            else
            {
                /* If part A is zero, equal error protection (EEP) is used */
                strBitrate += " EEP";
            }
            //LabelBitrate->setText(strBitrate);

        }
        else
        {
            //            LabelServiceLabel->setText(tr("No Service"));
            //            LabelBitrate->setText("");
            //            LabelCodec->setText("");
            //            LabelStereoMono->setText("");
            //            LabelProgrType->setText("");
            //            LabelLanguage->setText("");
            //            LabelCountryCode->setText("");
            //            LabelServiceID->setText("");
        }
    }
}

void FDRMDialog::ClearDisplay()
{
    /* No signal is currently received ---------------------------------- */
    /* Disable service buttons and associated labels */
    pButtonGroup->setExclusive(false);
    for(size_t i=0; i<serviceLabels.size(); i++)
    {
        QPushButton* button = (QPushButton*)pButtonGroup->button(i);
        if (button && button->isEnabled()) button->setEnabled(false);
        if (button && button->isChecked()) button->setChecked(false);
        serviceLabels[i]->setText("");
    }
    pButtonGroup->setExclusive(true);

    /* Main text labels */
    //    LabelBitrate->setText("");
    //    LabelCodec->setText("");
    //    LabelStereoMono->setText("");
    //    LabelProgrType->setText("");
    //    LabelLanguage->setText("");
    //    LabelCountryCode->setText("");
    //    LabelServiceID->setText("");

    /* Hide text message label */
    //    TextTextMessage->setEnabled(false);
    //    TextTextMessage->setText("");

    //    LabelServiceLabel->setText(tr("Scanning..."));
}

void FDRMDialog::OnSoundFileChanged(QString s)
{
    UpdateWindowTitle(s);
    ClearDisplay();
}

void FDRMDialog::UpdateWindowTitle(QString filename)
{
    QFileInfo fi(filename);
    if(fi.exists()) {

        setWindowTitle(QString("Dream") + " - " + fi.baseName());
        pAnalogDemDlg->setWindowTitle(tr("Analog Demodulation") + " - " + fi.baseName());
        pFMDlg->setWindowTitle(tr("FM Receiver") + " - " + fi.baseName());
    }
    else {
        //        setWindowTitle("Dream");
        setWindowTitle("DRM数字广播检测");

        // 设置标题栏高度 ，并增大标题文字的字体大小  但是  soundbar 就会乱
        //        this->setStyleSheet("QMainWindow {"
        //                                 "    border-top: 40px solid black; /* 设置标题栏高度 */"
        //                                 "}"
        //                                 "QMainWindow::title {"
        //                                 "    font-size: 20pt; /* 设置标题文字的字体大小 */"
        //                                 "}");

        pAnalogDemDlg->setWindowTitle(tr("Analog Demodulation"));
        pFMDlg->setWindowTitle(tr("FM Receiver"));
    }
}

/* change mode is only called when the mode REALLY has changed
 * so no conditionals are needed in this routine
 */

void FDRMDialog::ChangeGUIModeToDRM()
{
    CSysTray::Start(pSysTray);
    pCurrentWindow = this;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::ChangeGUIModeToAM()
{
    hide();
    Timer.stop();
    CSysTray::Stop(pSysTray, tr("Dream AM"));
    pCurrentWindow = pAnalogDemDlg;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::ChangeGUIModeToFM()
{
    hide();
    Timer.stop();
    CSysTray::Stop(pSysTray, tr("Dream FM"));
    pCurrentWindow = pFMDlg;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::eventUpdate()
{
    /* Put (re)initialization code here for the settings that might have
       be changed by another top level window. Called on mode switch */
    //pFileMenu->UpdateMenu();
    SetDisplayColor(CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true)));
}

void FDRMDialog::eventShow(QShowEvent*)
{
    /* Set timer for real-time controls */
    OnTimer();
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::eventHide(QHideEvent*)
{
    /* Deactivate real-time timers */
    Timer.stop();
}

void FDRMDialog::OnNewAcquisition()
{
    rx.Restart();
}

void FDRMDialog::OnSwitchMode(int newMode)
{
    rx.SetReceiverMode(ERecMode(newMode));
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::OnSelectAudioService(int shortId)
{
    CParameter& Parameters = *rx.GetParameters();

    Parameters.Lock();

    Parameters.SetCurSelAudioService(shortId);

    Parameters.Unlock();
}

void FDRMDialog::OnSelectDataService(int shortId)
{
    CParameter& Parameters = *rx.GetParameters();
    QWidget* pDlg = nullptr;

    Parameters.Lock();

    int iAppIdent = Parameters.Service[shortId].DataParam.iUserAppIdent;

    switch(iAppIdent)
    {
    case DAB_AT_EPG:
        pDlg = pEPGDlg;
        break;
    case DAB_AT_BROADCASTWEBSITE:
#ifdef QT_WEBENGINE_LIB
        pDlg = pBWSDlg;
#endif
        break;
    case DAB_AT_JOURNALINE:
        pDlg = pJLDlg;
        break;
    case DAB_AT_MOTSLIDESHOW:
        pDlg = pSlideShowDlg;
        break;
    }

    if(pDlg != nullptr)
        Parameters.SetCurSelDataService(shortId);

    CService::ETyOServ eAudDataFlag = Parameters.Service[shortId].eAudDataFlag;

    Parameters.Unlock();

    if(pDlg != nullptr)
    {
        if (pDlg != pEPGDlg)
            iLastMultimediaServiceSelected = shortId;
        pDlg->show();
    }
    else
    {
        if (eAudDataFlag == CService::SF_DATA)
        {
            QMessageBox::information(this, "Dream", tr("unsupported data application"));
        }
    }
}

void FDRMDialog::OnViewMultimediaDlg()
{
    /* Check if multimedia service is available */
    if (iMultimediaServiceBit != 0)
    {
        /* Initialize the multimedia service to show, -1 mean none */
        int iMultimediaServiceToShow = -1;

        /* Check the validity of iLastMultimediaServiceSelected,
           if invalid set it to none */
        if (((1<<iLastMultimediaServiceSelected) & iMultimediaServiceBit) == 0)
            iLastMultimediaServiceSelected = -1;

        /* Simply set to the last selected multimedia if any */
        if (iLastMultimediaServiceSelected != -1)
            iMultimediaServiceToShow = iLastMultimediaServiceSelected;

        else
        {
            /* Select the first multimedia found */
            for (int i = 0; i < MAX_NUM_SERVICES; i++)
            {
                /* A bit is set when a service is available,
                   the position of the bit is the service number */
                if ((1<<i) & iMultimediaServiceBit)
                {
                    /* Service found! */
                    iMultimediaServiceToShow = i;
                    break;
                }
            }
        }
        /* If we have a service then show it */
        if (iMultimediaServiceToShow != -1)
            OnSelectDataService(iMultimediaServiceToShow);
    }
}

void FDRMDialog::OnMenuSetDisplayColor()
{
    const QColor color = CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true));
    const QColor newColor = QColorDialog::getColor(color, this);
    if (newColor.isValid())
    {
        /* Store new color and update display */
        SetDisplayColor(newColor);
        putSetting("colorscheme", CRGBConversion::RGB2int(newColor), true);
    }
}

void FDRMDialog::eventClose(QCloseEvent* ce)
{
    /* The close event has been actioned and we want to shut
     * down, but the main window should be the last thing to
     * close so that the user knows the program has completed
     * when the window closes
     */

    if(done) {
        ce->accept();
    }
    else {
        /* Request that the working thread stops */
        rx.Stop();

        /* Stop real-time timer */
        Timer.stop();

        pLogging->SaveSettings(Settings);

        CSysTray::Destroy(&pSysTray);

        AboutDlg.close();
        pAnalogDemDlg->close();
        pFMDlg->close();
        // we might have already had the signal by now
        if(done) {
            ce->accept();
        }
        else {
            ce->ignore();
        }
    }
}

void FDRMDialog::OnWorkingThreadFinished()
{
    done = true;
    close();
}

QString FDRMDialog::GetCodecString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        const CAudioParam::EAudSamRat eSamRate = service.AudioParam.eAudioSamplRate;

        /* Audio coding */
        switch (service.AudioParam.eAudioCoding)
        {
        case CAudioParam::AC_AAC:
            /* Only 12 and 24 kHz sample rates are supported for AAC encoding in DRM30 */
            if (eSamRate == CAudioParam::AS_12KHZ)
                strReturn = "aac";
            else
                strReturn = "AAC";
            break;

        case CAudioParam::AC_xHE_AAC:
            strReturn = "xHE-AAC";
            break;

        case CAudioParam::AC_OPUS:
            strReturn = "OPUS ";
            /* Opus Audio sub codec */
            switch (service.AudioParam.eOPUSSubCod)
            {
            case CAudioParam::OS_SILK:
                strReturn += "SILK ";
                break;
            case CAudioParam::OS_HYBRID:
                strReturn += "HYBRID ";
                break;
            case CAudioParam::OS_CELT:
                strReturn += "CELT ";
                break;
            }
            /* Opus Audio bandwidth */
            switch (service.AudioParam.eOPUSBandwidth)
            {
            case CAudioParam::OB_NB:
                strReturn += "NB";
                break;
            case CAudioParam::OB_MB:
                strReturn += "MB";
                break;
            case CAudioParam::OB_WB:
                strReturn += "WB";
                break;
            case CAudioParam::OB_SWB:
                strReturn += "SWB";
                break;
            case CAudioParam::OB_FB:
                strReturn += "FB";
                break;
            }
            break;
        default:;
        }

        /* SBR */
        if (service.AudioParam.eSBRFlag == CAudioParam::SB_USED)
        {
            strReturn += "+";
        }
    }
    else
    {
        /* Data service */
        strReturn = "Data:";
    }

    return strReturn;
}

QString FDRMDialog::GetTypeString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        switch (service.AudioParam.eAudioCoding)
        {
        case CAudioParam::AC_NONE:
            break;

        case CAudioParam::AC_OPUS:
            /* Opus channels configuration */
            switch (service.AudioParam.eOPUSChan)
            {
            case CAudioParam::OC_MONO:
                //            strReturn = "MONO";
                strReturn = "单声道(MONO)";
                break;

            case CAudioParam::OC_STEREO:
                strReturn = "双声道(STEREO)";
                break;
            }
            break;

        default:
            /* Mono-Stereo */
            switch (service.AudioParam.eAudioMode)
            {
            case CAudioParam::AM_MONO:
                strReturn = "单声道(MONO)";
                break;

            case CAudioParam::AM_P_STEREO:
                strReturn = "P-Stereo";
                break;

            case CAudioParam::AM_STEREO:
                strReturn = "Stereo";
                break;
            case CAudioParam::AM_RESERVED:;
            }
        }
    }
    else
    {
        /* Data service */
        if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
        {
            if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
            {
                switch (service.DataParam.iUserAppIdent)
                {
                case 1:
                    strReturn = tr("Dynamic labels");
                    break;

                case DAB_AT_MOTSLIDESHOW:
                    strReturn = tr("MOT Slideshow");
                    break;

                case DAB_AT_BROADCASTWEBSITE:
                    strReturn = tr("MOT WebSite");
                    break;

                case DAB_AT_TPEG:
                    strReturn = tr("TPEG");
                    break;

                case DAB_AT_DGPS:
                    strReturn = tr("DGPS");
                    break;

                case DAB_AT_TMC:
                    strReturn = tr("TMC");
                    break;

                case DAB_AT_EPG:
                    strReturn = tr("EPG - Electronic Programme Guide");
                    break;

                case DAB_AT_JAVA:
                    strReturn = tr("Java");
                    break;

                case DAB_AT_JOURNALINE: /* Journaline */
                    strReturn = tr("Journaline");
                    break;
                }
            }
            else
                strReturn = tr("Unknown Service");
        }
        else
            strReturn = tr("Unknown Service");
    }

    return strReturn;
}

void FDRMDialog::SetDisplayColor(const QColor newColor)
{
    /* Collect pointers to the desired controls in a vector */
    vector<QWidget*> vecpWidgets;
    //vecpWidgets.push_back(TextTextMessage);
    //vecpWidgets.push_back(LabelBitrate);
    //vecpWidgets.push_back(LabelCodec);
    //vecpWidgets.push_back(LabelStereoMono);
    //vecpWidgets.push_back(FrameAudioDataParams);
    //    vecpWidgets.push_back(LabelProgrType);
    //    vecpWidgets.push_back(LabelLanguage);
    //    vecpWidgets.push_back(LabelCountryCode);
    //    vecpWidgets.push_back(LabelServiceID);
    //    vecpWidgets.push_back(TextLabelInputLevel);
    //    vecpWidgets.push_back(ProgrInputLevel);
    //    vecpWidgets.push_back(CLED_FAC);
    //    vecpWidgets.push_back(CLED_SDC);
    //    vecpWidgets.push_back(CLED_MSC);
    vecpWidgets.push_back(FrameMainDisplay);

    for (size_t i = 0; i < vecpWidgets.size(); i++)
    {
        /* Request old palette */
        QPalette CurPal(vecpWidgets[i]->palette());

        /* Change colors */
        //        if (vecpWidgets[i] != TextTextMessage)
        //        {
        //            CurPal.setColor(QPalette::Active, QPalette::Text, newColor);
        //            CurPal.setColor(QPalette::Active, QPalette::Foreground, newColor);
        //            CurPal.setColor(QPalette::Inactive, QPalette::Text, newColor);
        //            CurPal.setColor(QPalette::Inactive, QPalette::Foreground, newColor);
        //        }
        CurPal.setColor(QPalette::Active, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Dark, newColor);

        CurPal.setColor(QPalette::Inactive, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Dark, newColor);

        /* Special treatment for text message window */
        //        if (vecpWidgets[i] == TextTextMessage)
        //        {
        //            /* We need to specify special color for disabled */
        //            CurPal.setColor(QPalette::Disabled, QPalette::Light, Qt::black);
        //            CurPal.setColor(QPalette::Disabled, QPalette::Dark, Qt::black);
        //        }

        /* Set new palette */
        vecpWidgets[i]->setPalette(CurPal);
    }
}

void FDRMDialog::AddWhatsThisHelp()
{
    /*
        This text was taken from the only documentation of Dream software
    */
    /* Text Message */
    QString strTextMessage =
            tr("<b>Text Message:</b> On the top right the text "
               "message label is shown. This label only appears when an actual text "
               "message is transmitted. If the current service does not transmit a "
               "text message, the label will be disabled.");

    /* Input Level */
    const QString strInputLevel =
            tr("<b>Input Level:</b> The input level meter shows "
               "the relative input signal peak level in dB. If the level is too high, "
               "the meter turns from green to red. The red region should be avoided "
               "since overload causes distortions which degrade the reception "
               "performance. Too low levels should be avoided too, since in this case "
               "the Signal-to-Noise Ratio (SNR) degrades.");


    /* Status LEDs */
    const QString strStatusLEDS =
            tr("<b>Status LEDs:</b> The three status LEDs show "
               "the current CRC status of the three logical channels of a DRM stream. "
               "These LEDs are the same as the top LEDs on the Evaluation Dialog.");


    /* Station Label and Info Display */
    const QString strStationLabelOther =
            tr("<b>Station Label and Info Display:</b> In the "
               "big label with the black background the station label and some other "
               "information about the current selected service is displayed. The "
               "magenta text on the top shows the bit-rate of the current selected "
               "service (The abbreviations EEP and "
               "UEP stand for Equal Error Protection and Unequal Error Protection. "
               "UEP is a feature of DRM for a graceful degradation of the decoded "
               "audio signal in case of a bad reception situation. UEP means that "
               "some parts of the audio is higher protected and some parts are lower "
               "protected (the ratio of higher protected part length to total length "
               "is shown in the brackets)), the audio compression format "
               "(e.g. AAC), if SBR is used and what audio mode is used (Mono, Stereo, "
               "P-Stereo -> low-complexity or parametric stereo). In case SBR is "
               "used, the actual sample rate is twice the sample rate of the core AAC "
               "decoder. The next two types of information are the language and the "
               "program type of the service (e.g. German / News).<br>The big "
               "turquoise text in the middle is the station label. This label may "
               "appear later than the magenta text since this information is "
               "transmitted in a different logical channel of a DRM stream. On the "
               "right, the ID number connected with this service is shown.");


    /* Service Selectors */
    const QString strServiceSel =
            tr("<b>Service Selectors:</b> In a DRM stream up to "
               "four services can be carried. The service can be an audio service, "
               "a data service or an audio service with data. "
               "Audio services can have associated text messages, in addition to any data component. "
               "If a Multimedia data service is selected, the Multimedia Dialog will automatically show up. "
               "On the right of each service selection button a short description of the service is shown. "
               "If an audio service has associated Multimedia data, \"+ MM\" is added to this text. "
               "If such a service is selected, opening the Multimedia Dialog will allow the data to be viewed "
               "while the audio is still playing. If the data component of a service is not Multimedia, "
               "but an EPG (Electronic Programme Guide) \"+ EPG\" is added to the description. "
               "The accumulated Programme Guides for all stations can be viewed by opening the Programme Guide Dialog. "
               "The selected channel in the Programme Guide Dialog defaults to the station being received. "
               "If Alternative Frequency Signalling is available, \"+ AFS\" is added to the description. "
               "In this case the alternative frequencies can be viewed by opening the Live Schedule Dialog."
               );

    //    TextTextMessage->setWhatsThis(strTextMessage);
    //    TextLabelInputLevel->setWhatsThis(strInputLevel);
    //    ProgrInputLevel->setWhatsThis(strInputLevel);
    //    CLED_MSC->setWhatsThis(strStatusLEDS);
    //    CLED_SDC->setWhatsThis(strStatusLEDS);
    //    CLED_FAC->setWhatsThis(strStatusLEDS);
    //LabelBitrate->setWhatsThis(strStationLabelOther);
    //LabelCodec->setWhatsThis(strStationLabelOther);
    //LabelStereoMono->setWhatsThis(strStationLabelOther);
    //LabelServiceLabel->setWhatsThis(strStationLabelOther);
    //    LabelProgrType->setWhatsThis(strStationLabelOther);
    //    LabelServiceID->setWhatsThis(strStationLabelOther);
    //    LabelLanguage->setWhatsThis(strStationLabelOther);
    //    LabelCountryCode->setWhatsThis(strStationLabelOther);
    //FrameAudioDataParams->setWhatsThis(strStationLabelOther);
    PushButtonService1->setWhatsThis(strServiceSel);
    PushButtonService2->setWhatsThis(strServiceSel);
    PushButtonService3->setWhatsThis(strServiceSel);
    PushButtonService4->setWhatsThis(strServiceSel);
    TextMiniService1->setWhatsThis(strServiceSel);
    TextMiniService2->setWhatsThis(strServiceSel);
    TextMiniService3->setWhatsThis(strServiceSel);
    TextMiniService4->setWhatsThis(strServiceSel);

    const QString strBars = tr("from 1 to 5 bars indicates WMER in the range 8 to 24 dB");
    //	onebar->setWhatsThis(strBars);
    //	twobars->setWhatsThis(strBars);
    //	threebars->setWhatsThis(strBars);
    //	fourbars->setWhatsThis(strBars);
    //	fivebars->setWhatsThis(strBars);
}
