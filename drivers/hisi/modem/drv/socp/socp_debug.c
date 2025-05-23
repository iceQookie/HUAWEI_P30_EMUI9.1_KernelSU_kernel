/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include "osl_types.h"
#include "bsp_rfile.h"
#include "bsp_icc.h"
#include "bsp_slice.h"
#include "osl_sem.h"
#include "osl_thread.h"
#include <bsp_module.h>
#include <securec.h>

#include "socp_balong.h"

#define  THIS_MODU mod_socp
#define SOCP_ROOT_PATH          "/data/hisi_logs/modem_log/socp/"


extern SOCP_DEBUG_INFO_S g_stSocpDebugInfo;
extern u32 g_ulSocpDebugTraceCfg;


struct socp_debug_icc_stru
{
    char OpsCmd[32];
};


typedef void (*socp_debug_ops)(void);

struct socp_debug_proc{
    char OpsCmd[32];
    socp_debug_ops OpsFunc;
};

struct socp_debug_ctrl_stru{
    osl_sem_id      task_sem;
    OSL_TASK_ID     task_id;
};
struct socp_debug_ctrl_stru g_stSocpDebugCtrl;


void socp_debug_help(void)
{
    socp_crit("help           :socp_debug_help\n");
    socp_crit("ap_count_clean :socp_debug_ApCountclean\n");
    socp_crit("ap_count_store :socp_debug_ApCountStore\n");
    socp_crit("cp_count_store :socp_debug_CpCountStore\n");
    socp_crit("cp_count_clean :socp_debug_CpCountClean\n");
    socp_crit("count_print    :socp_debug_PrintCount\n");
    socp_crit("reg_store      :socp_debug_RegStore\n");
    socp_crit("all_store      :socp_debug_AllStore\n");
    socp_crit("all_clean      :socp_debug_AllClean\n");
}

/* cov_verified_start */
void socp_debug_sendCmd(char* cmd)
{
    struct socp_debug_icc_stru icc_req;
    int ret;
    u32 channel_id = ICC_CHN_ACORE_CCORE_MIN <<16 | IFC_RECV_FUNC_SOCP_DEBUG;
    u32 cpu_id;

    cpu_id = (u32)ICC_CPU_MODEM;
    memset_s(icc_req.OpsCmd,sizeof(icc_req.OpsCmd),0,sizeof(icc_req.OpsCmd));
    /* coverity[overrun-buffer-arg] */ /* coverity[overrun-local] */
    memcpy_s(icc_req.OpsCmd, sizeof(icc_req.OpsCmd), cmd, strlen(cmd));/*lint !e666*/
    socp_crit("enter here:\n");
    ret = bsp_icc_send(cpu_id,channel_id, (u8*)&icc_req,sizeof(icc_req));
    if(ret != (int)sizeof(icc_req))
    {
        socp_error("bsp_icc_send failed(0x%x)\n",ret);
    }
    return;
}
void socp_debug_ApCountclean(void)
{
    memset_s(&g_stSocpDebugInfo,sizeof(g_stSocpDebugInfo),0x00,sizeof(g_stSocpDebugInfo));
}

void socp_debug_CountStore(char* p,int len)
{
    /* [false alarm]:alarm */
    char path[128];
    /* [false alarm]:alarm */
    int fd = -1;
    /* [false alarm]:alarm */
    int ret;

    memset_s(path,sizeof(path),0,sizeof(path));
    snprintf_s(path, sizeof(path), 128 - 1,"%s%s%d.bin",SOCP_ROOT_PATH,p,bsp_get_slice_value());

    /* [false alarm]:alarm */
    fd = bsp_open((s8*)path, RFILE_RDWR|RFILE_CREAT, 0660);
    /* [false alarm]:alarm */
    if(fd<0){
        socp_error("create %s error,save failed!\n",path);
        return;
    }
    /* [false alarm]:alarm */
    ret = bsp_write((u32)fd,(s8*)&g_stSocpDebugInfo,sizeof(g_stSocpDebugInfo));
    /* [false alarm]:alarm */
    if(ret != (int)sizeof(g_stSocpDebugInfo)){
        socp_error("write %s error,save failed!\n",path);
        (void)bsp_close((u32)fd);
        return;
    }
    (void)bsp_close((u32)fd);
    return;
}
void socp_debug_ApCountStore(void)
{
    char p[] = "ApCount_";
    socp_debug_CountStore(p,strlen(p));
}
void socp_debug_RegStore(void)
{
    /* [false alarm]:alarm */
    char path[128];
    /* [false alarm]:alarm */
    char p[] = "Reg_";
    /* [false alarm]:alarm */
    int fd = -1;
    /* [false alarm]:alarm */
    int ret;

    memset_s(path,sizeof(path),0,sizeof(path));
    snprintf_s(path, sizeof(path), 128 -1,"%s%s%d.bin",SOCP_ROOT_PATH,p,bsp_get_slice_value());
    /* [false alarm]:alarm */
    fd = bsp_open((s8*)path, RFILE_RDWR|RFILE_CREAT, 0660);
    /* [false alarm]:alarm */
    if(fd<0){
        socp_error("create %s error,save failed!\n",path);
        return;
    }
    /* [false alarm]:alarm */
    ret = bsp_write((u32)fd,(s8*)g_strSocpStat.baseAddr,4096);
    /* [false alarm]:alarm */
    if(ret != (int)4096){
        socp_error("write %s error | 0x%x,save failed!\n",path,ret);
        (void)bsp_close((u32)fd);
        return;
    }
    (void)bsp_close((u32)fd);
    return;
}
void socp_debug_CpCountStore(void)
{
    char p[] = "CpCount_";
    socp_debug_CountStore(p,(int)strlen(p));
}
void socp_debug_CpCountClean(void)
{
    memset_s(&g_stSocpDebugInfo,sizeof(g_stSocpDebugInfo),0x00,sizeof(g_stSocpDebugInfo));
}
void socp_debug_PrintCount(void)
{
    socp_help();
}

void socp_debug_AllStore(void)
{
    socp_debug_ApCountStore();
    socp_debug_CpCountStore();
    socp_debug_RegStore();
}

void socp_debug_AllClean(void)
{
    socp_debug_ApCountclean();
    socp_debug_CpCountClean();
}

struct socp_debug_proc g_strSocpOps[] = {
    {"help"                 ,socp_debug_help},
    {"ap_count_clean"       ,socp_debug_ApCountclean},
    {"ap_count_store"       ,socp_debug_ApCountStore},
    {"reg_store"            ,socp_debug_RegStore},
    {"cp_count_store"       ,socp_debug_CpCountStore},
    {"cp_count_clean"       ,socp_debug_CpCountClean},
    {"count_print"          ,socp_debug_PrintCount},
    {"all_store"            ,socp_debug_AllStore},
    {"all_clean"            ,socp_debug_AllClean},
};

int socp_debug_icc_msg_callback(u32 chanid ,u32 len,void* pdata)
{
    u32 channel_id = ICC_CHN_ACORE_CCORE_MIN <<16 | IFC_RECV_FUNC_SOCP_DEBUG;
    if(channel_id != chanid)
        return 0;

    socp_crit("enter here:\n");

    osl_sem_up(&g_stSocpDebugCtrl.task_sem);
    return 0;
}
int socp_debug_icc_task(void* para)
{
    int ret;
    u32 i;
    u32 chanid = ICC_CHN_ACORE_CCORE_MIN <<16 | IFC_RECV_FUNC_SOCP_DEBUG;
    u8  p[32];
    while(1)
    {
        osl_sem_down(&g_stSocpDebugCtrl.task_sem);

        memset_s(p, sizeof(p), 0, sizeof(p));

        ret = bsp_icc_read(chanid,p, sizeof(p));
        if(ret< (int)0 || ret >= (int)sizeof(p))
        {
            continue;
        }

        for(i = 0;i<sizeof(g_strSocpOps)/sizeof(g_strSocpOps[0]);i++)
        {
            if(!strncmp(g_strSocpOps[i].OpsCmd,(char*)p,strlen(g_strSocpOps[i].OpsCmd))){
                g_strSocpOps[i].OpsFunc();
            }
        }
    }
}

/*lint -save -e745*/
__init int socp_debug_init(void)
{
    u32 channel_id = ICC_CHN_ACORE_CCORE_MIN <<16 | IFC_RECV_FUNC_SOCP_DEBUG;

    osl_sem_init(0, &g_stSocpDebugCtrl.task_sem);

    (void)osl_task_init("socpProc",30, 0x1000, (OSL_TASK_FUNC)socp_debug_icc_task,NULL, &g_stSocpDebugCtrl.task_id);

    (void)bsp_icc_event_register(channel_id, (read_cb_func)socp_debug_icc_msg_callback,NULL,NULL,NULL);

    return 0;
}

/*****************************************************************************
* 函 数 名   : socp_debug_set_reg_bits
*
* 功能描述  : 测试socp功能时调用，用于设置socp寄存器
*
* 输入参数  :   reg
                pos
                bits
                val
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_debug_set_reg_bits(u32 reg, u32 pos, u32 bits, u32 val)
{
    SOCP_REG_SETBITS(reg, pos, bits, val);
}

/*****************************************************************************
* 函 数 名   : socp_debug_get_reg_bits
*
* 功能描述  : 测试socp功能时调用，用于设置socp寄存器
*
* 输入参数  :   reg
                pos
                bit
                val
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_debug_get_reg_bits(u32 reg, u32 pos, u32 bits)
{
    return SOCP_REG_GETBITS(reg, pos, bits);
}

/*****************************************************************************
* 函 数 名   : socp_debug_write_reg
*
* 功能描述  : 测试socp功能时调用，用于读取寄存器值
*
* 输入参数  :   reg
                result
*
* 输出参数  : 无
*
* 返 回 值   : 寄存器值
*****************************************************************************/
u32 socp_debug_read_reg(u32 reg)
{
    u32 result = 0;

    SOCP_REG_READ(reg, result);

    return result;
}

/*****************************************************************************
* 函 数 名   : socp_debug_read_reg
*
* 功能描述  : 测试socp功能时调用，用于读取寄存器值
*
* 输入参数  :   reg
                result
*
* 输出参数  : 无
*
* 返 回 值   : 返回码
*****************************************************************************/
u32 socp_debug_write_reg(u32 reg, u32 data)
{
    u32 result = 0;

    SOCP_REG_WRITE(reg, data);
    SOCP_REG_READ(reg, result);

    if(result == data)
    {
        return 0;
    }

    return (u32)-1;
}

/*****************************************************************************
* 函 数 名   : socp_help
*
* 功能描述  : 获取socp打印信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_help(void)
{
    socp_crit("\r socp_show_debug_gbl: 查看全局统计信息:通道申请、配置和中断总计数，无参数\n");
    socp_crit("\r socp_show_enc_src_chan_cur : 查看编码源通道属性，参数为通道ID\n");
    socp_crit("\r socp_show_enc_src_chan_add : 查看编码源通道操作统计值，参数为通道ID\n");
    socp_crit("\r socp_show_enc_src_chan_add : 查看所有编码源通道属性和统计值，无参数\n");
    socp_crit("\r socp_show_enc_dst_chan_cur : 查看编码目的通道属性，参数为通道ID\n");
    socp_crit("\r socp_show_enc_dst_chan_add : 查看编码目的通道操作统计值，参数为通道ID\n");
    socp_crit("\r socp_show_enc_dst_chan_all : 查看所有编码目的通道属性和统计值，无参数\n");
    socp_crit("\r socp_show_dec_src_chan_cur : 查看解码源通道属性，参数为通道ID\n");
    socp_crit("\r socp_show_dec_src_chan_add : 查看解码源通道操作统计值，参数为通道ID\n");
    socp_crit("\r socp_show_dec_src_chan_all : 查看所有解码源通道属性和统计值，无参数\n");
    socp_crit("\r socp_show_dec_dst_chan_cur : 查看解码目的通道属性，参数为通道ID\n");
    socp_crit("\r socp_show_dec_dst_chan_add : 查看解码目的通道操作统计值，参数为通道ID\n");
    socp_crit("\r socp_show_dec_dst_chan_all : 查看所有解码目的通道属性和统计值，无参数\n");
    socp_crit("\r socp_show_ccore_head_err_cnt : 查看C核所有编码源通道包头错误统计值，无参数\n");
    socp_crit("\r socp_debug_cnt_show : 查看全部统计信息，无参数\n");
}

/*****************************************************************************
* 函 数 名   : socp_show_debug_gbl
*
* 功能描述  : 显示全局debug 计数信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_show_debug_gbl(void)
{
    SOCP_DEBUG_GBL_S *sSocpDebugGblInfo;

    sSocpDebugGblInfo = &g_stSocpDebugInfo.sSocpDebugGBl;

    socp_crit(" SOCP全局状态维护信息:\n");
    socp_crit(" socp基地址:                   : 0x%x\n", (s32)g_strSocpStat.baseAddr);
    socp_crit(" socp申请编码源通道的次数      : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAllocEncSrcCnt);
    socp_crit(" socp申请编码源通道成功的次数  : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAllocEncSrcSucCnt);
    socp_crit(" socp配置编码目的通道的次数    : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpSetEncDstCnt);
    socp_crit(" socp配置编码目的通道成功的次数: 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpSetEncDstSucCnt);
    socp_crit(" socp配置解码源通道的次数      : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpSetDecSrcCnt);
    socp_crit(" socp配置解码源通道成功的次数  : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpSetDeSrcSucCnt);
    socp_crit(" socp申请解码目的通道的次数    : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAllocDecDstCnt);
    socp_crit(" socp申请解码目的通道成功的次数: 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAllocDecDstSucCnt);
    socp_crit(" socp进入APP中断的次数         : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAppEtrIntCnt);
    socp_crit(" socp完成APP中断的次数         : 0x%x\n", (s32)sSocpDebugGblInfo->u32SocpAppSucIntCnt);
}

/* cov_verified_stop */

/*****************************************************************************
* 函 数 名   : socp_show_enc_src_chan_cur
*
* 功能描述  : 打印编码源通道当前属性
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/

u32 socp_show_enc_src_chan_cur(u32 u32UniqueId)
{
    u32 u32RealId   = 0;
    u32 u32ChanType = 0;
    u32 ret;

    u32RealId   = SOCP_REAL_CHAN_ID(u32UniqueId);
    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);

    if((ret=(u32)socp_check_chan_type(u32ChanType, SOCP_CODER_SRC_CHAN)) != BSP_OK)
    {
        return ret;
    }
    if((ret=(u32)socp_check_encsrc_chan_id(u32RealId)) != BSP_OK)
    {
        return ret;
    }

    socp_crit("申请的编码源通道 0x%x 属性:\n", u32UniqueId);
    socp_crit("通道ID:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].u32ChanID);
    socp_crit("通道分配状态:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].u32AllocStat);
    socp_crit("通道使能状态:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].u32ChanEn);
    socp_crit("目的通道ID:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].u32DestChanID);
    socp_crit("通道优先级:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].ePriority);
    socp_crit("通道旁路状态:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].u32BypassEn);
    socp_crit("通道数据格式类型:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].eChnMode);
    socp_crit("通道所属模类型:\t\t%d\n", g_strSocpStat.sEncSrcChan[u32RealId].eDataType);
    socp_crit("通道buffer 起始地址:\t\t0x%pK\n", (char*)g_strSocpStat.sEncSrcChan[u32RealId].sEncSrcBuf.Start);
    socp_crit("通道buffer 结束地址:\t\t0x%pK\n", (char*)g_strSocpStat.sEncSrcChan[u32RealId].sEncSrcBuf.End);
    socp_crit("通道buffer 读指针:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sEncSrcBuf.u32Read);
    socp_crit("通道buffer 写指针:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sEncSrcBuf.u32Write);
    socp_crit("通道buffer 长度:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sEncSrcBuf.u32Length);
    if (SOCP_ENCSRC_CHNMODE_LIST == g_strSocpStat.sEncSrcChan[u32RealId].eChnMode)
    {
        socp_crit("通道RD buffer 起始地址:\t\t0x%pK\n", (char*)g_strSocpStat.sEncSrcChan[u32RealId].sRdBuf.Start);
        socp_crit("通道RD buffer 结束地址:\t\t0x%pK\n", (char*)g_strSocpStat.sEncSrcChan[u32RealId].sRdBuf.End);
        socp_crit("通道RD buffer 读指针:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sRdBuf.u32Read);
        socp_crit("通道RD buffer 写指针:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sRdBuf.u32Write);
        socp_crit("通道RD buffer 长度:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].sRdBuf.u32Length);
        socp_crit("通道RD buffer 门限:\t\t0x%x\n", g_strSocpStat.sEncSrcChan[u32RealId].u32RdThreshold);
    }

    return BSP_OK;
}



/*****************************************************************************
* 函 数 名   : socp_show_enc_src_chan_add
*
* 功能描述  : 打印编码源通道累计统计值
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_enc_src_chan_add(u32 u32UniqueId)
{
    u32 u32ChanType;
    u32 u32RealChanID;
    SOCP_DEBUG_ENCSRC_S *sSocpAddDebugEncSrc;
    s32 ret;

    u32RealChanID = SOCP_REAL_CHAN_ID(u32UniqueId);
    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);
    if((ret=socp_check_chan_type(u32ChanType, SOCP_CODER_SRC_CHAN)) != BSP_OK)
    {
        return (u32)ret;
    }

    sSocpAddDebugEncSrc = &g_stSocpDebugInfo.sSocpDebugEncSrc;

    if((ret=socp_check_encsrc_chan_id(u32RealChanID)) != BSP_OK)
    {
        return (u32)ret;
    }
    socp_crit("编码源通道0x%x累计统计值:\n", u32UniqueId);
    socp_crit("socp释放编码源通道成功的次数                           : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpFreeEncSrcCnt[u32RealChanID]);
    socp_crit("socp启动编码源通道成功的次数                           : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpStartEncSrcCnt[u32RealChanID]);
    socp_crit("socp停止编码源通道成功的次数                           : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpStopEncSrcCnt[u32RealChanID]);
    socp_crit("socp软复位编码源通道成功的次数                         : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpSoftResetEncSrcCnt[u32RealChanID]);
    socp_crit("socp注册编码源通道异常处理函数的次数                   : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpRegEventEncSrcCnt[u32RealChanID]);
    socp_crit("socp编码源通道尝试获得写buffer的次数                   : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpGetWBufEncSrcEtrCnt[u32RealChanID]);
    socp_crit("socp编码源通道获得写buffer成功的次数                   : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpGetWBufEncSrcSucCnt[u32RealChanID]);
    socp_crit("socp编码源通道尝试更新写buffer指针的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32socp_write_doneEncSrcEtrCnt[u32RealChanID]);
    socp_crit("socp编码源通道更新写buffer指针成功的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32socp_write_doneEncSrcSucCnt[u32RealChanID]);
    socp_crit("socp编码源通道更新写buffer指针失败的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32socp_write_doneEncSrcFailCnt[u32RealChanID]);
    socp_crit("socp编码源通道注册RD buffer回调函数成功的次数          : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpRegRdCBEncSrcCnt[u32RealChanID]);
    socp_crit("socp编码源通道尝试获得RD buffer的次数                  : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpGetRdBufEncSrcEtrCnt[u32RealChanID]);
    socp_crit("socp编码源通道获得RD buffer成功的次数                  : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpGetRdBufEncSrcSucCnt[u32RealChanID]);
    socp_crit("socp编码源通道尝试更新RDbuffer指针的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpReadRdDoneEncSrcEtrCnt[u32RealChanID]);
    socp_crit("socp编码源通道更新RDbuffer指针成功的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpReadRdDoneEncSrcSucCnt[u32RealChanID]);
    socp_crit("socp编码源通道更新RDbuffer指针失败的次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpReadRdDoneEncSrcFailCnt[u32RealChanID]);
    socp_crit("socp ISR 中进入编码源通道包头错误中断次数              : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcIsrHeadIntCnt[u32RealChanID]);
    socp_crit("socp 任务中回调编码源通道包头错误中断处理函数次数      : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcTskHeadCbOriCnt[u32RealChanID]);
    socp_crit("socp 回调编码源通道包头错误中断处理函数成功的次数      : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcTskHeadCbCnt[u32RealChanID]);
    socp_crit("socp ISR 中进入编码源通道Rd 完成中断次数               : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcIsrRdIntCnt[u32RealChanID]);
    socp_crit("socp 任务中回调编码源通道Rd 完成中断处理函数次数       : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcTskRdCbOriCnt[u32RealChanID]);
    socp_crit("socp 回调编码源通道Rd 完成中断处理函数成功的次数       : 0x%x\n",
           (s32)sSocpAddDebugEncSrc->u32SocpEncSrcTskRdCbCnt[u32RealChanID]);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_enc_src_chan_add
*
* 功能描述  : 打印所有编码源通道信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void  socp_show_enc_src_chan_all(void)
{
    u32 i;

    for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++)
    {
        (void)socp_show_enc_src_chan_cur(i);
        (void)socp_show_enc_src_chan_add(i);
    }

    return;
}

/*****************************************************************************
* 函 数 名   : socp_show_enc_dst_chan_cur
*
* 功能描述  : 打印编码目的通道信息
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_enc_dst_chan_cur(u32 u32UniqueId)
{
    u32 u32RealId   = 0;
    u32 u32ChanType = 0;
    u32 ret;

    u32RealId   = SOCP_REAL_CHAN_ID(u32UniqueId);
    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);

    if((ret=socp_check_chan_type(u32ChanType, SOCP_CODER_DEST_CHAN)) != BSP_OK)
    {
        return ret;
    }

    socp_crit("编码目的通道0x%x属性:\n", u32UniqueId);
    socp_crit("通道ID                 :%d\n", g_strSocpStat.sEncDstChan[u32RealId].u32ChanID);
    socp_crit("通道配置状态           :%d\n", g_strSocpStat.sEncDstChan[u32RealId].u32SetStat);
    socp_crit("通道buffer 起始地址    :0x%pK\n", (char*)g_strSocpStat.sEncDstChan[u32RealId].sEncDstBuf.Start);
    socp_crit("通道buffer 结束地址    :0x%pK\n", (char*)g_strSocpStat.sEncDstChan[u32RealId].sEncDstBuf.End);
    socp_crit("通道buffer 读指针      :0x%x\n", g_strSocpStat.sEncDstChan[u32RealId].sEncDstBuf.u32Read);
    socp_crit("通道buffer 写指针      :0x%x\n", g_strSocpStat.sEncDstChan[u32RealId].sEncDstBuf.u32Write);
    socp_crit("通道buffer 长度        :0x%x\n", g_strSocpStat.sEncDstChan[u32RealId].sEncDstBuf.u32Length);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_enc_dst_chan_add
*
* 功能描述  : 打印编码目的通道累计统计值
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_enc_dst_chan_add(u32 u32UniqueId)
{
    u32 u32RealChanID;
    u32 u32ChanType = 0;
    u32 ret;
    SOCP_DEBUG_ENCDST_S *sSocpAddDebugEncDst;

    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);
    if((ret=socp_check_chan_type(u32ChanType, SOCP_CODER_DEST_CHAN)) != BSP_OK)
    {
        return ret;
    }

    u32RealChanID = SOCP_REAL_CHAN_ID(u32UniqueId);
    sSocpAddDebugEncDst = &g_stSocpDebugInfo.sSocpDebugEncDst;

    socp_crit("编码目的通道 0x%x 累计统计值:\n", u32UniqueId);
    socp_crit("socp注册编码目的通道异常处理函数的次数                 : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpRegEventEncDstCnt[u32RealChanID]);
    socp_crit("socp编码目的通道注册读数据回调函数成功的次数           : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpRegReadCBEncDstCnt[u32RealChanID]);
    socp_crit("socp编码目的通道尝试获得读buffer 的次数                : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpGetReadBufEncDstEtrCnt[u32RealChanID]);
    socp_crit("socp编码目的通道获得读buffer成功的次数                 : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpGetReadBufEncDstSucCnt[u32RealChanID]);
    socp_crit("socp编码目的通道尝试更新读buffer指针的次数             : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32socp_read_doneEncDstEtrCnt[u32RealChanID]);
    socp_crit("socp编码目的通道更新读buffer指针成功的次数             : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32socp_read_doneEncDstSucCnt[u32RealChanID]);
    socp_crit("socp编码目的通道更新读buffer指针失败的次数             : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32socp_read_doneEncDstFailCnt[u32RealChanID]);
    socp_crit("socp编码目的通道更新读buffer指针移动0 字节成功的次数   : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32socp_read_doneZeroEncDstCnt[u32RealChanID]);
    socp_crit("socp编码目的通道更新读buffer指针移动非0 字节成功的次数 : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32socp_read_doneValidEncDstCnt[u32RealChanID]);
    socp_crit("socpISR 中进入编码目的通道传输完成中断次数             : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstIsrTrfIntCnt[u32RealChanID]);
    socp_crit("socp任务中回调编码目的通道传输完成中断处理函数次数     : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskTrfCbOriCnt[u32RealChanID]);
    socp_crit("socp回调编码目的通道传输完成中断处理函数成功的次数     : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskTrfCbCnt[u32RealChanID]);
    socp_crit("socpISR 中进入编码目的通道buf 溢出中断次数             : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstIsrOvfIntCnt[u32RealChanID]);
    socp_crit("socp任务中回调编码目的通道buf 溢出中断处理函数次数    : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskOvfCbOriCnt[u32RealChanID]);
    socp_crit("socp回调编码目的通道buf 溢出中断处理函数成功的次数    : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskOvfCbCnt[u32RealChanID]);
    socp_crit("socpISR 中进入编码目的通道buf阈值溢出中断次数          : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstIsrThresholdOvfIntCnt[u32RealChanID]);
    socp_crit("socp任务中回调编码目的通道buf阈值溢出中断处理函数次数  : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskThresholdOvfCbOriCnt[u32RealChanID]);
    socp_crit("socp回调编码目的通道buf阈值溢出中断处理函数成功的次数  : 0x%x\n",
           (s32)sSocpAddDebugEncDst->u32SocpEncDstTskThresholdOvfCbCnt[u32RealChanID]);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_enc_dst_chan_all
*
* 功能描述  : 打印编码目的通道信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_show_enc_dst_chan_all(void)
{
    u32 i;
    u32 u32UniqueId = 0;

    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++)
    {
        u32UniqueId = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
        (void)socp_show_enc_dst_chan_cur(u32UniqueId);
        (void)socp_show_enc_dst_chan_add(u32UniqueId);
    }

    return;
}
/*****************************************************************************
* 函 数 名   : socp_show_dec_src_chan_cur
*
* 功能描述  : 打印解码源通道信息
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_dec_src_chan_cur(u32 u32UniqueId)
{
    u32 u32RealId   = 0;
    u32 u32ChanType = 0;
    u32 ret;

    u32RealId   = SOCP_REAL_CHAN_ID(u32UniqueId);
    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);

    if((ret=socp_check_chan_type(u32ChanType, SOCP_DECODER_SRC_CHAN)) != BSP_OK)
    {
        return ret;
    }

    socp_crit("解码源通道 0x%x 属性:\n", u32UniqueId);
    socp_crit("通道ID                 :%d\n", g_strSocpStat.sDecSrcChan[u32RealId].u32ChanID);
    socp_crit("通道配置状态           :%d\n", g_strSocpStat.sDecSrcChan[u32RealId].u32SetStat);
    socp_crit("通道使能状态           :%d\n", g_strSocpStat.sDecSrcChan[u32RealId].u32ChanEn);
    socp_crit("通道模式               :%d\n", g_strSocpStat.sDecSrcChan[u32RealId].eChnMode);
    socp_crit("通道buffer 起始地址    :0x%pK\n", (char*)g_strSocpStat.sDecSrcChan[u32RealId].sDecSrcBuf.Start);
    socp_crit("通道buffer 结束地址    :0x%pK\n", (char*)g_strSocpStat.sDecSrcChan[u32RealId].sDecSrcBuf.End);
    socp_crit("通道buffer 读指针      :0x%x\n", g_strSocpStat.sDecSrcChan[u32RealId].sDecSrcBuf.u32Read);
    socp_crit("通道buffer 写指针      :0x%x\n", g_strSocpStat.sDecSrcChan[u32RealId].sDecSrcBuf.u32Write);
    socp_crit("通道buffer 长度        :0x%x\n", g_strSocpStat.sDecSrcChan[u32RealId].sDecSrcBuf.u32Length);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_dec_src_chan_add
*
* 功能描述  : 打印解码源通道累计统计值
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_dec_src_chan_add(u32 u32UniqueId)
{
    u32 u32RealChanID;
    SOCP_DEBUG_DECSRC_S *sSocpAddDebugDecSrc;
    u32 u32ChanType = 0;
    u32 ret;

    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);
    if((ret=socp_check_chan_type(u32ChanType, SOCP_DECODER_SRC_CHAN)) != BSP_OK)
    {
        return ret;
    }
    u32RealChanID = SOCP_REAL_CHAN_ID(u32UniqueId);
    sSocpAddDebugDecSrc = &g_stSocpDebugInfo.sSocpDebugDecSrc;

    socp_crit("解码源通道 0x%x 累计统计值:\n", u32UniqueId);
    socp_crit("socp软复位解码源通道成功的次数                     : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpSoftResetDecSrcCnt[u32RealChanID]);
    socp_crit("socp启动解码源通道成功的次数                       : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpStartDecSrcCnt[u32RealChanID]);
    socp_crit("socp停止解码源通道成功的次数                       : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpStopDecSrcCnt[u32RealChanID]);
    socp_crit("socp注册解码源通道异常处理函数的次数               : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpRegEventDecSrcCnt[u32RealChanID]);
    socp_crit("socp解码源通道尝试获得写buffer的次数               : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpGetWBufDecSrcEtrCnt[u32RealChanID]);
    socp_crit("socp解码源通道获得写buffer成功的次数               : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpGetWBufDecSrcSucCnt[u32RealChanID]);
    socp_crit("socp解码源通道尝试更新写buffer指针的次数           : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32socp_write_doneDecSrcEtrCnt[u32RealChanID]);
    socp_crit("socp解码源通道更新写buffer指针成功的次数           : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32socp_write_doneDecSrcSucCnt[u32RealChanID]);
    socp_crit("socp解码源通道更新写buffer指针失败的次数           : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32socp_write_doneDecSrcFailCnt[u32RealChanID]);
    socp_crit("socpISR 中进入解码源通道错误中断次数               : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpDecSrcIsrErrIntCnt[u32RealChanID]);
    socp_crit("socp任务中回调解码源通道错误中断处理函数次数       : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpDecSrcTskErrCbOriCnt[u32RealChanID]);
    socp_crit("socp回调解码源通道错误中断处理函数成功的次数       : 0x%x\n",
           (s32)sSocpAddDebugDecSrc->u32SocpDecSrcTskErrCbCnt[u32RealChanID]);
    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_dec_src_chan_all
*
* 功能描述  : 打印解码源通道信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_show_dec_src_chan_all(void)
{
    u32 i;
    u32 u32UniqueId = 0;

    for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++)
    {
        u32UniqueId = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN, i);
        (void)socp_show_dec_src_chan_cur(u32UniqueId);
        (void)socp_show_dec_src_chan_add(u32UniqueId);
    }

    return;
}

/*****************************************************************************
* 函 数 名   : socp_show_dec_dst_chan_cur
*
* 功能描述  : 打印解码目的通道信息
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_dec_dst_chan_cur(u32 u32UniqueId)
{
    u32 u32RealId   = 0;
    u32 u32ChanType = 0;
    u32 ret;

    u32RealId   = SOCP_REAL_CHAN_ID(u32UniqueId);
    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);

    if((ret=socp_check_chan_type(u32ChanType, SOCP_DECODER_DEST_CHAN)) != BSP_OK)
    {
        return ret;
    }
    socp_crit("解码目的通道 0x%x 属性:\n", u32UniqueId);
    socp_crit("通道ID                 :%d\n", g_strSocpStat.sDecDstChan[u32RealId].u32ChanID);
    socp_crit("通道分配状态           :%d\n", g_strSocpStat.sDecDstChan[u32RealId].u32AllocStat);
    socp_crit("通道使用模类型         :%d\n", g_strSocpStat.sDecDstChan[u32RealId].eDataType);
    socp_crit("通道buffer 起始地址    :0x%pK\n", (char*)g_strSocpStat.sDecDstChan[u32RealId].sDecDstBuf.Start);
    socp_crit("通道buffer 结束地址    :0x%pK\n", (char*)g_strSocpStat.sDecDstChan[u32RealId].sDecDstBuf.End);
    socp_crit("通道buffer 读指针      :0x%x\n", g_strSocpStat.sDecDstChan[u32RealId].sDecDstBuf.u32Read);
    socp_crit("通道buffer 写指针      :0x%x\n", g_strSocpStat.sDecDstChan[u32RealId].sDecDstBuf.u32Write);
    socp_crit("通道buffer 长度        :0x%x\n", g_strSocpStat.sDecDstChan[u32RealId].sDecDstBuf.u32Length);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_dec_dst_chan_add
*
* 功能描述  : 打印解码目的通道累计统计值
*
* 输入参数  : 通道ID
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
u32 socp_show_dec_dst_chan_add(u32 u32UniqueId)
{
    u32 u32RealChanID;
    SOCP_DEBUG_DECDST_S *sSocpAddDebugDecDst;
    u32 u32ChanType = 0;
    u32 ret;

    u32ChanType = SOCP_REAL_CHAN_TYPE(u32UniqueId);
    if((ret=socp_check_chan_type(u32ChanType, SOCP_DECODER_DEST_CHAN)) != BSP_OK)
    {
        return ret;
    }

    u32RealChanID = SOCP_REAL_CHAN_ID(u32UniqueId);
    sSocpAddDebugDecDst = &g_stSocpDebugInfo.sSocpDebugDecDst;

    socp_crit("================== 解码目的通道 0x%x  累计统计值:=================\n", u32UniqueId);
    socp_crit("socp释放解码目的通道成功的次数                         : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpFreeDecDstCnt[u32RealChanID]);
    socp_crit("socp注册解码目的通道异常处理函数的次数                 : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpRegEventDecDstCnt[u32RealChanID]);
    socp_crit("socp解码目的通道注册读数据回调函数成功的次数           : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpRegReadCBDecDstCnt[u32RealChanID]);
    socp_crit("socp解码目的通道尝试获得读buffer的次数                 : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpGetReadBufDecDstEtrCnt[u32RealChanID]);
    socp_crit("socp解码目的通道获得读buffer成功的次数                 : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpGetReadBufDecDstSucCnt[u32RealChanID]);
    socp_crit("socp解码目的通道尝试更新读buffer指针的次数             : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32socp_read_doneDecDstEtrCnt[u32RealChanID]);
    socp_crit("socp解码目的通道更新读buffer指针成功的次数             : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32socp_read_doneDecDstSucCnt[u32RealChanID]);
    socp_crit("socp解码目的通道更新读buffer指针失败的次数             : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32socp_read_doneDecDstFailCnt[u32RealChanID]);
    socp_crit("socp解码目的通道更新读buffer指针移动0 字节成功的次数   : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32socp_read_doneZeroDecDstCnt[u32RealChanID]);
    socp_crit("socp解码目的通道更新读buffer指针移动非0 字节成功的次数 : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32socp_read_doneValidDecDstCnt[u32RealChanID]);
    socp_crit("socpISR 中进入解码目的通道传输完成中断次数             : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstIsrTrfIntCnt[u32RealChanID]);
    socp_crit("socp任务中 回调解码目的通道传输完成中断处理函数的次数  : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstTskTrfCbOriCnt[u32RealChanID]);
    socp_crit("socp回调解码目的通道传输完成中断处理函数成功的次数     : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstTskTrfCbCnt[u32RealChanID]);
    socp_crit("socpISR 中进入解码目的通道buf 溢出中断次数             : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstIsrOvfIntCnt[u32RealChanID]);
    socp_crit("socp任务中 回调解码目的通道buf 溢出中断处理函数次数    : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstTskOvfCbOriCnt[u32RealChanID]);
    socp_crit("socp回调解码目的通道buf 溢出中断处理函数成功的次数     : 0x%x\n",
           (s32)sSocpAddDebugDecDst->u32SocpDecDstTskOvfCbCnt[u32RealChanID]);

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名   : socp_show_dec_dst_chan_all
*
* 功能描述  : 打印解码目的通道信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_show_dec_dst_chan_all(void)
{
    u32 i;
    u32 u32UniqueId = 0;

    for (i = 0; i < SOCP_MAX_DECDST_CHN; i++)
    {
        u32UniqueId = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN, i);
        (void)socp_show_dec_dst_chan_cur(u32UniqueId);
        (void)socp_show_dec_dst_chan_add(u32UniqueId);
    }

    return;
}

EXPORT_SYMBOL(socp_show_dec_src_chan_cur);
EXPORT_SYMBOL(socp_show_dec_src_chan_add);
EXPORT_SYMBOL(socp_show_dec_src_chan_all);
EXPORT_SYMBOL(socp_show_dec_dst_chan_cur);
EXPORT_SYMBOL(socp_show_dec_dst_chan_add);
EXPORT_SYMBOL(socp_show_dec_dst_chan_all);

/*****************************************************************************
* 函 数 名   : socp_debug_cnt_show
*
* 功能描述  : 显示debug 计数信息
*
* 输入参数  : 无
*
* 输出参数  : 无
*
* 返 回 值   : 无
*****************************************************************************/
void socp_debug_cnt_show(void)
{
    socp_show_debug_gbl();
    socp_show_enc_src_chan_all();
    socp_show_enc_dst_chan_all();
    socp_show_dec_src_chan_all();
    socp_show_dec_dst_chan_all();
}

void socp_debug_set_trace(u32 v)
{
    g_ulSocpDebugTraceCfg = v;
}
EXPORT_SYMBOL(socp_debug_help);
EXPORT_SYMBOL(socp_debug_AllStore);
EXPORT_SYMBOL(socp_debug_AllClean);
EXPORT_SYMBOL(socp_debug_set_reg_bits);
EXPORT_SYMBOL(socp_debug_get_reg_bits);
EXPORT_SYMBOL(socp_debug_read_reg);
EXPORT_SYMBOL(socp_debug_write_reg);
EXPORT_SYMBOL(socp_help);
EXPORT_SYMBOL(socp_show_debug_gbl);
EXPORT_SYMBOL(socp_show_enc_src_chan_cur);
EXPORT_SYMBOL(socp_show_enc_src_chan_add);
EXPORT_SYMBOL(socp_show_enc_src_chan_all);
EXPORT_SYMBOL(socp_show_enc_dst_chan_cur);
EXPORT_SYMBOL(socp_show_enc_dst_chan_add);
EXPORT_SYMBOL(socp_show_enc_dst_chan_all);
EXPORT_SYMBOL(socp_debug_cnt_show);




