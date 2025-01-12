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

#ifndef _TRANSMDLG_H_
#define _TRANSMDLG_H_

#include "ui_TransmDlgbase.h"
#include "AACCodecParams.h"
#include "OpusCodecParams.h"
#include "DialogUtil.h"
#include "CWindow.h"
#include "../DrmTransmitter.h"
#include "../Parameter.h"
#include <QPushButton>
#include <QString>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QFileInfo>
#include <QStringList>
#include <QMenuBar>
#include <QLayout>
#include <QThread>
#include <QTimer>
#include <QMainWindow>
#include <QMenu>
#include <qwt_thermo.h>
#include "../Soundbar/soundbar.h"
#include <QMessageBox>
#include "../main-Qt/crx.h"
#include "fdrmdialog.h"
#include <QStringListModel>
#include <stdlib.h>
#include "../Soundbar/soundbar.h"

#include "../MDI/managementsource.h"
#include "../MDI/globalvariables_.h"


class ReceiverManager;

char* my_itoa(int value, char* str, int base);
class ReceiverManager;
/* Classes ********************************************************************/
class CTx;

class TransmDialog : public CWindow, public Ui_TransmDlgBase
{
	Q_OBJECT

public:
    TransmDialog(CTx&, QWidget* parent=nullptr);
	virtual ~TransmDialog();
    ReceiverManager* pReceiverManager;
protected:
	virtual void eventClose(QCloseEvent*);
	void DisableAllControlsForSet();
	void EnableAllControlsForSet();
	void TabWidgetEnableTabs(QTabWidget* tabWidget, bool enable);

    //my
    QStringListModel * model;
    QStringList list;    // 私有成员变量
    QTimer				Timer_500ms;  //用来刷新 监测界面

    CTx&                tx; /* Working thread object 工作线程对象*/
	CAboutDlg			AboutDlg;
	QTimer				Timer;
	QTimer				TimerStop;
	CVector<string>		vecstrTextMessage;
	QMenu*				pSettingsMenu;
    AACCodecParams*		pAACCodecDlg;
    OpusCodecParams*	pOpusCodecDlg;
    CSysTray*           pSysTray;
	QAction*			pActionStartStop;

	bool			bIsStarted;
	int					iIDCurrentText;
	int					iServiceDescr;
	bool			bCloseRequested;
	int					iButtonCodecState;

	void				ShowButtonCodec(bool bShow, int iKey);
    bool			    GetMessageText(const int iID);
	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const bool bFlag);
	void				EnableAudio(const bool bFlag);
	void				EnableData(const bool bFlag);
	void				AddWhatsThisHelp();
    QString  Getserver0_4(CDRMReceiver&  rx, CParameter &Parameters,int index);
    void updateComboBoxFromListView();
    void modifyListViewItem(int index, const QString &newText);
    void setServer0_4(int index,  QString serverlabel );
public slots:

    //1 2 3 switch
    void Onswitch_sourse();
    void Onswitch_reuse();
    void Onswitch_modulation();
    //ReceiverManager receiverManager CRx &rx
    void onParametersReceived();
    //my
    void onRefresh_Tx_Ui();

	void OnButtonStartStop();
	void OnPushButtonAddText();
	void OnButtonClearAllText();
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
	void OnButtonCodec();
	void OnToggleCheckBoxHighQualityIQ(bool bState);
	void OnToggleCheckBoxAmplifiedOutput(bool bState);
	void OnToggleCheckBoxEnableData(bool bState);
	void OnToggleCheckBoxEnableAudio(bool bState);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnToggleCheckBoxRemovePath(bool bState);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxLanguageActivated(int iID);
	void OnComboBoxProgramTypeActivated(int iID);
	void OnComboBoxTextMessageActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioOutput(int iID);
	void OnRadioCodec(int iID);
	void OnRadioCurrentTime(int iID);
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnTimer();
	void OnTimerStop();
	void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
	void OnWhatsThis();

};

QString GetRobmodestr(CParameter& Parameters);

//谱
QString GetSpectrumOccup(CParameter& Parameters);

//交织
QString GetInterl(CParameter& Parameters);

QString GetSDCmode (CParameter& Parameters);

QString GetMSCmode (CParameter& Parameters);

QString	GetCodecstring(const CService&service);
QString	GetTypestring(const CService& service );
QString Getserver0_4(CRx &rx ,CParameter& Parameters);
#endif // _TRANSMDLG_H_
