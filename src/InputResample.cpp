/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Resample input DRM stream
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

#include "InputResample.h"


/* Implementation *************************************************************/
void CInputResample::ProcessDataInternal(CParameter& Parameters)
{

    // 是否同步输入  直接复制数据还是进行重采样。
    // cout<<"bSyncInput: "<<bSyncInput<<endl;  //else
    // bSyncInput = true;
    if (bSyncInput)
    {
        /* Only copy input data to output buffer, do not resample data */
        for (int i = 0; i < iInputBlockSize; i++)
            (*pvecOutputData)[i] = (*pvecInputData)[i];

        iOutputBlockSize = iInputBlockSize;
        // cout<<"同步iOutputBlockSize "<<iOutputBlockSize <<endl;
    }
    else
    {
        Parameters.Lock();
        // 获取重采样偏移量（rSamRateOffset）
        _REAL rSamRateOffset = Parameters.rResampleOffset;
        // 信号采样率（rSigSampleRate）
        _REAL rSigSampleRate = _REAL(Parameters.GetSigSampleRate());
        Parameters.Unlock();

        /* Adjust maximum resample offset to sample rate */
        const _REAL rMaxResampleOffset = ADJ_FOR_SRATE(_REAL(MAX_RESAMPLE_OFFSET),
                                                       rSigSampleRate);

        /* Constrain the sample rate offset estimation to prevent from an
           output buffer overrun */
        if (rSamRateOffset > rMaxResampleOffset)
            rSamRateOffset = rMaxResampleOffset;
        if (rSamRateOffset < -rMaxResampleOffset)
            rSamRateOffset = -rMaxResampleOffset;

        /* Do actual resampling */
        //调用 Resample方法进行重采样
        iOutputBlockSize = ResampleObj.Resample(pvecInputData, pvecOutputData,
                                                rSigSampleRate /
                                                (rSigSampleRate - rSamRateOffset));
        // cout<<"else同步 iOutputBlockSize "<<iOutputBlockSize <<endl;
    }

}

void CInputResample::InitInternal(CParameter& Parameters)
{
    Parameters.Lock();
    /* Init resample object */
    ResampleObj.Init(Parameters.CellMappingTable.iSymbolBlockSize);

    /* Define block-sizes for input and output */
    iInputBlockSize = Parameters.CellMappingTable.iSymbolBlockSize;

    //打印现象是 CInputResample input size = 1280
    // cout<<"CInputResample input size = "<<  iInputBlockSize <<endl;


    /* With this parameter we define the maximum lenght of the output buffer
       Due to the constrained sample rate offset estimation the output
       buffer size is also constrained to a certain number of samples. The
       maximum possible number of samples defines the output buffer maximum
       memory allocation.
       We have to consider the following case: The output block size is
       smaller than one symbol -> no data is read by the next unit, but
       after that the output block size is bigger than one symbol, therefore
       we have to allocate three symbols for output buffer
        使用这个参数，我们定义输出缓冲区的最大长度由于采样率的约束，输出的偏移估计
        缓冲区大小也受限于一定数量的样本。的最大可能的样本数定义了输出缓冲区的最大值
        内存分配。
        我们必须考虑以下情况:输出块大小为 小于一个符号->下一个单元不读取数据，但是
        因此，在此之后，输出块的大小大于一个符号 我们必须为输出缓冲区分配三个符号
        */
    iMaxOutputBlockSize = 3 * Parameters.CellMappingTable.iSymbolBlockSize;
    Parameters.Unlock();
}
