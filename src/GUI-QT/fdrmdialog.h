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

#ifndef _FDRMDIALOG_H_
#define _FDRMDIALOG_H_

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QString>
#include <QMenuBar>
#include <QLayout>
#include <QPalette>
#include <QColorDialog>
#include <QActionGroup>
#include <QSignalMapper>
#include <QDialog>
#include <QMenu>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <qwt_thermo.h>
#include "ui_DRMMainWindow.h"

#include "CWindow.h"
#include "EvaluationDlg.h"
#include "SoundCardSelMenu.h"
#include "DialogUtil.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "EPGDlg.h"
#include "fmdialog.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "GeneralSettingsDlg.h"
#include "MultColorLED.h"
#include "Logging.h"
#include "../main-Qt/crx.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"
#include "../datadecoding/DataDecoder.h"


#include "DRMPlot.h"

#include "../Soundbar/soundbar.h"
//#include "../CustomScaleDraw/customscaledraw.h"

/* Classes ********************************************************************/
class BWSViewer;
class JLViewer;
class SlideShowViewer;
class CScheduler;
class ReceiverManager;
// 接收机
class FDRMDialog : public CWindow, public Ui_DRMMainWindow
{
    Q_OBJECT

public:
    FDRMDialog(CTRx*, CSettings&,
           #ifdef HAVE_LIBHAMLIB
               CRig&,
           #endif
               QWidget* parent = 0);



    virtual ~FDRMDialog();
    int			        iMultimediaServiceBit;
protected:
    CRx&                rx;

    ReceiverManager* getReceiverManager(){return pReceiverManager; }
    ReceiverManager* pReceiverManager;
    bool                done;
    QTimer				Timer;
    std::vector<QLabel*>		serviceLabels;

    // main窗口 大小
    QSize initialSize;
    QSize max_soundbar_Size;
    QSize min_soundbar_Size;

    //频谱
    CDRMPlot*		pMainPlot;  //频谱
    int				iPlotStyle;
    QTimer MainPlotTimer;
    void FS_func();    //绘画  频谱
    CDRMPlot*		pConstellationPlot; //星座图
    int				iPlotStyle2;
    QTimer          LED_timer;

    // 槽函数
    void updateLabelColor();
    void OnOpenFile_soure();
    // 帧同步
    int Frame_syn ;

    // 模拟 sounbar    <QAudioInput>
    QAudioInput *audioInput;
    QIODevice *audioDevice;


    CLogging*			pLogging;
    systemevalDlg*		pSysEvalDlg;
    BWSViewer*			pBWSDlg;
    JLViewer*			pJLDlg;
    SlideShowViewer*	pSlideShowDlg;
    MultSettingsDlg*	pMultSettingsDlg;
    StationsDlg*		pStationsDlg;
    LiveScheduleDlg*	pLiveScheduleDlg;
    EPGDlg*				pEPGDlg;
    AnalogDemDlg*		pAnalogDemDlg;
    FMDialog*			pFMDlg;
    GeneralSettingsDlg* pGeneralSettingsDlg;
    QMenuBar*			pMenu;
    QButtonGroup*		pButtonGroup;
    QMenu*				pReceiverModeMenu;
    QMenu*				pSettingsMenu;
    QMenu*				pPlotStyleMenu;
    CSysTray*           pSysTray;
    CWindow*            pCurrentWindow;
    CFileMenu*			pFileMenu;
    CSoundCardSelMenu*	pSoundCardMenu; /* ! ! */
    CAboutDlg		    AboutDlg;

    int			        iLastMultimediaServiceSelected;
    QString             SysTrayTitle;
    QString             SysTrayMessage;
    QTimer				TimerSysTray;
    CScheduler* 	    pScheduler;
    QTimer*		        pScheduleTimer;
    Soundbar*           pSoundbar;
    void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
    virtual void        eventClose(QCloseEvent* ce);
    virtual void        eventHide(QHideEvent* pEvent);
    virtual void        eventShow(QShowEvent* pEvent);
    virtual void        eventUpdate();
    void		AddWhatsThisHelp();
    void		UpdateDRM_GUI();
    void		UpdateDisplay();
    void		ClearDisplay();
    void		UpdateWindowTitle(QString);

    void		SetDisplayColor(const QColor newColor);

    QString	GetCodecString(const CService&);
    QString	GetTypeString(const CService&);
    QString serviceSelector(CParameter&, int);
    void showTextMessage(const QString&);
    void showServiceInfo(const CService&);
    void startLogging();
    void stopLogging();
    void SysTrayCreate();
    void SysTrayStart();
    void SysTrayStop(const QString&);
    void SysTrayToolTip(const QString&, const QString&);
    void setBars(int);

    void changeEvent(QEvent *event) override
    {
        QWindowStateChangeEvent *stateChangeEvent =
                static_cast<QWindowStateChangeEvent*>(event);
        if (event->type() == QEvent::WindowStateChange)
        {
            if (isMaximized())
            {

                pSoundbar->moveSoundbar(max_soundbar_Size.width(),
                                        max_soundbar_Size.height());

            }
            else if (stateChangeEvent->oldState() & Qt::WindowMaximized)
            {
                pSoundbar->moveSoundbar(min_soundbar_Size.width(),
                                        min_soundbar_Size.height());

            }

        }

        QMainWindow::changeEvent(event);
    }
    void resizeEvent(QResizeEvent *event) override
    {
        (void )event;
        //        // 如果窗口不是最大化状态，忽略调整大小事件
        //        if (!isMaximized()) {
        //            resize(initialSize);
        //        } else {
        //            QMainWindow::resizeEvent(event);  // 调用基类实现
        //        }
    }

public slots:


    void OnTimer();
    void OnScheduleTimer();
    void OnSysTrayTimer();
    void OnSelectAudioService(int);
    void OnSelectDataService(int);
    void OnViewMultimediaDlg();
    void OnMenuSetDisplayColor();
    void OnNewAcquisition();
    void OnSwitchMode(int);
    void OnSwitchToFM();
    void OnSwitchToAM();
    void OnHelpAbout() {AboutDlg.show();}
    void OnSoundFileChanged(QString);
    void OnWhatsThis();
    void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
    void OnWorkingThreadFinished();
    void ChangeGUIModeToDRM();
    void ChangeGUIModeToAM();
    void ChangeGUIModeToFM();

    // Frequency Spectrum
    void onButtonClicked()
    {
        pMainPlot->SetupChart(CDRMPlot::ECharType::INPUTSPECTRUM_NO_AV);
        pMainPlot->OnTimerChart();
    }
    void onCheckBoxToggledFAC()
    {
        // 检查checkBox_SDC是否被选中
        if (checkBox_SDC->isChecked()|| checkBox_MSC->isChecked())
        {
            // 复选框被选中，执行相应的操作false
            checkBox_SDC->setCheckState(Qt::CheckState::Unchecked);
            checkBox_MSC->setCheckState(Qt::CheckState::Unchecked);
        }
        pConstellationPlot->SetupChart(CDRMPlot::ECharType::FAC_CONSTELLATION);
        pConstellationPlot->OnTimerChart();
    }
    void onCheckBoxToggledSDC()
    {
        // 检查checkBox_SDC是否被选中
        if (checkBox_FAC->isChecked()|| checkBox_MSC->isChecked())
        {
            // 复选框被选中，执行相应的操作false
            checkBox_FAC->setCheckState(Qt::CheckState::Unchecked);
            checkBox_MSC->setCheckState(Qt::CheckState::Unchecked);
        }
        pConstellationPlot->SetupChart(CDRMPlot::ECharType::SDC_CONSTELLATION);
        pConstellationPlot->OnTimerChart();
    }
    void onCheckBoxToggledMSC()
    {
        // 检查checkBox_SDC是否被选中
        if (checkBox_FAC->isChecked()|| checkBox_SDC->isChecked())
        {
            // 复选框被选中，执行相应的操作false
            checkBox_FAC->setCheckState(Qt::CheckState::Unchecked);
            checkBox_SDC->setCheckState(Qt::CheckState::Unchecked);
        }
        pConstellationPlot->SetupChart(CDRMPlot::ECharType::MSC_CONSTELLATION);
        pConstellationPlot->OnTimerChart();
    }
    // sound bar
    void processAudio();
    void receiveParameters();
signals:
    void plotStyleChanged(int);
    // void parametersReceived(ReceiverManager* pReceiverManager);
};

#endif // _FDRMDIALOG_H_
