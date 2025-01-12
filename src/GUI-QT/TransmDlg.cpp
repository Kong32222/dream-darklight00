/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#include "TransmDlg.h"
#include "SoundCardSelMenu.h"
#include <QCloseEvent>
#include <QTreeWidget>
#include <QFileDialog>
#include <QTextEdit>
#include <QProgressBar>
#include <QHeaderView>
#include <QWhatsThis>

#include "../main-Qt/ctx.h"
#include "../receivermanager.h"
#include "../csubdrmreceiver.h"
#include <QDebug>


TransmDialog::TransmDialog(CTx& ntx, QWidget* parent)
    :
      CWindow(parent, *ntx.GetSettings(), "Transmit"),
      tx(ntx),
      vecstrTextMessage(1) /* 1 for new text */,
      pAACCodecDlg(nullptr),pOpusCodecDlg(nullptr), pSysTray(nullptr),
      pActionStartStop(nullptr), bIsStarted(false),
      iIDCurrentText(0), iServiceDescr(0),
      bCloseRequested(false), iButtonCodecState(0)
{
    setupUi(this);
#if QWT_VERSION < 0x060100
    ProgrInputLevel->setScalePosition(QwtThermo::BottomScale);
#else
    ProgrInputLevel->setScalePosition(QwtThermo::LeadingScale);
#endif

    /* Load transmitter settings */
    tx.LoadSettings();

    /* Set help text for the controls */
    AddWhatsThisHelp();

    /* Init controls with default settings [Start]  tx.start(); */
    ButtonStartStop->setText(tr("&启动"));
    OnButtonClearAllText();
    UpdateMSCProtLevCombo();

    /* Init progress bar for input signal level */
#if QWT_VERSION < 0x060100
    ProgrInputLevel->setRange(-50.0, 0.0);
    ProgrInputLevel->setOrientation(Qt::Horizontal, QwtThermo::BottomScale);
#else
    ProgrInputLevel->setScale(-50.0, 0.0);
    ProgrInputLevel->setOrientation(Qt::Horizontal);
    ProgrInputLevel->setScalePosition(QwtThermo::LeadingScale);
#endif
    ProgrInputLevel->setAlarmLevel(-5.0);
#if QWT_VERSION < 0x060000
    ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));
    ProgrInputLevel->setFillColor(QColor(0, 190, 0));
#else
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
    newPalette.setColor(QPalette::ButtonText, QColor(0, 190, 0));
    newPalette.setColor(QPalette::Highlight,  QColor(255, 0, 0));
    ProgrInputLevel->setPalette(newPalette);
#endif

    // 1 2 3 page switch
    connect(pushButton_source,&QPushButton::clicked,this,
            &TransmDialog::Onswitch_sourse);
    //&TransmDialog::Onswitch_reuse
    connect(pushButton_reuse, &QPushButton::clicked,this,
            &TransmDialog::Onswitch_reuse );
    connect(pushButton_modulation,&QPushButton::clicked,this,
            &TransmDialog::Onswitch_modulation);
    connect(&Timer_500ms, &QTimer::timeout, this, &TransmDialog::onRefresh_Tx_Ui);


#if 1
    // 所有连接的DMI源  list
    // 创建一个字符串列表模型
    model = new QStringListModel();

    // 创建一个包含 10 项的列表
    for (int i = 1; i <= 3; ++i)
    {
        list << QString("消息源 %1: xxxx").arg(i);
    }
    model->setStringList(list);

    // 将模型设置到 QListView
    listView->setModel(model);


    // QObject::
    connect(listView, &QListView::clicked, [&](const QModelIndex &index)
    {
        int pageIndex = index.row();
        // 对应的界面
        if (pageIndex >= 0 && pageIndex < stackedWidget->count())
        {
            stackedWidget_2->setCurrentIndex(pageIndex);
            stackedWidget_3->setCurrentIndex(pageIndex);
        }
    });



    // 将每一项添加到ComboBox中  TODO 之后就是4个流, 都要添加 并且加if判断 激活的才会出现
    for (const QString &item : list)
    {
        ComboBoxSelectiveSource1->addItem(item);
    }

    for (const QString &item : list)
    {
        ComboBoxSelectiveSource2->addItem(item);
    }

    for (const QString &item : list)
    {
        ComboBoxSelectiveSource3->addItem(item);
    }
    for (const QString &item : list)
    {
        ComboBoxSelectiveSource4->addItem(item);
    }

#endif

    //my自定义 输出设备
    pushButton_inputdev->setEnabled(false);
    pushButton_outdev->setEnabled(false);

    // 获取系统中的所有音频输出设备
    QList<QAudioDeviceInfo> devices =
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    // 如果设备列表不为空，添加设备名字到 comboBox_tr
    if (!devices.isEmpty())
    {
        // 添加第一个设备到 comboBox_tr 的顶部
        comboBox_tr->insertItem(0, devices.first().deviceName());

        // 遍历其余设备，将设备名字添加到 comboBox_tr 中
        for (int i = 1; i < devices.size(); ++i)
        {
            comboBox_tr->addItem(devices[i].deviceName());
        }
    }
    else
    {
        // 如果设备列表为空，添加一个无可用设备的提示
        comboBox_tr->addItem("No available devices");
    }

    // 输入设备
    // 获取系统中的所有音频输入设备
    QList<QAudioDeviceInfo> devices_rx =
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    // 如果设备列表不为空，添加设备名字到 comboBox_tr
    if (!devices_rx.isEmpty())
    {
        // 添加第一个设备到 comboBox_tr 的顶部
        comboBox_rx->insertItem(0, devices.first().deviceName());

        // 遍历其余设备，将设备名字添加到 comboBox_tr 中
        for (int i = 1; i < devices.size(); ++i)
        {
            comboBox_rx->addItem(devices[i].deviceName());
        }
    }
    else
    {
        // 如果设备列表为空，添加一个无可用设备的提示
        comboBox_rx->addItem("No available devices");
    }


    //----  源码   ----
    /* Init progress bar for current transmitted picture */
    ProgressBarCurPict->setRange(0, 100);
    ProgressBarCurPict->setValue(0);
    TextLabelCurPict->setText("");

    /* Output mode (real valued, I / Q or E / P) */
    // cout<<"Output mode ( = "<<tx.GetIQOutput()<<endl;
    switch (tx.GetIQOutput())
    {
    case CTransmitData::OF_REAL_VAL:
        RadioButtonOutReal->setChecked(true);
        break;

    case CTransmitData::OF_IQ_POS:
        RadioButtonOutIQPos->setChecked(true);
        break;

    case CTransmitData::OF_IQ_NEG:
        RadioButtonOutIQNeg->setChecked(true);
        break;

    case CTransmitData::OF_EP:
        RadioButtonOutEP->setChecked(true);
        break;
    }

    /* Output High Quality I/Q */
    CheckBoxHighQualityIQ->setEnabled(tx.GetIQOutput() != CTransmitData::OF_REAL_VAL);
    CheckBoxHighQualityIQ->setChecked(tx.isHighQualityIQ());

    /* Output Amplified */
    CheckBoxAmplifiedOutput->setEnabled(tx.GetIQOutput() != CTransmitData::OF_EP);
    CheckBoxAmplifiedOutput->setChecked(tx.isOutputAmplified());

    /* Don't lock the Parameter object since the working thread is stopped */
    CParameter& Parameters = *tx.GetParameters();

    /* Transmission of current time */
    switch (Parameters.eTransmitCurrentTime)
    {
    case CParameter::CT_OFF:
        RadioButtonCurTimeOff->setChecked(true);
        break;

    case CParameter::CT_LOCAL:
        RadioButtonCurTimeLocal->setChecked(true);
        break;

    case CParameter::CT_UTC:
        RadioButtonCurTimeUTC->setChecked(true);
        break;

    case CParameter::CT_UTC_OFFSET:
        RadioButtonCurTimeUTCOffset->setChecked(true);
    }

    /* Robustness mode */
    switch (Parameters.GetWaveMode())
    {
    case RM_ROBUSTNESS_MODE_A:
        RadioButtonRMA->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_B:
        RadioButtonRMB->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_C:
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        RadioButtonRMC->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_D:
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        RadioButtonRMD->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_E:
        break;

    case RM_NO_MODE_DETECTED:
        break;
    }

    /* Bandwidth */
    switch (Parameters.GetSpectrumOccup())
    {
    case SO_0:
        RadioButtonBandwidth45->setChecked(true);
        break;

    case SO_1:
        RadioButtonBandwidth5->setChecked(true);
        break;

    case SO_2:
        RadioButtonBandwidth9->setChecked(true);
        break;

    case SO_3:
        RadioButtonBandwidth10->setChecked(true);
        break;

    case SO_4:
        RadioButtonBandwidth18->setChecked(true);
        break;

    case SO_5:
        RadioButtonBandwidth20->setChecked(true);
        break;
    }

    /* MSC interleaver mode 长交织 */
    //	ComboBoxMSCInterleaver->insertItem(0, tr("2 s (Long Interleaving)"));
    //	ComboBoxMSCInterleaver->insertItem(1, tr("400 ms (Short Interleaving)"));
    ComboBoxMSCInterleaver->insertItem(0, tr("2 s (长交织)"));
    ComboBoxMSCInterleaver->insertItem(1, tr("400 ms (短交织)"));
    switch (Parameters.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        ComboBoxMSCInterleaver->setCurrentIndex(0);
        break;

    case CParameter::SI_SHORT:
        ComboBoxMSCInterleaver->setCurrentIndex(1);
        break;
    }

    /* MSC Constellation Scheme */
    ComboBoxMSCConstellation->insertItem(0, tr("SM 16-QAM"));
    ComboBoxMSCConstellation->insertItem(1, tr("SM 64-QAM"));

    // These modes should not be used right now, TODO
    // DF: I reenabled those, because it seems to work, at least with dream
    ComboBoxMSCConstellation->insertItem(2, tr("HMsym 64-QAM"));
    ComboBoxMSCConstellation->insertItem(3, tr("HMmix 64-QAM"));

    switch (Parameters.eMSCCodingScheme)
    {
    case CS_1_SM:
        break;

    case CS_2_SM:
        ComboBoxMSCConstellation->setCurrentIndex(0);
        break;

    case CS_3_SM:
        ComboBoxMSCConstellation->setCurrentIndex(1);
        break;

    case CS_3_HMSYM:
        ComboBoxMSCConstellation->setCurrentIndex(2);
        break;

    case CS_3_HMMIX:
        ComboBoxMSCConstellation->setCurrentIndex(3);
        break;
    }

    /* SDC Constellation Scheme */
    ComboBoxSDCConstellation->insertItem(0, tr("4-QAM"));
    ComboBoxSDCConstellation->insertItem(1, tr("16-QAM"));

    switch (Parameters.eSDCCodingScheme)
    {
    case CS_1_SM:
        ComboBoxSDCConstellation->setCurrentIndex(0);
        break;

    case CS_2_SM:
        ComboBoxSDCConstellation->setCurrentIndex(1);
        break;

    case CS_3_SM:
    case CS_3_HMSYM:
    case CS_3_HMMIX:
        break;
    }


    /* Service parameters --------------------------------------------------- */
    /* Service label */
    CService& Service = Parameters.Service[0]; // TODO
    QString label = QString::fromUtf8(Service.strLabel.c_str());
    LineEditServiceLabel->setText(label);

    /* Service ID */
    LineEditServiceID->setText(QString().setNum(int(Service.iServiceID), 16));


    int i;
    /* Language */
    for (i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
        ComboBoxLanguage->insertItem(i, strTableLanguageCode[i].c_str());

    ComboBoxLanguage->setCurrentIndex(Service.iLanguage);

    /* Program type */
    for (i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
        ComboBoxProgramType->insertItem(i, strTableProgTypCod[i].c_str());

    /* Service description */
    iServiceDescr = Service.iServiceDescr;
    ComboBoxProgramType->setCurrentIndex(iServiceDescr);

    /* 声卡IF  Sound card IF */

    LineEditSndCrdIF->setText(QString().number(tx.GetCarrierOffset(),
                                               'f', 2));

    /* Clear list box for file names */
    OnButtonClearAllFileNames();

    /* Disable other three services */
    TabWidgetServices->setTabEnabled(1, true);//false
    TabWidgetServices->setTabEnabled(2, true);//false
    TabWidgetServices->setTabEnabled(3, true);//false

    //  server1 默认是不选
    CheckBoxEnableService->setChecked(false);
    //  server1 是否可选操作
    CheckBoxEnableService->setEnabled(true);

    /* Setup audio codec check boxes */
    switch (Service.AudioParam.eAudioCoding)
    {
    case CAudioParam::AC_AAC:
        RadioButtonAAC->setChecked(true);
        ShowButtonCodec(false, 1);
        break;

    case CAudioParam::AC_OPUS:
        RadioButtonOPUS->setChecked(true);
        ShowButtonCodec(true, 1);
        break;

    default:
        ShowButtonCodec(false, 1);
        break;
    }
    if (!tx.CanEncode(CAudioParam::AC_AAC)) {
        RadioButtonAAC->setText(tr("No DRM capable AAC Codec"));
        RadioButtonAAC->setToolTip(tr("see http://drm.sourceforge.net"));
        RadioButtonAAC->setEnabled(false);
        //RadioButtonAAC->hide();
    }
    if (!tx.CanEncode(CAudioParam::AC_OPUS)) {
        RadioButtonOPUS->setText(tr("No Opus Codec"));
        RadioButtonOPUS->setToolTip(tr("see http://drm.sourceforge.net"));
        RadioButtonOPUS->setEnabled(false);
        //RadioButtonOPUS->hide();
    }
    if (!tx.CanEncode(CAudioParam::AC_AAC)
            && !tx.CanEncode(CAudioParam::AC_OPUS)) {
        /* Let this service be an data service */
        CheckBoxEnableAudio->setChecked(false);
        CheckBoxEnableAudio->setEnabled(false);
        EnableAudio(false);
        CheckBoxEnableData->setChecked(true);
        EnableData(true);
    }
    else {
        /* Let this service be an audio service for initialization */
        /* Set audio enable check box */
        CheckBoxEnableAudio->setChecked(true);
        EnableAudio(true);
        CheckBoxEnableData->setChecked(false);
        EnableData(false);
    }

    /* Add example text message at startup ---------------------------------- */
    /* Activate text message */
    EnableTextMessage(true);
    CheckBoxEnableTextMessage->setChecked(true);

    /* Add example text in internal container */
    vecstrTextMessage.Add(
                tr("DRM Transmitter\x0B\x0AThis is a test transmission").toUtf8().constData());

    /* Insert item in combo box, display text and set item to our text */
    ComboBoxTextMessage->insertItem(1, QString().setNum(1));
    ComboBoxTextMessage->setCurrentIndex(1);

    /* Update the TextEdit with the default text */
    OnComboBoxTextMessageActivated(1);

    /* Now make sure that the text message flag is activated in global struct */
    Service.AudioParam.bTextflag = true;


    /* Enable all controls */
    EnableAllControlsForSet();


    /* Set check box remove path */
    CheckBoxRemovePath->setChecked(true);
    OnToggleCheckBoxRemovePath(true);


    /* Set Menu ***************************************************************/
    // 发射机的 setting 控件
    CFileMenu* pFileMenu = new CFileMenu(tx, this, menu_Settings);

    menu_Settings->addMenu(new CSoundCardSelMenu(tx, pFileMenu, this));

    connect(actionAbout_Dream, SIGNAL(triggered()), &AboutDlg, SLOT(show()));
    connect(actionWhats_This, SIGNAL(triggered()), reinterpret_cast<QObject*>(this), SLOT(OnWhatsThis()));

    /* Connections ---------------------------------------------------------- */
    /* Push buttons */
    connect(ButtonStartStop, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnButtonStartStop()));

    connect(ButtonCodec, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnButtonCodec()));
    connect(PushButtonAddText, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnPushButtonAddText()));
    connect(PushButtonClearAllText, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnButtonClearAllText()));
    connect(PushButtonAddFile, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnPushButtonAddFileName()));
    connect(PushButtonClearAllFileNames, SIGNAL(clicked()),
            reinterpret_cast<QObject*>(this), SLOT(OnButtonClearAllFileNames()));

    /* Check boxes */
    connect(CheckBoxHighQualityIQ, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxHighQualityIQ(bool)));
    connect(CheckBoxAmplifiedOutput, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxAmplifiedOutput(bool)));
    connect(CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
    connect(CheckBoxEnableAudio, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxEnableAudio(bool)));
    connect(CheckBoxEnableData, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxEnableData(bool)));
    connect(CheckBoxRemovePath, SIGNAL(toggled(bool)),
            reinterpret_cast<QObject*>(this), SLOT(OnToggleCheckBoxRemovePath(bool)));

    /* Combo boxes */
    connect(ComboBoxMSCInterleaver, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxMSCInterleaverActivated(int)));
    connect(ComboBoxMSCConstellation, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxMSCConstellationActivated(int)));
    connect(ComboBoxSDCConstellation, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxSDCConstellationActivated(int)));
    connect(ComboBoxLanguage, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxLanguageActivated(int)));
    connect(ComboBoxProgramType, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxProgramTypeActivated(int)));
    connect(ComboBoxTextMessage, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxTextMessageActivated(int)));
    connect(ComboBoxMSCProtLev, SIGNAL(activated(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnComboBoxMSCProtLevActivated(int)));

    /* Button groups */
    connect(ButtonGroupRobustnessMode, SIGNAL(buttonClicked(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnRadioRobustnessMode(int)));
    connect(ButtonGroupBandwidth, SIGNAL(buttonClicked(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnRadioBandwidth(int)));
    connect(ButtonGroupOutput, SIGNAL(buttonClicked(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnRadioOutput(int)));
    connect(ButtonGroupCodec, SIGNAL(buttonClicked(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnRadioCodec(int)));
    connect(ButtonGroupCurrentTime, SIGNAL(buttonClicked(int)),
            reinterpret_cast<QObject*>(this), SLOT(OnRadioCurrentTime(int)));

    /* Line edits */
    connect(LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
            reinterpret_cast<QObject*>(this), SLOT(OnTextChangedServiceLabel(const QString&)));
    connect(LineEditServiceID, SIGNAL(textChanged(const QString&)),
            reinterpret_cast<QObject*>(this),
            SLOT(OnTextChangedServiceID(const QString&)));

    // 6000.0 kHz
    connect(LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
            reinterpret_cast<QObject*>(this), SLOT(OnTextChangedSndCrdIF(const QString&)));

    /* Timers */
    connect(&Timer, SIGNAL(timeout()), reinterpret_cast<QObject*>(this), SLOT(OnTimer()));
    connect(&TimerStop, SIGNAL(timeout()), reinterpret_cast<QObject*>(this), SLOT(OnTimerStop()));

    /* System tray setup  系统托盘设置 */
    pSysTray = CSysTray::Create(reinterpret_cast<QWidget*>(this),
                                SLOT(OnSysTrayActivated(QSystemTrayIcon::ActivationReason)),
                                nullptr, ":/icons/MainIconTx.svg");

    pActionStartStop = CSysTray::AddAction(pSysTray,
                                           ButtonStartStop->text(),
                                           reinterpret_cast<QObject*>(this),
                                           SLOT(OnButtonStartStop()));

    CSysTray::AddSeparator(pSysTray);
    CSysTray::AddAction(pSysTray, tr("&Exit"), reinterpret_cast<QObject*>(this), SLOT(close()));
    CSysTray::SetToolTip(pSysTray, QString(), tr("Stopped"));

    /* Set timer for real-time controls */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
    TimerStop.setSingleShot(true);
    EnableAllControlsForSet();

    //my timer
    Timer_500ms.start(500);

}

TransmDialog::~TransmDialog()
{
    /* Destroy codec dialog if exist */
    if (pAACCodecDlg)
        delete pAACCodecDlg;
    if (pOpusCodecDlg)
        delete pOpusCodecDlg;
}

void TransmDialog::eventClose(QCloseEvent* ce)
{
    bCloseRequested = true;
    if (bIsStarted)
    {
        OnButtonStartStop();
        ce->ignore();
    }
    else {
        CSysTray::Destroy(&pSysTray);

        /* Stop transmitter */
        if (bIsStarted)
            tx.Stop();

        /* Restore the service description, may
           have been reset by data service */
        CParameter& Parameters = *tx.GetParameters();
        Parameters.Lock();
        CService& Service = Parameters.Service[0]; // TODO
        Service.iServiceDescr = iServiceDescr;

        /* Save transmitter settings */
        tx.SaveSettings();
        Parameters.Unlock();

        ce->accept();
    }
}

void TransmDialog::OnWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}



void TransmDialog::OnSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger
        #if QT_VERSION < 0x050000
            || reason == QSystemTrayIcon::DoubleClick
        #endif
            )
    {
        const Qt::WindowStates ws = windowState();
        if (ws & Qt::WindowMinimized)
            setWindowState((ws & ~Qt::WindowMinimized) | Qt::WindowActive);
        else
            toggleVisibility();
    }
}

//构造中start 信号与槽连接（信号槽）
void TransmDialog::OnTimer()
{
    /* Set value for input level meter (only in "start" mode) */
    if (bIsStarted)
    {
        //  cout<<"bIsStarted"<<endl;  //测试结果是 ：当用户 点击了 开始按钮， 才会进入if
        ProgrInputLevel->setValue(tx.GetLevelMeter());

        string strCPictureName;
        _REAL rCPercent = 0.0;

        /* Activate progress bar for slide show pictures only if current state
           can be queried and if data service is active
           (check box is checked) */
        if (tx.GetTransmissionStatus(strCPictureName, rCPercent) && CheckBoxEnableData->isChecked())
        {
            /* Enable controls */
            ProgressBarCurPict->setEnabled(true);
            TextLabelCurPict->setEnabled(true);

            /* We want to file name, not the complete path -> "QFileInfo" */
            QFileInfo FileInfo(strCPictureName.c_str());

            /* Show current file name and percentage */
            TextLabelCurPict->setText(FileInfo.fileName());
            ProgressBarCurPict->setValue((int) (rCPercent * 100)); /* % */
        }
        else
        {
            /* Disable controls */
            ProgressBarCurPict->setEnabled(false);
            TextLabelCurPict->setEnabled(false);
        }

        //my code 定时刷新 我的调制信息 复用信息
        if(1)
        {
            CParameter& Parameters = *(tx.GetParameters());
            QString FACInfo;
            /***鲁棒模式:**/
            FACInfo = "鲁棒模式:\t"; // 固定前缀
            FACInfo +=  GetRobmodestr(Parameters); // 获取新的 DRM 模式
            this->label_Robustmode->setText(FACInfo); //鲁棒模式


            // this->label_iFrameIDTransm;  //超帧 中FAC块的索引


            /**交织情况：***/
            FACInfo = "交织深度:\t"; // 固定前缀
            FACInfo += GetInterl(Parameters);
            this->label_SymbolInterlMode->setText(FACInfo);

            /**MSC mode ***/
            FACInfo = "MSC mode:\t"; // 固定前缀
            FACInfo += GetMSCmode(Parameters); // 获取新的
            this->label_MSCmode->setText(FACInfo);


            /**SDC mode ***/
            FACInfo = "SDC mode:\t"; // 固定前缀
            FACInfo += GetSDCmode(Parameters);
            this->label_SDCnode->setText(FACInfo);



            /**服务个数 **/
            QString AudioService_DataService;
            AudioService_DataService = tr("Audio:");
            AudioService_DataService += QString().setNum(Parameters.iNumAudioService);
            AudioService_DataService += tr("/ Data:");
            AudioService_DataService += QString().setNum(Parameters.iNumDataService);
            this->label_TableNumOfServices->setText(AudioService_DataService);

            // this->label_CurShortID;
            // FACRepetition[FACRepetitionCounter];
            int iCurShortID = 0;


            // 24位 this->label_ServiceID;
            // Parameter.Service[iCurShortID].iServiceID
            QString serviveID;
            serviveID = "服务ID:\t";
            serviveID += QString::number(Parameters.Service[iCurShortID].iServiceID);
            this->label_ServiceID->setText(serviveID);


            /**频谱占用：**/
            QString SpectrumOccup;
            SpectrumOccup = GetSpectrumOccup(Parameters);
            FACInfo = "频谱占用:\t";
            FACInfo += SpectrumOccup;
            this->label_SpectrumOccup->setText(FACInfo);

            // TODO
            // SDC::


            /*server: 4 */
            QString Server;
            SpectrumOccup = GetSpectrumOccup(Parameters);


            for(int i =0 ; i< 4;i++)
            {
                //  Parameters.Service[i]; 根据参数中 是否活跃
                CService service = Parameters.Service[i];
                QString strLabel = "服务"+QString::number(i)+ ":";
                strLabel += QString().fromUtf8(service.strLabel.c_str());
                // cout<<i <<"serverlabel = "<< strLabel.toStdString()<<endl;
                if(service.IsActive())
                {
                    switch (i)
                    {
                    case 0:
                        this->label_server0->setText(strLabel);
                        break;
                    case 1:
                        this->label_server1->setText(strLabel);
                        break;
                    case 2:
                        this->label_server2->setText(strLabel);
                        break;
                    case 3:
                        this->label_server3->setText(strLabel);
                        break;
                    default :
                        break;
                    }
                }
                else
                {
                    strLabel += " 无";
                    switch (i)
                    {
                    case 0:
                        this->label_server0->setText(strLabel);
                        break;
                    case 1:
                        this->label_server1->setText(strLabel);
                        break;
                    case 2:
                        this->label_server2->setText(strLabel);
                        break;
                    case 3:
                        this->label_server3->setText(strLabel);
                        break;
                    default :
                        break;
                    }
                }

            }
        }
    }
    else
    {
        //  cout<<"no IsStarted"<<endl;  //my
    }
}

void TransmDialog::OnTimerStop()
{
    if (tx.isRunning())
    {
        // 发送 线程  计时器  ，停止重试
        //        cerr << "transmitter thread is running on timer stop - retrying"
        //             << endl;

        /* Request a transmitter stop */
        tx.Stop();

        /* Start the timer for polling the thread state */
        TimerStop.start(50);
        return;
    }

    bIsStarted = false;

    if (bCloseRequested)
    {
        close();
        return;
    }

    ButtonStartStop->setText(tr("开始"));// &Start
    if (pActionStartStop) {
        pActionStartStop->setText(ButtonStartStop->text());
    }
    CSysTray::SetToolTip(pSysTray, QString(), tr("Stopped"));

    EnableAllControlsForSet();
}


//启动 & 关闭
void TransmDialog::OnButtonStartStop()
{
    // 再次点击
    if (!TimerStop.isActive())
    {
        // 如果是活动的话， 就变成 Stopping...
        if (bIsStarted)
        {
            ButtonStartStop->setText(tr("Stopping..."));
            if (pActionStartStop)
                pActionStartStop->setText(ButtonStartStop->text());
            CSysTray::SetToolTip(pSysTray, QString(),
                                 ButtonStartStop->text());

            // DEBUG_COUT("点击了关闭");
            /* Request a transmitter stop */
            tx.Stop();

            /* Start the timer for polling the thread state */
            TimerStop.start(50);
        }
        else
        {
            int i;
            DEBUG_COUT("点击了启动");
            // 启动
            /* Start transmitter */
            /* Set text message */
            tx.ClearTextMessage();

            for (i = 1; i < vecstrTextMessage.Size(); i++)
                tx.SetTextMessage(vecstrTextMessage[i]);

            /* Set file names for data application */
            tx.ClearPicFileNames();

            /* Iteration through table widget items
             * 遍历表  widget项  */
            int count = TreeWidgetFileNames->topLevelItemCount();

            for (i = 0; i < count ; i++)
            {
                /* Complete file path is in third column
                 * 完整的文件路径在第三列 */
                QTreeWidgetItem* item =
                        TreeWidgetFileNames->topLevelItem(i);

                if (item)
                {
                    /* Get the file path  */
                    const QString strFileName = item->text(2);
                    DEBUG_COUT("发射机 strFileName = "<< strFileName);
                    /* Extract format string
                        提取格式字符串*/
                    QFileInfo FileInfo(strFileName);
                    const QString strFormat = FileInfo.suffix();

                    tx.SetPicFileName(strFileName.toStdString(),
                                      strFormat.toStdString());
                }
            }

            //my 的函数(不好用  测试用的)
            //            cout<<"启动之前调用了onParametersReceived(); "<<endl;
            //            onParametersReceived();


            tx.start();

            ButtonStartStop->setText(tr("停止")); //&Stop
            if (pActionStartStop)
                pActionStartStop->setText(ButtonStartStop->text());
            CSysTray::SetToolTip(pSysTray, QString(), tr("Transmitting"));

            DisableAllControlsForSet();

            bIsStarted = true;
        }
    }
}

void TransmDialog::OnToggleCheckBoxHighQualityIQ(bool bState)
{
    tx.SetHighQualityIQ(bState);
}

void TransmDialog::OnToggleCheckBoxAmplifiedOutput(bool bState)
{
    tx.SetOutputAmplified(bState);
}

void TransmDialog::OnToggleCheckBoxEnableTextMessage(bool bState)
{
    EnableTextMessage(bState);
}

void TransmDialog::EnableTextMessage(const bool bFlag)
{
    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();

    if (bFlag)
    {
        /* Enable text message controls */
        ComboBoxTextMessage->setEnabled(true);
        MultiLineEditTextMessage->setEnabled(true);
        PushButtonAddText->setEnabled(true);
        PushButtonClearAllText->setEnabled(true);

        /* Set text message flag in global struct */
        Parameters.Service[0].AudioParam.bTextflag = true;
    }
    else
    {
        /* Disable text message controls */
        ComboBoxTextMessage->setEnabled(false);
        MultiLineEditTextMessage->setEnabled(false);
        PushButtonAddText->setEnabled(false);
        PushButtonClearAllText->setEnabled(false);

        /* Set text message flag in global struct */
        Parameters.Service[0].AudioParam.bTextflag = false;
    }

    Parameters.Unlock();
}

void TransmDialog::OnToggleCheckBoxEnableAudio(bool bState)
{
    EnableAudio(bState);

    if (bState)
    {
        /* Set audio enable check box */
        CheckBoxEnableData->setChecked(false);
        EnableData(false);
        ShowButtonCodec(true, 2);
    }
    else
    {
        /* Set audio enable check box */
        CheckBoxEnableData->setChecked(true);
        EnableData(true);
        ShowButtonCodec(true, 2);
    }
}

void TransmDialog::EnableAudio(const bool bFlag)
{
    if (bFlag)
    {
        /* Enable audio controls */
        GroupBoxCodec->setEnabled(true);
        GroupBoxTextMessage->setEnabled(true);
        ComboBoxProgramType->setEnabled(true);

        CParameter& Parameters = *tx.GetParameters();

        Parameters.Lock();

        /* Only one audio service */
        Parameters.iNumAudioService = 1;
        Parameters.iNumDataService = 0;

        /* Audio flag of this service */
        Parameters.Service[0].eAudDataFlag = CService::SF_AUDIO;

        /* Always use stream number 0 right now, TODO */
        Parameters.Service[0].AudioParam.iStreamID = 0;

        /* Programme Type code */
        Parameters.Service[0].iServiceDescr = iServiceDescr;

        Parameters.Unlock();
    }
    else
    {
        /* Disable audio controls */
        GroupBoxCodec->setEnabled(false);
        GroupBoxTextMessage->setEnabled(false);
        ComboBoxProgramType->setEnabled(false);
    }
}

void TransmDialog::OnToggleCheckBoxEnableData(bool bState)
{
    EnableData(bState);

    if (bState)
    {
        /* Set audio enable check box */
        CheckBoxEnableAudio->setChecked(false);
        EnableAudio(false);
    }
    else
    {
        /* Set audio enable check box */
        CheckBoxEnableAudio->setChecked(true);
        EnableAudio(true);
    }
}

void TransmDialog::EnableData(const bool bFlag)
{
    /* Enable/Disable data controls */
    CheckBoxRemovePath->setEnabled(bFlag);
    TreeWidgetFileNames->setEnabled(bFlag);
    PushButtonClearAllFileNames->setEnabled(bFlag);
    PushButtonAddFile->setEnabled(bFlag);

    if (bFlag)
    {
        CParameter& Parameters = *tx.GetParameters();

        Parameters.Lock();

        /* Only one data service */
        Parameters.iNumAudioService = 0;
        Parameters.iNumDataService = 1;

        /* Data flag for this service */
        Parameters.Service[0].eAudDataFlag = CService::SF_DATA;

        /* Always use stream number 0, TODO */
        Parameters.Service[0].DataParam.iStreamID = 0;

        /* Init SlideShow application */
        Parameters.Service[0].DataParam.iPacketLen = 45; /* TEST */
        Parameters.Service[0].DataParam.eDataUnitInd = CDataParam::DU_DATA_UNITS;
        Parameters.Service[0].DataParam.eAppDomain = CDataParam::AD_DAB_SPEC_APP;

        /* The value 0 indicates that the application details are provided
           solely by SDC data entity type 5 */
        Parameters.Service[0].iServiceDescr = 0;

        Parameters.Unlock();
    }
}

bool TransmDialog::GetMessageText(const int iID)
{
    bool bTextIsNotEmpty = true;

    /* Check if text control is not empty */
    if (!MultiLineEditTextMessage->toPlainText().isEmpty())
    {
        /* Check size of container. If not enough space, enlarge */
        if (iID == vecstrTextMessage.Size())
            vecstrTextMessage.Enlarge(1);

        /* DF: I did some test on both Qt3 and Qt4, and
           UTF8 char are well preserved */
        /* Get the text from MultiLineEditTextMessage */
        QString text = MultiLineEditTextMessage->toPlainText();

        /* Each line is already separated by a newline char,
           so no special processing is further required */

        /* Save the text */
        vecstrTextMessage[iID] = text.toUtf8().constData();

    }
    else
        bTextIsNotEmpty = false;
    return bTextIsNotEmpty;
}

void TransmDialog::OnPushButtonAddText()
{
    if (iIDCurrentText == 0)
    {
        /* Add new message */
        if (GetMessageText(vecstrTextMessage.Size()))
        {
            /* If text was not empty, add new text in combo box */
            const int iNewID = vecstrTextMessage.Size() - 1;
            ComboBoxTextMessage->insertItem(iNewID, QString().setNum(iNewID));
            /* Clear added text */
            MultiLineEditTextMessage->clear();
        }
    }
    else
    {
        /* Text was modified */
        GetMessageText(iIDCurrentText);
    }
}

void TransmDialog::OnButtonClearAllText()
{
    /* Clear container */
    vecstrTextMessage.Init(1);
    iIDCurrentText = 0;

    /* Clear combo box */
    ComboBoxTextMessage->clear();
    ComboBoxTextMessage->insertItem(0, "new");
    /* Clear multi line edit */
    MultiLineEditTextMessage->clear();
}

void TransmDialog::OnToggleCheckBoxRemovePath(bool bState)
{
    tx.SetPathRemoval(bState);
}

void TransmDialog::OnPushButtonAddFileName()
{
    /* Show "open file" dialog. Let the user select more than one file */
    QStringList list = QFileDialog::getOpenFileNames(this, tr("Add Files"), nullptr, tr("Image Files (*.png *.jpg *.jpeg *.jfif)"));

    /* Check if user not hit the cancel button */
    if (!list.isEmpty())
    {
        /* Insert all selected file names */
        for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
        {
            QFileInfo FileInfo((*it));

            /* Insert tree widget item. The objects which is created
               here will be automatically destroyed by QT when
               the parent ("TreeWidgetFileNames") is destroyed */
            QTreeWidgetItem* item = new QTreeWidgetItem();
            if (item)
            {
                item->setText(0, FileInfo.fileName());
                item->setText(1, QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2));
                item->setText(2, FileInfo.filePath());
                TreeWidgetFileNames->addTopLevelItem(item);
            }
        }
        /* Resize columns to content */
        for (int i = 0; i < 3; i++)
            TreeWidgetFileNames->resizeColumnToContents(i);
    }
}

void TransmDialog::OnButtonClearAllFileNames()
{
    /* Remove all items */
    TreeWidgetFileNames->clear();
    /* Resize columns */
    for (int i = 0; i < 3; i++)
        TreeWidgetFileNames->resizeColumnToContents(i);
}

void TransmDialog::OnButtonCodec()
{
    /* Create Codec Dialog if nullptr */
    if(RadioButtonAAC->isChecked()) {
        if (!pAACCodecDlg)
        {
            const int iShortID = 0; // TODO
            CParameter& Parameters = *tx.GetParameters();
            pAACCodecDlg = new AACCodecParams(Settings, Parameters, iShortID, this);
        }
        /* Toggle the visibility */
        if (pAACCodecDlg)
            pAACCodecDlg->Toggle();
    }
    else {
        if (!pOpusCodecDlg)
        {
            const int iShortID = 0; // TODO
            CParameter& Parameters = *tx.GetParameters();
            pOpusCodecDlg = new OpusCodecParams(Settings, Parameters, iShortID, this);
        }
        /* Toggle the visibility */
        if (pOpusCodecDlg)
            pOpusCodecDlg->Toggle();
    }
}

void TransmDialog::OnComboBoxTextMessageActivated(int iID)
{
    iIDCurrentText = iID;

    /* Set text control with selected message */
    MultiLineEditTextMessage->clear();
    if (iID != 0)
    {
        /* Get the text */
        QString text = QString::fromUtf8(vecstrTextMessage[iID].c_str());

        /* Write stored text in multi line edit control */
        MultiLineEditTextMessage->setText(text);
    }
}

// kHz
void TransmDialog::OnTextChangedSndCrdIF(const QString& strIF)
{
    // 将字符串转换为浮点值"toFloat()"
    tx.SetCarrierOffset(strIF.toFloat());
}

void TransmDialog::OnTextChangedServiceID(const QString& strID)
{
    CParameter& Parameters = *tx.GetParameters();

    if(strID.length()<6)
        return;

    /* Convert hex string to unsigned integer "toUInt()" */
    bool ok;
    uint32_t iServiceID = strID.toUInt(&ok, 16);
    if(ok == false)
        return;

    Parameters.Lock();

    Parameters.Service[0].iServiceID = iServiceID;

    Parameters.Unlock();
}

void TransmDialog::OnTextChangedServiceLabel(const QString& strLabel)
{
    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();
    /* Set additional text for log file. */
    Parameters.Service[0].strLabel = strLabel.toUtf8().constData();
    Parameters.Unlock();
}

void TransmDialog::OnComboBoxMSCInterleaverActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();
    switch (iID)
    {
    case 0:
        Parameters.eSymbolInterlMode = CParameter::SI_LONG;
        break;

    case 1:
        Parameters.eSymbolInterlMode = CParameter::SI_SHORT;
        break;
    }
    Parameters.Unlock();
}

void TransmDialog::OnComboBoxMSCConstellationActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();
    switch (iID)
    {
    case 0:
        Parameters.eMSCCodingScheme = CS_2_SM;
        break;

    case 1:
        Parameters.eMSCCodingScheme = CS_3_SM;
        break;

    case 2:
        Parameters.eMSCCodingScheme = CS_3_HMSYM;
        break;

    case 3:
        Parameters.eMSCCodingScheme = CS_3_HMMIX;
        break;
    }
    Parameters.Unlock();

    /* Protection level must be re-adjusted when constelletion mode was
       changed */
    UpdateMSCProtLevCombo();
}

void TransmDialog::OnComboBoxMSCProtLevActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();
    Parameters.Lock();
    Parameters.MSCPrLe.iPartB = iID;
    Parameters.Unlock();
}

void TransmDialog::UpdateMSCProtLevCombo()
{
    CParameter& Parameters = *tx.GetParameters();
    Parameters.Lock();
    if (Parameters.eMSCCodingScheme == CS_2_SM)
    {
        /* Only two protection levels possible in 16 QAM mode */
        ComboBoxMSCProtLev->clear();
        ComboBoxMSCProtLev->insertItem(0, "0");
        ComboBoxMSCProtLev->insertItem(1, "1");
        /* Set protection level to 1 if greater than 1 */
        if (Parameters.MSCPrLe.iPartB > 1)
            Parameters.MSCPrLe.iPartB = 1;
    }
    else
    {
        /* Four protection levels defined */
        ComboBoxMSCProtLev->clear();
        ComboBoxMSCProtLev->insertItem(0, "0");
        ComboBoxMSCProtLev->insertItem(1, "1");
        ComboBoxMSCProtLev->insertItem(2, "2");
        ComboBoxMSCProtLev->insertItem(3, "3");
    }
    Parameters.Unlock();
    ComboBoxMSCProtLev->setCurrentIndex(Parameters.MSCPrLe.iPartB);
}

void TransmDialog::OnComboBoxSDCConstellationActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();
    Parameters.Lock();
    switch (iID)
    {
    case 0:
        Parameters.eSDCCodingScheme = CS_1_SM;
        break;

    case 1:
        Parameters.eSDCCodingScheme = CS_2_SM;
        break;
    }
    Parameters.Unlock();
}

void TransmDialog::OnComboBoxLanguageActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();
    Parameters.Lock();
    Parameters.Service[0].iLanguage = iID;
    Parameters.Unlock();
}

void TransmDialog::OnComboBoxProgramTypeActivated(int iID)
{
    CParameter& Parameters = *tx.GetParameters();
    Parameters.Lock();
    Parameters.Service[0].iServiceDescr = iID;
    Parameters.Unlock();
    iServiceDescr = iID;
}

void TransmDialog::OnRadioRobustnessMode(int iID)
{
    // qDebug()<<"选择模式是:"<<iID <<endl;  // Amode = -2 ;
    // qDebug()<<"选择模式是:"<<iID <<endl;

    iID = -iID - 2; // TODO understand why
    CParameter& Parameters = *tx.GetParameters();

    /* Set current bandwidth */
    Parameters.Lock();
    ESpecOcc eCurSpecOcc = Parameters.GetSpectrumOccup();
    Parameters.Unlock();

    /* Set default new robustness mode */
    ERobMode eNewRobMode = RM_ROBUSTNESS_MODE_B;

    /* Check, which bandwith's are possible with this robustness mode */
    switch (iID)
    {
    case 0:
        /* All bandwidth modes are possible */
        RadioButtonBandwidth45->setEnabled(true);
        RadioButtonBandwidth5->setEnabled(true);
        RadioButtonBandwidth9->setEnabled(true);
        RadioButtonBandwidth18->setEnabled(true);
        /* Set new robustness mode */
        eNewRobMode = RM_ROBUSTNESS_MODE_A;
        break;

    case 1:
        /* All bandwidth modes are possible */
        RadioButtonBandwidth45->setEnabled(true);
        RadioButtonBandwidth5->setEnabled(true);
        RadioButtonBandwidth9->setEnabled(true);
        RadioButtonBandwidth18->setEnabled(true);
        /* Set new robustness mode */
        eNewRobMode = RM_ROBUSTNESS_MODE_B;
        break;

    case 2:
        /* Only 10 and 20 kHz possible in robustness mode C */
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        /* Set check on the nearest bandwidth if "out of range" */
        if (eCurSpecOcc == SO_0 || eCurSpecOcc == SO_1 ||
                eCurSpecOcc == SO_2 || eCurSpecOcc == SO_4)
        {
            if (eCurSpecOcc == SO_4)
                RadioButtonBandwidth20->setChecked(true);
            else
                RadioButtonBandwidth10->setChecked(true);
            OnRadioBandwidth(ButtonGroupBandwidth->checkedId());  // SetSpectrumOccup(eNewSpecOcc);
        }
        /* Set new robustness mode */
        eNewRobMode = RM_ROBUSTNESS_MODE_C;
        break;

    case 3:
        /* Only 10 and 20 kHz possible in robustness mode D */
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        /* Set check on the nearest bandwidth if "out of range" */
        if (eCurSpecOcc == SO_0 || eCurSpecOcc == SO_1 ||
                eCurSpecOcc == SO_2 || eCurSpecOcc == SO_4)
        {
            if (eCurSpecOcc == SO_4)
                RadioButtonBandwidth20->setChecked(true);
            else
                RadioButtonBandwidth10->setChecked(true);
            OnRadioBandwidth(ButtonGroupBandwidth->checkedId());
        }
        /* Set new robustness mode */
        eNewRobMode = RM_ROBUSTNESS_MODE_D;
        break;
    }

    /* Set new robustness mode */
    Parameters.Lock();
    Parameters.SetWaveMode(eNewRobMode);
    Parameters.Unlock();
}

void TransmDialog::OnRadioBandwidth(int iID)
{
    iID = -iID - 2; // TODO understand why
    static const int table[6] = {0, 2, 4, 1, 3, 5};
    iID = table[(unsigned int)iID % (unsigned int)6];
    ESpecOcc eNewSpecOcc = SO_0;

    switch (iID)
    {
    case 0:
        eNewSpecOcc = SO_0;
        break;

    case 1:
        eNewSpecOcc = SO_1;
        break;

    case 2:
        eNewSpecOcc = SO_2;
        break;

    case 3:
        eNewSpecOcc = SO_3;
        break;

    case 4:
        eNewSpecOcc = SO_4;
        break;

    case 5:
        eNewSpecOcc = SO_5;
        break;
    }

    CParameter& Parameters = *tx.GetParameters();

    /* Set new spectrum occupancy */
    Parameters.Lock();
    Parameters.SetSpectrumOccup(eNewSpecOcc);
    Parameters.Unlock();
}

void TransmDialog::OnRadioOutput(int iID)
{
    iID = -iID - 2; // TODO understand why
    switch (iID)
    {
    case 0:
        /* Button "Real Valued" */
        tx.SetIQOutput(CTransmitData::OF_REAL_VAL);
        CheckBoxAmplifiedOutput->setEnabled(true);
        CheckBoxHighQualityIQ->setEnabled(false);
        break;

    case 1:
        /* Button "I / Q (pos)" */
        tx.SetIQOutput(CTransmitData::OF_IQ_POS);
        CheckBoxAmplifiedOutput->setEnabled(true);
        CheckBoxHighQualityIQ->setEnabled(true);
        break;

    case 2:
        /* Button "I / Q (neg)" */
        tx.SetIQOutput(CTransmitData::OF_IQ_NEG);
        CheckBoxAmplifiedOutput->setEnabled(true);
        CheckBoxHighQualityIQ->setEnabled(true);
        break;

    case 3:
        /* Button "E / P" */
        tx.SetIQOutput(CTransmitData::OF_EP);
        CheckBoxAmplifiedOutput->setEnabled(false);
        CheckBoxHighQualityIQ->setEnabled(true);
        break;
    }
}

void TransmDialog::OnRadioCurrentTime(int iID)
{
    iID = -iID - 2; // TODO understand why
    CParameter::ECurTime eCurTime = CParameter::CT_OFF;

    switch (iID)
    {
    case 0:
        eCurTime = CParameter::CT_OFF;
        break;

    case 1:
        eCurTime = CParameter::CT_LOCAL;
        break;

    case 2:
        eCurTime = CParameter::CT_UTC;
        break;

    case 3:
        eCurTime = CParameter::CT_UTC_OFFSET;
        break;
    }

    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();

    /* Set transmission of current time in global struct */
    Parameters.eTransmitCurrentTime = eCurTime;

    Parameters.Unlock();
}

void TransmDialog::OnRadioCodec(int iID)
{
    iID = -iID - 2; // TODO understand why
    CAudioParam::EAudCod eNewAudioCoding = CAudioParam::AC_AAC;

    switch (iID)
    {
    case 0:
        eNewAudioCoding = CAudioParam::AC_AAC;
        ShowButtonCodec(false, 1);
        break;

    case 1:
        eNewAudioCoding = CAudioParam::AC_OPUS;
        ShowButtonCodec(true, 1);
        break;
    }

    CParameter& Parameters = *tx.GetParameters();

    Parameters.Lock();

    /* Set new audio codec */
    Parameters.Service[0].AudioParam.eAudioCoding = eNewAudioCoding;

    Parameters.Unlock();
}

void TransmDialog::DisableAllControlsForSet()
{
    TabWidgetEnableTabs(TabWidgetParam, false);
    TabWidgetEnableTabs(TabWidgetServices, false);

    GroupInput->setEnabled(true); /* For run-mode */
}

void TransmDialog::EnableAllControlsForSet()
{
    TabWidgetEnableTabs(TabWidgetParam, true);
    TabWidgetEnableTabs(TabWidgetServices, true);

    GroupInput->setEnabled(false); /* For run-mode */

    /* Reset status bars */
    ProgrInputLevel->setValue(RET_VAL_LOG_0);
    ProgressBarCurPict->setValue(0);
    TextLabelCurPict->setText("");
}

void TransmDialog::TabWidgetEnableTabs(QTabWidget *tabWidget, bool enable)
{
    for (int i = 0; i < tabWidget->count(); i++)
        tabWidget->widget(i)->setEnabled(enable);
}

void TransmDialog::ShowButtonCodec(bool bShow, int iKey)
{
    bool bLastShow = iButtonCodecState == 0;
    if (bShow)
        iButtonCodecState &= ~iKey;
    else
        iButtonCodecState |= iKey;
    bShow = iButtonCodecState == 0;
    if (bShow != bLastShow)
    {
        if (bShow)
            ButtonCodec->show();
        else
            ButtonCodec->hide();
        if (pAACCodecDlg)
            pAACCodecDlg->Show(bShow);
    }
}

void TransmDialog::AddWhatsThisHelp()
{
    /* Dream Logo */
    const QString strPixmapLabelDreamLogo =
            tr("<b>Dream Logo:</b> This is the official logo of "
               "the Dream software.");

    //PixmapLabelDreamLogo->setWhatsThis(strPixmapLabelDreamLogo);

    /* Input Level */
    const QString strInputLevel =
            tr("<b>Input Level:</b> The input level meter shows "
               "the relative input signal peak level in dB. If the level is too high, "
               "the meter turns from green to red.");

    TextLabelAudioLevel->setWhatsThis(strInputLevel);
    ProgrInputLevel->setWhatsThis(strInputLevel);

    /* Progress Bar */
    const QString strProgressBar =
            tr("<b>Progress Bar:</b> The progress bar shows "
               "the advancement of transmission of current file. "
               "Only meaningful when 'Data (SlideShow Application)' "
               "mode is enabled.");

    ProgressBarCurPict->setWhatsThis(strProgressBar);

    /* DRM Robustness Mode */
    const QString strRobustnessMode =
            tr("<b>DRM Robustness Mode:</b> In a DRM system, "
               "four possible robustness modes are defined to adapt the system to "
               "different channel conditions. According to the DRM standard:"
               "<ul><li><i>Mode A:</i> Gaussian channels, with "
               "minor fading</li><li><i>Mode B:</i> Time "
               "and frequency selective channels, with longer delay spread"
               "</li><li><i>Mode C:</i> As robustness mode B, "
               "but with higher Doppler spread</li><li><i>Mode D:"
               "</i> As robustness mode B, but with severe delay and "
               "Doppler spread</li></ul>");

    GroupBoxRobustnessMode->setWhatsThis(strRobustnessMode);
    RadioButtonRMA->setWhatsThis(strRobustnessMode);
    RadioButtonRMB->setWhatsThis(strRobustnessMode);
    RadioButtonRMC->setWhatsThis(strRobustnessMode);
    RadioButtonRMD->setWhatsThis(strRobustnessMode);

    /* Bandwidth */
    const QString strBandwidth =
            tr("<b>DRM Bandwidth:</b> The bandwith is the gross "
               "bandwidth of the generated DRM signal. Not all DRM robustness mode / "
               "bandwidth constellations are possible, e.g., DRM robustness mode D "
               "and C are only defined for the bandwidths 10 kHz and 20 kHz.");

    GroupBoxBandwidth->setWhatsThis(strBandwidth);
    RadioButtonBandwidth45->setWhatsThis(strBandwidth);
    RadioButtonBandwidth5->setWhatsThis(strBandwidth);
    RadioButtonBandwidth9->setWhatsThis(strBandwidth);
    RadioButtonBandwidth10->setWhatsThis(strBandwidth);
    RadioButtonBandwidth18->setWhatsThis(strBandwidth);
    RadioButtonBandwidth20->setWhatsThis(strBandwidth);

    /* TODO: ComboBoxMSCConstellation, ComboBoxMSCProtLev,
             ComboBoxSDCConstellation */

    /* MSC interleaver mode */
    const QString strInterleaver =
            tr("<b>MSC interleaver mode:</b> The symbol "
               "interleaver depth can be either short (approx. 400 ms) or long "
               "(approx. 2 s). The longer the interleaver the better the channel "
               "decoder can correct errors from slow fading signals. But the longer "
               "the interleaver length the longer the delay until (after a "
               "re-synchronization) audio can be heard.");

    TextLabelInterleaver->setWhatsThis(strInterleaver);
    ComboBoxMSCInterleaver->setWhatsThis(strInterleaver);

    /* Output intermediate frequency of DRM signal */
    const QString strOutputIF =
            tr("<b>Output intermediate frequency of DRM signal:</b> "
               "Set the output intermediate frequency (IF) of generated DRM signal "
               "in the 'sound-card pass-band'. In some DRM modes, the IF is located "
               "at the edge of the DRM signal, in other modes it is centered. The IF "
               "should be chosen that the DRM signal lies entirely inside the "
               "sound-card bandwidth.");

    ButtonGroupIF->setWhatsThis(strOutputIF);
    LineEditSndCrdIF->setWhatsThis(strOutputIF);
    TextLabelIFUnit->setWhatsThis(strOutputIF);

    /* Output format */
    const QString strOutputFormat =
            tr("<b>Output format:</b> Since the sound-card "
               "outputs signals in stereo format, it is possible to output the DRM "
               "signal in three formats:<ul><li><b>real valued"
               "</b> output on both, left and right, sound-card "
               "channels</li><li><b>I / Q</b> output "
               "which is the in-phase and quadrature component of the complex "
               "base-band signal at the desired IF. In-phase is output on the "
               "left channel and quadrature on the right channel."
               "</li><li><b>E / P</b> output which is the "
               "envelope and phase on separate channels. This output type cannot "
               "be used if the Dream transmitter is regularly compiled with a "
               "sound-card sample rate of 48 kHz since the spectrum of these "
               "components exceed the bandwidth of 20 kHz.<br>The envelope signal "
               "is output on the left channel and the phase is output on the right "
               "channel.</li></ul>");

    GroupBoxOutput->setWhatsThis(strOutputFormat);
    RadioButtonOutReal->setWhatsThis(strOutputFormat);
    RadioButtonOutIQPos->setWhatsThis(strOutputFormat);
    RadioButtonOutIQNeg->setWhatsThis(strOutputFormat);
    RadioButtonOutEP->setWhatsThis(strOutputFormat);

    /* Current Time Transmission */
    const QString strCurrentTime =
            tr("<b>Current Time Transmission:</b> The current time is transmitted, "
               "four possible modes are defined:"
               "<ul><li><i>Off:</i> No time information is transmitted</li>"
               "<li><i>Local:</i> The local time is transmitted</li>"
               "<li><i>UTC:</i> The Coordinated Universal Time is transmitted</li>"
               "<li><i>UTC+Offset:</i> Same as UTC but with the addition of an offset "
               "in hours from local time</li></ul>");

    GroupBoxCurrentTime->setWhatsThis(strCurrentTime);
    RadioButtonCurTimeOff->setWhatsThis(strCurrentTime);
    RadioButtonCurTimeLocal->setWhatsThis(strCurrentTime);
    RadioButtonCurTimeUTC->setWhatsThis(strCurrentTime);
    RadioButtonCurTimeUTCOffset->setWhatsThis(strCurrentTime);

    /* TODO: Services... */

    /* Data (SlideShow Application) */

    /* Remove path from filename */
    const QString strRemovePath =
            tr("<b>Remove path from filename:</b> Un-checking this box will "
               "transmit the full path of the image location on your computer. "
               "This might not be what you want.");

    CheckBoxRemovePath->setWhatsThis(strRemovePath);
}

void TransmDialog::Onswitch_sourse()
{
    DEBUG_COUT("数据源");
    stackedWidget->setCurrentWidget(page);
    pushButton_reuse->setEnabled(true);
    pushButton_source->setEnabled(false);
    pushButton_modulation->setEnabled(true);

}

void TransmDialog::Onswitch_reuse()
{
    DEBUG_COUT("复用");
    stackedWidget->setCurrentWidget(page_reuse);
    pushButton_reuse->setEnabled(false);
    pushButton_source->setEnabled(true);
    pushButton_modulation->setEnabled(true);

}

void TransmDialog::Onswitch_modulation()
{
    DEBUG_COUT("解调");
    stackedWidget->setCurrentWidget(page_modulation);
    pushButton_reuse->setEnabled(true);
    pushButton_source->setEnabled(true);
    pushButton_modulation->setEnabled(false);

}



// 从接收机中参数中 拿出数据 CRx &Rx :  ReceiverManager receiverManager
void TransmDialog::onParametersReceived()
{

    if(this->pReceiverManager == nullptr)
    {
        cout<<"Warning: pReceiverManager null"<<endl;
        return;
    }

    ReceiverManager& receiverManager = *this->pReceiverManager;
    // 向下转型
    CSubDRMReceiver* subrecv = receiverManager.getSubReceiver(1);
    if (subrecv == nullptr)
    {
        cout<<"subrecv == nullptr error."<< endl;
        return;
    }
    CParameter& Parameters = *subrecv->GetParameters();

#if 0 //监测界面 放在了 my定时器(Timer_500ms)中 所以注释掉了



    //DRM mode
    QString FACInfo;
    QString DRM_mode;
    DRM_mode = GetRobmodestr(Parameters);
    this->FACDRMModeBWV->setText(DRM_mode);
    //带宽
    QString SpectrumOccup;
    SpectrumOccup = GetSpectrumOccup(Parameters);
    this->FACDRMWV_2->setText(SpectrumOccup);
    //交织深度
    QString Interl_Mode;
    Interl_Mode = GetInterl(Parameters);
    this->FACInterleaverDepthV->setText(Interl_Mode);

    //SDC mode  MSC mode
    QString SDC_mode;
    QString MSC_mode;
    SDC_mode = GetSDCmode(Parameters);
    this->FACSDCMSCModeV_2->setText(SDC_mode);
    MSC_mode = GetMSCmode(Parameters);
    this->FACSDCMSCModeV_3->setText(MSC_mode);

    //保护等级
    QString MSCPrLe;
    MSCPrLe = QString().setNum(Parameters.MSCPrLe.iPartA);
    MSCPrLe += " / ";
    MSCPrLe += QString().setNum(Parameters.MSCPrLe.iPartB);
    this->FACCodeRateV->setText(MSCPrLe);

    //服务个数
    QString AudioService_DataService;
    AudioService_DataService = tr("Audio:");
    AudioService_DataService += QString().setNum(Parameters.iNumAudioService);
    AudioService_DataService += tr("/ Data:");
    AudioService_DataService += QString().setNum(Parameters.iNumDataService);
    this->FACNumServicesV->setText(AudioService_DataService);

    //更新 服务ID



    // 刷新新的信息源 ipList

    int i = 0;
    ReceiverManager* pReceiverManager = tx.getTx_manager();
    if(pReceiverManager == nullptr)
    {
        cerr<< "pReceiverManager == nullptr "<<endl;
        return;
    }


    std::vector<CSubDRMReceiver*> vecSubDRMReceiver = pReceiverManager->LoopReceivers();
    for(auto subRx : vecSubDRMReceiver)
    {

        //子接收机
        std::string serverIP = "消息源 " + std::to_string(i+1) + "：" + subRx->sourceIP
                +"："+ std::to_string(subRx->localPort);
        modifyListViewItem(i, QString::fromStdString(serverIP));
        //设置服务   serveer 解码 lable
        for(int j =0 ; j< 4;j++)
        {
            //  Parameters.Service[i]; 根据参数中 是否活跃
            QString serverlabel = Getserver0_4(*subRx ,*subRx->GetParameters(), j);
            /*****没有解码出来
            serverlabel = 空
            serverlabel = 空
            serverlabel = 空
            serverlabel = 空
            */
            cout<<"serverlabel = "<< serverlabel.toStdString()<<endl;
            this->setServer0_4( j, serverlabel);
        }
        i++;
    }

#endif   //监测界面 放在了 my定时器(Timer_500ms)中 所以注释掉了

    //调制机 的自动选择
#if 1
    /* Robustness mode */
    switch (Parameters.GetWaveMode())
    {
    case RM_ROBUSTNESS_MODE_A:
        RadioButtonRMA->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_B:
        RadioButtonRMB->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_C:
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        RadioButtonRMC->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_D:
        RadioButtonBandwidth45->setEnabled(false);
        RadioButtonBandwidth5->setEnabled(false);
        RadioButtonBandwidth9->setEnabled(false);
        RadioButtonBandwidth18->setEnabled(false);
        RadioButtonRMD->setChecked(true);
        break;

    case RM_ROBUSTNESS_MODE_E:
        break;

    case RM_NO_MODE_DETECTED:
        break;
    }

    /* Bandwidth */
    switch (Parameters.GetSpectrumOccup())
    {
    case SO_0:
        RadioButtonBandwidth45->setChecked(true);
        break;

    case SO_1:
        RadioButtonBandwidth5->setChecked(true);
        break;

    case SO_2:
        RadioButtonBandwidth9->setChecked(true);
        break;

    case SO_3:
        RadioButtonBandwidth10->setChecked(true);
        break;

    case SO_4:
        RadioButtonBandwidth18->setChecked(true);
        break;

    case SO_5:
        RadioButtonBandwidth20->setChecked(true);
        break;
    }

    /* MSC interleaver mode 长交织 */

    switch (Parameters.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        ComboBoxMSCInterleaver->setCurrentIndex(0);
        break;

    case CParameter::SI_SHORT:
        ComboBoxMSCInterleaver->setCurrentIndex(1);
        break;
    }

    switch (Parameters.eMSCCodingScheme)
    {
    case CS_1_SM:
        break;

    case CS_2_SM:
        ComboBoxMSCConstellation->setCurrentIndex(0);
        break;

    case CS_3_SM:
        ComboBoxMSCConstellation->setCurrentIndex(1);
        break;

    case CS_3_HMSYM:
        ComboBoxMSCConstellation->setCurrentIndex(2);
        break;

    case CS_3_HMMIX:
        ComboBoxMSCConstellation->setCurrentIndex(3);
        break;
    }

    /* SDC Constellation Scheme */

    switch (Parameters.eSDCCodingScheme)
    {
    case CS_1_SM:
        ComboBoxSDCConstellation->setCurrentIndex(0);
        break;

    case CS_2_SM:
        ComboBoxSDCConstellation->setCurrentIndex(1);
        break;

    case CS_3_SM:
    case CS_3_HMSYM:
    case CS_3_HMMIX:
        break;
    }

    /* Set new robustness mode */
    OnRadioRobustnessMode(-Parameters.GetWaveMode()- 2);
    //Parameters.SetWaveMode(Parameters.GetWaveMode());

    /* Output High Quality I/Q */
    CheckBoxHighQualityIQ->setEnabled(false);//源码 tx.GetIQOutput() != CTransmitData::OF_REAL_VAL
    CheckBoxHighQualityIQ->setChecked(false);//源码 tx.isHighQualityIQ()
#endif

}


//如果没有点击启动(tx)按钮, 刷新监测界面显示,并且选择发射机模式.
void TransmDialog::onRefresh_Tx_Ui()
{
    if(!bIsStarted)
    {

        if(this->pReceiverManager == nullptr)
        {
            cout<<"Warning: pReceiverManager null"<<endl;
            return;
        }

        ReceiverManager& receiverManager = *this->pReceiverManager;
        // 向下转型
        CSubDRMReceiver* subrecv = receiverManager.getSubReceiver(1);
        if (subrecv == nullptr)
        {
            cout<<"subrecv == nullptr error."<< endl;
            return;
        }
        CParameter& Parameters = *subrecv->GetParameters();



        //DRM mode
        QString FACInfo;
        QString DRM_mode;
        DRM_mode = GetRobmodestr(Parameters);
        this->FACDRMModeBWV->setText(DRM_mode);
        //带宽
        QString SpectrumOccup;
        SpectrumOccup = GetSpectrumOccup(Parameters);
        this->FACDRMWV_2->setText(SpectrumOccup);
        //交织深度
        QString Interl_Mode;
        Interl_Mode = GetInterl(Parameters);
        this->FACInterleaverDepthV->setText(Interl_Mode);

        //SDC mode  MSC mode
        QString SDC_mode;
        QString MSC_mode;
        SDC_mode = GetSDCmode(Parameters);
        this->FACSDCMSCModeV_2->setText(SDC_mode);
        MSC_mode = GetMSCmode(Parameters);
        this->FACSDCMSCModeV_3->setText(MSC_mode);

        //保护等级
        QString MSCPrLe;
        MSCPrLe = QString().setNum(Parameters.MSCPrLe.iPartA);
        MSCPrLe += " / ";
        MSCPrLe += QString().setNum(Parameters.MSCPrLe.iPartB);
        this->FACCodeRateV->setText(MSCPrLe);

        //服务个数
        QString AudioService_DataService;
        AudioService_DataService = tr("Audio:");
        AudioService_DataService += QString().setNum(Parameters.iNumAudioService);
        AudioService_DataService += tr("/ Data:");
        AudioService_DataService += QString().setNum(Parameters.iNumDataService);
        this->FACNumServicesV->setText(AudioService_DataService);

        //更新 服务ID
        // 刷新新的信息源 ipList

        int i = 0;
        ReceiverManager* pReceiverManager = tx.getTx_manager();
        if(pReceiverManager == nullptr)
        {
            cerr<< "pReceiverManager == nullptr "<<endl;
            return;
        }


        std::vector<CSubDRMReceiver*> vecSubDRMReceiver =
                pReceiverManager->LoopReceivers();
        for(auto subRx : vecSubDRMReceiver)
        {

            //子接收机
            std::string serverIP = "消息源 " + std::to_string(i+1) + "：" + subRx->sourceIP
                    +"："+ std::to_string(subRx->localPort);
            modifyListViewItem(i, QString::fromStdString(serverIP));
            //设置服务   serveer 解码 lable
            for(int j =0 ; j< 4;j++)
            {
                //  Parameters.Service[i]; 根据参数中 是否活跃
                QString serverlabel = Getserver0_4(*subRx ,*subRx->GetParameters(), j);

                //设置 服务
                this->setServer0_4( j, serverlabel);
            }
            i++;
        }
        //调制机 的自动选择
#if 1
        /* Robustness mode */
        switch (Parameters.GetWaveMode())
        {
        case RM_ROBUSTNESS_MODE_A:
            RadioButtonRMA->setChecked(true);
            break;

        case RM_ROBUSTNESS_MODE_B:
            RadioButtonRMB->setChecked(true);
            break;

        case RM_ROBUSTNESS_MODE_C:
            RadioButtonBandwidth45->setEnabled(false);
            RadioButtonBandwidth5->setEnabled(false);
            RadioButtonBandwidth9->setEnabled(false);
            RadioButtonBandwidth18->setEnabled(false);
            RadioButtonRMC->setChecked(true);
            break;

        case RM_ROBUSTNESS_MODE_D:
            RadioButtonBandwidth45->setEnabled(false);
            RadioButtonBandwidth5->setEnabled(false);
            RadioButtonBandwidth9->setEnabled(false);
            RadioButtonBandwidth18->setEnabled(false);
            RadioButtonRMD->setChecked(true);
            break;

        case RM_ROBUSTNESS_MODE_E:
            break;

        case RM_NO_MODE_DETECTED:
            break;
        }

        /* Bandwidth */
        switch (Parameters.GetSpectrumOccup())
        {
        case SO_0:
            RadioButtonBandwidth45->setChecked(true);
            break;

        case SO_1:
            RadioButtonBandwidth5->setChecked(true);
            break;

        case SO_2:
            RadioButtonBandwidth9->setChecked(true);
            break;

        case SO_3:
            RadioButtonBandwidth10->setChecked(true);
            break;

        case SO_4:
            RadioButtonBandwidth18->setChecked(true);
            break;

        case SO_5:
            RadioButtonBandwidth20->setChecked(true);
            break;
        }

        /* MSC interleaver mode 长交织 */

        switch (Parameters.eSymbolInterlMode)
        {
        case CParameter::SI_LONG:
            ComboBoxMSCInterleaver->setCurrentIndex(0);
            break;

        case CParameter::SI_SHORT:
            ComboBoxMSCInterleaver->setCurrentIndex(1);
            break;
        }

        switch (Parameters.eMSCCodingScheme)
        {
        case CS_1_SM:
            break;

        case CS_2_SM:
            ComboBoxMSCConstellation->setCurrentIndex(0);
            break;

        case CS_3_SM:
            ComboBoxMSCConstellation->setCurrentIndex(1);
            break;

        case CS_3_HMSYM:
            ComboBoxMSCConstellation->setCurrentIndex(2);
            break;

        case CS_3_HMMIX:
            ComboBoxMSCConstellation->setCurrentIndex(3);
            break;
        }

        /* SDC Constellation Scheme */

        switch (Parameters.eSDCCodingScheme)
        {
        case CS_1_SM:
            ComboBoxSDCConstellation->setCurrentIndex(0);
            break;

        case CS_2_SM:
            ComboBoxSDCConstellation->setCurrentIndex(1);
            break;

        case CS_3_SM:
        case CS_3_HMSYM:
        case CS_3_HMMIX:
            break;
        }

        /* Set new robustness mode */
        OnRadioRobustnessMode(-Parameters.GetWaveMode()- 2);
        //Parameters.SetWaveMode(Parameters.GetWaveMode());

        /* Output High Quality I/Q */
        CheckBoxHighQualityIQ->setEnabled(false);//源码 tx.GetIQOutput() != CTransmitData::OF_REAL_VAL
        CheckBoxHighQualityIQ->setChecked(false);//源码 tx.isHighQualityIQ()
#endif
    }
}



QString GetRobmodestr(CParameter& Parameters)
{
    switch (Parameters.GetWaveMode())
    {
    case RM_ROBUSTNESS_MODE_A:
        return "A";
        break;

    case RM_ROBUSTNESS_MODE_B:
        return "B";
        break;

    case RM_ROBUSTNESS_MODE_C:
        return "C";
        break;

    case RM_ROBUSTNESS_MODE_D:
        return "D";
        break;

    default:
        return "?";
    }
}

QString GetSpectrumOccup(CParameter &Parameters)
{
    switch (Parameters.GetSpectrumOccup())
    {
    case SO_0:
        return "4.5 kHz";
        break;

    case SO_1:
        return "5 kHz";
        break;

    case SO_2:
        return "9 kHz";
        break;

    case SO_3:
        return "10 kHz";
        break;

    case SO_4:
        return "18 kHz";
        break;

    case SO_5:
        return "20 kHz";
        break;

    default:
        return "?";
    }
}

QString GetInterl(CParameter &Parameters)
{
    switch (Parameters.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        return "2 s (长交织)" ;  //Long Interleaving  短
        break;

    case CParameter::SI_SHORT:
        return"400 ms (短交织)" ;   //Short Interleaving
        break;

    default:
        return "?";
    }
}

QString GetSDCmode(CParameter &Parameters)
{
    /* SDC */
    switch (Parameters.eSDCCodingScheme)
    {
    case CS_1_SM:
        return  "4-QAM ";
        //         strFACInfo = "4-QAM / ";
        break;

    case CS_2_SM:
        return "16-QAM ";
        //         strFACInfo = "16-QAM / ";
        break;

    default:
        return "? / ";
    }
}

QString GetMSCmode(CParameter &Parameters)
{
    /* MSC */
    switch (Parameters.eMSCCodingScheme)
    {
    case CS_2_SM:
        //        strFACInfo += "SM 16-QAM";
        return "SM 16-QAM";
        break;

    case CS_3_SM:
        //        strFACInfo += "SM 64-QAM";
        return"SM 64-QAM";
        break;

    case CS_3_HMSYM:
        //        strFACInfo += "HMsym 64-QAM";
        return  "HMsym 64-QAM";
        break;

    case CS_3_HMMIX:
        //        strFACInfo += "HMmix 64-QAM";
        return "HMmix 64-QAM";
        break;

    default:
        return "?";
    }
}

QString TransmDialog::Getserver0_4(CDRMReceiver& Rx, CParameter &Parameters,int index)
{
    Parameters.Lock();
    int i = index;
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
        QString strCodec = GetCodecstring(service);
        QString strType = GetTypestring(service);
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
            /* Report missing codec */
            if (!Rx.GetAudSorceDec()->CanDecode(
                        CAudioParam::EAudCod( service.AudioParam.eAudioCoding)))
                text += tr(" [@ no codec available]");

            /* Show, if a multimedia stream is connected to this service */
            if (service.DataParam.iStreamID != STREAM_ID_NOT_USED)
            {
                if (service.DataParam.iUserAppIdent == DAB_AT_EPG)
                    text += tr(" + EPG"); /* EPG service */
                else
                {
                    text += tr(" + MM"); /* other multimedia service */
                    /* Set multimedia service bit */

                    //TODO  2个 先注释掉
                    // iMultimediaServiceBit |= 1 << i;
                }

                /* Bit-rate of connected data stream */
                text += " (" + QString().setNum(rDataBitRate, 'f', 2) + " kbps)";
            }
        }
        /* Data service */
        else
        {
#if 1
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
                        //TODO  2个  先注释掉
                        //iMultimediaServiceBit |= 1 << i;
                        // cout<<"iMultimediaServiceBit |= 1 << i"<< endl;
                        break;
                    }
                }
            }
#endif
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
    else
    {
        text = tr("空");
    }
    return text;
}

void TransmDialog::updateComboBoxFromListView()
{
    //服务 1 ,服务 2 的刷新
    ComboBoxSelectiveSource1->clear();
    ComboBoxSelectiveSource2->clear();
    ComboBoxSelectiveSource3->clear();
    ComboBoxSelectiveSource4->clear();
    // 获取listView的模型
    QStringList listItems = model->stringList();

    // 将每一项添加到ComboBox1 中
    for (const QString &item : listItems)
    {
        ComboBoxSelectiveSource1->addItem(item);
    }
    // 将每一项添加到ComboBox2 中
    for (const QString &item : listItems)
    {
        ComboBoxSelectiveSource2->addItem(item);
    }
    // 将每一项添加到ComboBox3 中
    for (const QString &item : listItems)
    {
        ComboBoxSelectiveSource3->addItem(item);
    }
    // 将每一项添加到ComboBox4 中
    for (const QString &item : listItems)
    {
        ComboBoxSelectiveSource4->addItem(item);
    }

    //只有第一项
    //    if (!listItems.isEmpty())
    //    {
    //        ComboBoxSelectiveSource2->addItem(listItems.first());
    //    }

}

void TransmDialog::modifyListViewItem(int index, const QString &newText)
{
    if (index >= 0 && index <  model->rowCount())
    {
        // 获取当前的字符串列表
        QStringList listItems = model->stringList();

        // 修改指定索引的项
        listItems[index] = newText;

        // 更新模型
        model->setStringList(listItems);

        // 更新ComboBox
        updateComboBoxFromListView();
    }
}

void TransmDialog::setServer0_4(int index, QString serverlabel)
{
    switch (index)
    {
    case 0:
        this->TextMiniService1->setText(serverlabel);
        break;
    case 1:
        this->TextMiniService2->setText(serverlabel);
        break;
    case 2:
        this->TextMiniService3->setText(serverlabel);
        break;
    case 3:
        this->TextMiniService4->setText(serverlabel);
        break;
    default :
        break;
    }

}

QString GetCodecstring(const CService & service)
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

QString GetTypestring(const CService & service )
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
                strReturn = "MONO";
                break;

            case CAudioParam::OC_STEREO:
                strReturn = "STEREO";
                break;
            }
            break;

        default:
            /* Mono-Stereo */
            switch (service.AudioParam.eAudioMode)
            {
            case CAudioParam::AM_MONO:
                strReturn = "Mono";
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
                    strReturn =  "Dynamic labels" ;
                    break;

                case DAB_AT_MOTSLIDESHOW:
                    strReturn = "MOT Slideshow" ;
                    break;

                case DAB_AT_BROADCASTWEBSITE:
                    strReturn =  "MOT WebSite" ;
                    break;

                case DAB_AT_TPEG:
                    strReturn =  "TPEG" ;
                    break;

                case DAB_AT_DGPS:
                    strReturn = "DGPS" ;
                    break;

                case DAB_AT_TMC:
                    strReturn =  "TMC" ;
                    break;

                case DAB_AT_EPG:
                    strReturn = "EPG - Electronic Programme Guide" ;
                    break;

                case DAB_AT_JAVA:
                    strReturn = "Java" ;
                    break;

                case DAB_AT_JOURNALINE: /* Journaline */
                    strReturn =  "Journaline" ;
                    break;
                }
            }
            else
                strReturn =  "Unknown Service" ;
        }
        else
            strReturn =  "Unknown Service" ;

    }

    return strReturn;
}


//1. num  2. temp buff 3. 指定了要转换的数字的基数（进制）
char* my_itoa(int value, char* str, int base)
{
    char *rc = str;
    char *ptr;
    char *low;

    // Set '-' for negative decimals.
    if (base == 10 && value < 0) {
        *str++ = '-';
        value = -value;
    }

    // Remember where the numbers start.
    ptr = str;

    // The actual conversion.
    do {
        *ptr++ = "0123456789abcdef"[value % base];
        value /= base;
    } while (value);

    // Terminate the string.
    *ptr-- = '\0';

    // Reverse the string.
    for (low = str; low < ptr; low++, ptr--) {
        char tmp = *low;
        *low = *ptr;
        *ptr = tmp;
    }

    return rc;


}
