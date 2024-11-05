#include "ctx.h"

#include "../DrmTransmitter.h"
//ReceiverManager
#include "../receivermanager.h"

CTx::CTx(CDRMTransmitter& nTx, CTRx *parent):
    CTRx(parent), tx(nTx), eRunState(STOPPED)
{
    //OutputDeviceChanged  连接 std::string  信号(不行)
    //    connect(parent, SIGNAL(OutputDeviceChanged(const std::string&)),
    //            this, SLOT(SetOutputDevice(std::string )));



}

CTx::~CTx()
{

}

void CTx::run()
{
    qDebug("Working thread started");
    try
    {
        /* Set restart flag */
        eRunState = RESTART;
        do
        {
            /* Initialization of the modules */
            tx.Init();
            /* Set run flag */
            eRunState = RUNNING;

            /* Start the transmitter run routine */
            //tx.eRunState_m = this->GetRunState();
            tx.peRunState_m = &eRunState;
            tx.Run();
        }
        while (eRunState == RESTART);

        /* Closing the sound interfaces */
        tx.Close();
    }
    catch (CGenErr GenErr)
    {
        qDebug("%s", GenErr.strError.c_str());
    }
    catch (string strError)
    {
        qDebug("%s", strError.c_str());
    }
    qDebug("Transmitter working thread complete");
}

void CTx::LoadSettings()
{
    tx.LoadSettings();
}

void CTx::SaveSettings()
{
    tx.SaveSettings();
}

void CTx::SetInputDevice(string s)
{
    tx.SetInputDevice(s);
}

void CTx::SetOutputDevice(string s)
{

    DEBUG_COUT("发射机切换 输出声卡设备");
    tx.SetOutputDevice(s);
}



void CTx::GetInputDevice(string& s)
{
    tx.GetInputDevice(s);
}

void CTx::GetOutputDevice(string& s)
{
    tx.GetOutputDevice(s);
}

void CTx::EnumerateInputs(vector<string>& names, vector<string>& descriptions, string& defaultInput)
{
    tx.EnumerateInputs(names, descriptions, defaultInput);
}

void CTx::EnumerateOutputs(vector<string>& names,
                           vector<string>& descriptions,
                           string& defaultOutput)
{
    tx.EnumerateOutputs(names, descriptions, defaultOutput);
}

void CTx::Restart()
{
    eRunState = RESTART;
}

void CTx::Stop()
{
    //DEBUG_COUT("发射机 eRunState = "<< eRunState);
    eRunState = STOP_REQUESTED;
}

CSettings* CTx::GetSettings()
{
    return tx.GetSettings();
}

void CTx::SetSettings(CSettings* s)
{
    tx.SetSettings(s);
}

CParameter*	CTx::GetParameters()
{
    return tx.GetParameters();
}

int CTx::GetIQOutput()
{
    return tx.GetTransData()->GetIQOutput();
}

bool CTx::isHighQualityIQ()
{
    return tx.GetTransData()->GetHighQualityIQ();
}

bool CTx::isOutputAmplified()
{
    return tx.GetTransData()->GetAmplifiedOutput();
}

bool CTx::CanEncode(int n)
{
    return tx.GetAudSrcEnc()->CanEncode(CAudioParam::EAudCod(n));
}

_REAL CTx::GetCarrierOffset()
{
    return tx.GetCarOffset();
}

int CTx::GetLevelMeter()
{
    return tx.GetReadData()->GetLevelMeter();
}

bool CTx::GetTransmissionStatus(string s, _REAL r)
{
    return tx.GetAudSrcEnc()->GetTransStat(s, r);
}

int CTx::GetFrequency()
{
    return 0; // Tx does not do RF Upconversion
}

ERunState CTx::GetRunState()
{
    return this->eRunState;
}



void CTx::SetFrequency(int)
{
}

ReceiverManager* CTx::getTx_manager()
{
   return  this->tx.pReceiverManager_m;
}


void CTx::SetTextMessage(string s)
{
    tx.GetAudSrcEnc()->SetTextMessage(s);
}

void CTx::ClearTextMessage()
{
    tx.GetAudSrcEnc()->ClearTextMessage();
}

void CTx::SetPicFileName(string s, string f)
{
    tx.GetAudSrcEnc()->SetPicFileName(s, f);
}

void CTx::SetPathRemoval(bool b)
{
    tx.GetAudSrcEnc()->SetPathRemoval(b);
}

void CTx::ClearPicFileNames()
{
    tx.GetAudSrcEnc()->ClearPicFileNames();
}

void CTx::SetHighQualityIQ(bool b)
{
    tx.GetTransData()->SetHighQualityIQ(b);
}

void CTx::SetOutputAmplified(bool b)
{
    tx.GetTransData()->SetAmplifiedOutput(b);
}

void CTx::SetCarrierOffset(_REAL r)
{
    tx.GetTransData()->SetCarOffset(r);
}

void CTx::SetIQOutput(int n)
{
    tx.GetTransData()->SetIQOutput(CTransmitData::EOutFormat(n));
}
