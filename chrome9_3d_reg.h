/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CHROME9_3D_REG_H
#define CHROME9_3D_REG_H
#define getmmioregister(base, offset)      \
	(*(__volatile__ unsigned int *)(void *)(((unsigned char *)(base)) + \
	(offset)))
#define setmmioregister(base, offset, val) \
	(*(__volatile__ unsigned int *)(void *)(((unsigned char *)(base)) + \
	(offset)) = (val))

#define getmmioregisteru8(base, offset)      \
	(*(__volatile__ unsigned char *)(void *)(((unsigned char *)(base)) + \
	(offset)))
#define setmmioregisteru8(base, offset, val) \
	(*(__volatile__ unsigned char *)(void *)(((unsigned char *)(base)) + \
	(offset)) = (val))

#define bci_send(bci, value)   (*(bci)++ = (unsigned long)(value))
#define bci_set_stream_register(bci_base, bci_index, reg_value)         \
do {                                                                    \
	unsigned long cmd;                                              \
									\
	cmd = (0x90000000                                               \
		| (1<<16) /* stream processor register */               \
		| (bci_index & 0x3FFC)); /* MMIO register address */    \
	bci_send(bci_base, cmd);                                        \
	bci_send(bci_base, reg_value);                                  \
	} while (0)

/* Command Header Type */

#define INV_DUMMY_MASK              0xFF000000
#define INV_AGPHEADER0              0xFE000000
#define INV_AGPHEADER1              0xFE010000
#define INV_AGPHEADER2              0xFE020000
#define INV_AGPHEADER3              0xFE030000
#define INV_AGPHEADER4              0xFE040000
#define INV_AGPHEADER5              0xFE050000
#define INV_AGPHEADER6              0xFE060000
#define INV_AGPHEADER7              0xFE070000
#define INV_AGPHEADER9              0xFE090000
#define INV_AGPHEADERA              0xFE0A0000
#define INV_AGPHEADER40             0xFE400000
#define INV_AGPHEADER41             0xFE410000
#define INV_AGPHEADER43             0xFE430000
#define INV_AGPHEADER45             0xFE450000
#define INV_AGPHEADER47             0xFE470000
#define INV_AGPHEADER4A             0xFE4A0000
#define INV_AGPHEADER82             0xFE820000
#define INV_AGPHEADER83             0xFE830000
#define INV_AGPHEADER_MASK          0xFFFF0000
#define INV_AGPHEADER25             0xFE250000
#define INV_AGPHEADER20             0xFE200000
#define INV_AGPHEADERE2             0xFEE20000
#define INV_AGPHEADERE3             0xFEE30000

/*send pause address of AGP ring command buffer via_chrome9 this IO port*/
#define INV_REG_PCIPAUSE            0x294
#define INV_REG_PCIPAUSE_ENABLE     0x4

#define INV_CMDBUF_THRESHOLD     (8)
#define INV_QW_PAUSE_ALIGN       0x40

/* Transmission IO Space*/
#define INV_REG_CR_TRANS            0x041C
#define INV_REG_CR_BEGIN            0x0420
#define INV_REG_CR_END              0x0438

#define INV_REG_3D_TRANS            0x043C
#define INV_REG_3D_BEGIN            0x0440
#define INV_REG_3D_END              0x06FC
#define INV_REG_23D_WAIT            0x326C
/*3D / 2D ID Control (Only For Group A)*/
#define INV_REG_2D3D_ID_CTRL     0x060


/* Engine Status */

#define INV_RB_ENG_STATUS           0x0400
#define INV_ENG_BUSY_HQV0           0x00040000
#define INV_ENG_BUSY_HQV1           0x00020000
#define INV_ENG_BUSY_CR             0x00000010
#define INV_ENG_BUSY_MPEG           0x00000008
#define INV_ENG_BUSY_VQ             0x00000004
#define INV_ENG_BUSY_2D             0x00000002
#define INV_ENG_BUSY_3D             0x00001FE1
#define INV_ENG_BUSY_ALL            		\
	(INV_ENG_BUSY_2D | INV_ENG_BUSY_3D | INV_ENG_BUSY_CR)

/* Command Queue Status*/
#define INV_RB_VQ_STATUS            0x0448
#define INV_VQ_FULL                 0x40000000

/* AGP command buffer pointer current position*/
#define INV_RB_AGPCMD_CURRADDR      0x043C

/* AGP command buffer status*/
#define INV_RB_AGPCMD_STATUS        0x0444
#define INV_AGPCMD_INPAUSE          0x80000000

/*AGP command buffer pause address*/
#define INV_RB_AGPCMD_PAUSEADDR     0x045C

/*AGP command buffer jump address*/
#define INV_RB_AGPCMD_JUMPADDR      0x0460

/*AGP command buffer start address*/
#define INV_RB_AGPCMD_STARTADDR      0x0464


/* Constants */
#define NUMBER_OF_EVENT_TAGS        1024
#define NUMBER_OF_APERTURES_CLB     16

/* Register definition */
#define HW_SHADOW_ADDR              0x8520
#define HW_GARTTABLE_ADDR           0x8540

#define INV_HSWFLAG_DBGMASK          0x00000FFF
#define INV_HSWFLAG_ENCODEMASK       0x007FFFF0
#define INV_HSWFLAG_ADDRSHFT         8
#define INV_HSWFLAG_DECODEMASK       			\
	(INV_HSWFLAG_ENCODEMASK << INV_HSWFLAG_ADDRSHFT)
#define INV_HSWFLAG_ADDR_ENCODE(x)   0xCC000000
#define INV_HSWFLAG_ADDR_DECODE(x)    			\
	(((unsigned int)x & INV_HSWFLAG_DECODEMASK) >> INV_HSWFLAG_ADDRSHFT)


#define INV_SUBA_HAGPBSTL        0x60000000
#define INV_SUBA_HAGPBSTH        0x61000000
#define INV_SUBA_HAGPBENDL       0x62000000
#define INV_SUBA_HAGPBENDH       0x63000000
#define INV_SUBA_HAGPBPL         0x64000000
#define INV_SUBA_HAGPBPID        0x65000000
#define INV_SUbA_HENBRANCH        0x69000000
#define INV_HAGPBPID_PAUSE               0x00000000
#define INV_HAGPBPID_JUMP                0x00000100
#define INV_HAGPBPID_STOP                0x00000200

#define INV_HAGPBpH_MASK                 0x000000FF
#define INV_HAGPBpH_SHFT                 0

#define INV_SUbA_HAGPBJUMPL      0x66000000
#define INV_SUbA_HAGPBJUMPH      0x67000000
#define INV_HAGPBJUMPH_MASK              0x000000FF
#define INV_HAGPBJUMPH_SHFT              0

#define INV_SUbA_HFTHRCM         0x68000000
#define INV_HFTHRCM_MASK                 0x003F0000
#define INV_HFTHRCM_SHFT                 16
#define INV_HFTHRCM_8                    0x00080000
#define INV_HFTHRCM_10                   0x000A0000
#define INV_HFTHRCM_18                   0x00120000
#define INV_HFTHRCM_24                   0x00180000
#define INV_HFTHRCM_32                   0x00200000

#define INV_HAGPBCLEAR                  0x00000008

#define INV_HRSTTRIG_RESTOREAGP          0x00000004
#define INV_HRSTTRIG_RESTOREALL          0x00000002
#define INV_HAGPBTRIG                    0x00000001

#define INV_PARASUBTYPE_MASK     0xff000000
#define INV_PARATYPE_MASK        0x00ff0000
#define INV_PARAOS_MASK          0x0000ff00
#define INV_PARAADR_MASK         0x000000ff
#define INV_PARASUbTYPE_SHIFT    24
#define INV_PARATYPE_SHIFT       16
#define INV_PARAOS_SHIFT         8
#define INV_PARAADR_SHIFT        0

#define INV_PARATYPE_VDATA       0x00000000
#define INV_PARATYPE_ATTR        0x00010000
#define INV_PARATYPE_TEX         0x00020000
#define INV_PARATYPE_PAL         0x00030000
#define INV_PARATYPE_FVF         0x00040000
#define INV_PARATYPE_PRECR       0x00100000
#define INV_PARATYPE_CR          0x00110000
#define INV_PARATYPE_CFG         0x00fe0000
#define INV_PARATYPE_DUMMY       0x00300000

#define INV_PARATYPE_TEX0         0x00000000
#define INV_PARATYPE_TEX1         0x00000001
#define INV_PARATYPE_TEX2         0x00000002
#define INV_PARATYPE_TEX3         0x00000003
#define INV_PARATYPE_TEX4         0x00000004
#define INV_PARATYPE_TEX5         0x00000005
#define INV_PARATYPE_TEX6         0x00000006
#define INV_PARATYPE_TEX7         0x00000007
#define INV_PARATYPE_GENERAL      0x000000fe
#define INV_PARATYPE_TEXSAMPLE    0x00000020

#define INV_HWBASL_MASK          0x00FFFFFF
#define INV_HWBASH_MASK          0xFF000000
#define INV_HWBASH_SHFT          24
#define INV_HWBASL(x)            ((unsigned int)(x) & INV_HWBASL_MASK)
#define INV_HWBASH(x)            ((unsigned int)(x) >> INV_HWBASH_SHFT)
#define INV_HWBAS256(x)          ((unsigned int)(x) >> 8)
#define INV_HWPIT32(x)           ((unsigned int)(x) >> 5)

/* Read Back Register Setting */
#define INV_SUBA_HSETRBGID       	 0x02000000
#define INV_HSETRBGID_CR                 0x00000000
#define INV_HSETRBGID_FE                 0x00000001
#define INV_HSETRBGID_PE                 0x00000002
#define INV_HSETRBGID_RC                 0x00000003
#define INV_HSETRBGID_PS                 0x00000004
#define INV_HSETRBGID_XE                 0x00000005
#define INV_HSETRBGID_BE                 0x00000006

/*Branch buffer Register Setting*/
#define INV_SUBA_HFCWBASL        0x01000000
#define INV_SUBA_HFCIDL          0x02000000
#define INV_SUBA_HFCTRIG         0x03000000
#define INV_SUBA_HFCBASH        0x04000000
#define INV_SUBA_HFCBASL2       0x05000000
#define INV_SUBA_HFCIDL2         0x06000000
#define INV_SUBA_HFCTRIG2        0x07000000

/*Fence ID Register Setting*/
#define INV_HFCTRG                       0x00100000
#define INV_HFCMODE_NOINTGEN             0x00000000
#define INV_HFCMODE_INTGEN               0x00020000
#define INV_HFCMODE_NOSYSWRITE           0x00000000
#define INV_HFCMODE_SYSWRITE             0x00040000
#define INV_HFCMODE_INTWAIT              0x00200000
#define INV_HFCMODE_DISABLEFENCEQUEUE    0x00400000
#define INV_HFCMODE_IMM                  0x00000000
#define INV_HFCMODE_IDLE                 0x00010000
#define INV_HFCIDL_MASK                  0x00FFFFFF

/*HW Interrupt Register Setting*/
#define INTERRUPT_CONTROL_REG 0x200
#define VBLANK_STATUS_REG1    0x200
/* turn on Interrupt control including TMDS */
#define INTERRUPT_ENABLE     0x80000000
#define CAPTURE0_ACTIVE_ENABLE  0x10000000
#define CAPTURE1_ACTIVE_ENABLE  0x01000000
#define CAPTURE0_INTSTATUS  0x00001000
#define CAPTURE1_INTSTATUS  0x00000100
#define HQV0_ACTIVE_ENABLE  0x02000000
#define HQV1_ACTIVE_ENABLE  0x00000200
#define HQV0_INSTATUS  0x00001000
#define HQV1_INSTATUS  0x00000400
#define DMA1_TRANS_INT      0x00800000
#define DMA1_TRANS_INTSTATUS  0x00000080
#define DMA0_TRANS_INT      0x00200000
#define DMA0_TRANS_INTSTATUS  0x00000020
#define IGA1_VSYNC_INT      0x00080000
#define IGA1_VSYNC_INTSTATUS  0x00000008
#define IGA2_VSYNC_INT      0x00020000
#define IGA2_VSYNC_INTSTATUS  0x00008000
#define LVDS_INTERRUPT      0x40000000  /* MM200 [30] */
#define LVDS_INT_STATUS     0x08000000  /* MM200 [27] */
#define TMDS_INTERRUPT      0x00010000  /* MM200 [16] */
#define TMDS_INT_STATUS     0x00000001  /* MM200 [0] */
#define IGA1VBLANKSTATUS    0x00000002  /* MM200 [1] */

/*Branch buffer Register Setting*/
#define INV_SubA_HFCWBasL        0x01000000
#define INV_SubA_HFCIDL          0x02000000
#define INV_SubA_HFCTrig         0x03000000
#define INV_SubA_HFCWBasH        0x04000000
#define INV_SubA_HFCWBasL2       0x05000000
#define INV_SubA_HFCIDL2         0x06000000
#define INV_SubA_HFCTrig2        0x07000000

/*Fence ID Register Setting*/
#define INV_HFCTrg                       0x00100000
#define INV_HFCMode_NoIntGen             0x00000000
#define INV_HFCMode_IntGen               0x00020000
#define INV_HFCMode_NoSysWrite           0x00000000
#define INV_HFCMode_SysWrite             0x00040000
#define INV_HFCMode_IntWait              0x00200000
#define INV_HFCMode_DisableFenceQueue    0x00400000
#define INV_HFCMode_IMM                  0x00000000
#define INV_HFCMode_Idle                 0x00010000
#define INV_HFCIDL_MASK                  0x00FFFFFF

/*VT410 new CR*/
#define NEW_BRANCHLOC_SL            0x00000000
#define NEW_BRANCHLOC_SF            0x00000001
#define VIDEORELATED                0x00000080

#define NEW_BRANCH_HIERARCHY_1ST    0x00000000
#define NEW_BRANCH_HIERARCHY_2ND    0x00000010
#define NEW_BRANCH_HIERARCHY_3RD    0x00000020

#define NEW_FENCEHeader2D           0xFE300000  //Header0~1
#define NEW_FENCEHeader3D           0xFE330000  //Header2~3
#define NEW_FENCEHeaderHQV          0xFE350000
#define NEW_FENCEHeaderDMA          0xFE370000
#define NEW_FENCEHeaderVD           0xFE3A0000

#define HINT_NEWCR_3D               0x00100000 //for checking command type
#define HINT_NEWCR_2D               0x00200000
#define HINT_NEWCR_DMA              0x00400000
#define HINT_NEWCR_HQV              0x01000000
#define HINT_NEWCR_VD               0x10000000
#define HINT_NEWCR_USERWAITALL      0xFFFABCDE // special wait all command from user mode
#define HINT_NEWCR_KERNELWAITALL    0xFFFEDCBA // special wait all command form kernel mode

#define NEW_FENCE_WAIT3D            0x00200000
#define NEW_FENCE_WAIT2D            0x00100000
#define NEW_FENCE_WAITHQV           0x000C0000
#define NEW_FENCE_WAITVD            0x00020000
#define NEW_FENCE_WAITDMA           0x0001E000
#define NEW_FENCE_WAITALL           0x003FE000  //wait 3D, 2D, DMA (no IGA flip, no scaler)

#define NEW_HENFCMode_IntGen        0x00000002
#define NEW_HENFCMode_SysWrite      0x00000004
#define NEW_FENCE_HEnFCIntWait      0x00000010
#define NEW_FENCE_HEnFCQSkip        0x00000020

#define FCFOLLOW_3D                 0x00000100
#define FCFOLLOW_2D                 0x00000200
#define FCFOLLOW_DMA                0x00000300
#define FCFOLLOW_SCALER             0x00000400
#define FCFOLLOW_HQV                0x00000500
#define FCFOLLOW_VD                 0x00000600

#define INV_ParaType_CR          0x00110000
#define AGP_REQ_FIFO_DEPTH  34
#define INV_ParaType_Dummy       0x00300000

#define INTERRPUT_ENABLE_MASK (DMA1_TRANS_INT | DMA0_TRANS_INT|\
	CAPTURE0_ACTIVE_ENABLE | CAPTURE1_ACTIVE_ENABLE | HQV0_ACTIVE_ENABLE |\
	HQV1_ACTIVE_ENABLE | IGA1_VSYNC_INT|IGA2_VSYNC_INT |\
	TMDS_INTERRUPT | LVDS_INTERRUPT)

#define INTERRPUT_STATUS_MASK (~(DMA1_TRANS_INTSTATUS  | DMA0_TRANS_INTSTATUS |\
	CAPTURE0_INTSTATUS | CAPTURE1_INTSTATUS | HQV0_INSTATUS|HQV1_INSTATUS |\
	IGA1_VSYNC_INTSTATUS | IGA2_VSYNC_INTSTATUS  | TMDS_INT_STATUS |\
	LVDS_INT_STATUS))

#define INTERRPUTCTLREG2    0x204
#define VBLANKSTATUSREG2    0x204
#define IGA2VBLANKSTATUS    0x00000004  /* MM204 [2] */
#define CR_INT_STATUS       0x00000020  /* MM204 [5] */

#define INTERRPUTCTLREG3    0x1280
/* MM1280[9], internal TMDS interrupt status = SR3E[6] */
#define INTERRPUT_TMDS_STATUS  0x200
/* MM1280[30], internal TMDS interrupt control = SR3E[7] */
#define INTERNAL_TMDS_INT_CONTROL 0x40000000

#define CHROME9_DMA_MR0 0xE00
#define CHROME9_DMA_MR1 0xE08
#define CHROME9_DMA_MR2 0xE10
#define CHROME9_DMA_MR3 0xE18

#define CHROME9_DMA_CSR0 0xE04
#define CHROME9_DMA_CSR1 0xE0c
#define CHROME9_DMA_CSR2 0xE14
#define CHROME9_DMA_CSR3 0xE1C

#define CHROME9_DMA_MARL0 0xE20
#define CHROME9_DMA_MARL1 0xE40
#define CHROME9_DMA_MARL2 0xE20
#define CHROME9_DMA_MARL3 0xE80

#define CHROME9_DMA_MARH0 0xE24
#define CHROME9_DMA_MARH1 0xE44
#define CHROME9_DMA_MARH2 0xE64
#define CHROME9_DMA_MARH3 0xE84

#define CHROME9_DMA_DAR0 0xE28
#define CHROME9_DMA_DAR1 0xE48
#define CHROME9_DMA_DAR2 0xE68
#define CHROME9_DMA_DAR3 0xE88

#define CHROME9_DMA_DQWCR0 0xE2C
#define CHROME9_DMA_DQWCR1 0xE4C
#define CHROME9_DMA_DQWCR2 0xE6C
#define CHROME9_DMA_DQWCR3 0xE8C

#define CHROME9_DMA_DPRL0 0xE34
#define CHROME9_DMA_DPRL1 0xE54
#define CHROME9_DMA_DPRL2 0xE74
#define CHROME9_DMA_DPRL3 0xE94

#define CHROME9_DMA_DPRH0 0xE38
#define CHROME9_DMA_DPRH1 0xE58
#define CHROME9_DMA_DPRH2 0xE78
#define CHROME9_DMA_DPRH3 0xE98

#define CHROME9_DMA_CSR_TS (1<<1)
#define CHROME9_DMA_CSR_DE (1<<0)
#define CHROME9_DMA_CSR_TD (1<<3)

#define CHROME9_DMA_MR_CM (1<<0)

#define CHROME9_DMA_DPRH_EB_CM (0xf<<16)

#define INV_CR_WAIT_ENGINE_IDLE           0x0000006C
#define INV_CR_WAIT_DMA0_IDLE             0x00040000
#define INV_CR_WAIT_DMA1_IDLE             0x00080000
#define INV_CR_WAIT_2D_IDLE				0x00400000
#define INV_CR_WAIT_3D_IDLE				0x00800000
#define INV_CR_WAIT_HQV0_IDLE			0x80000000
#define INV_CR_WAIT_HQV1_IDLE			0x40000000

#define DMA_CMD_TMP_BUFFER (20 * sizeof(uint32_t))

/*Location bit in HV adderss regeiste*/
#define HQV_LOC_SF 0x1
struct drm_clb_event_tag_info {
	unsigned int *linear_address;
	unsigned int *event_tag_linear_address;
	int   usage[NUMBER_OF_EVENT_TAGS];
	unsigned int   pid[NUMBER_OF_EVENT_TAGS];
};

static inline int is_agp_header(unsigned int data)
{
	switch (data & INV_AGPHEADER_MASK) {
	case INV_AGPHEADER0:
	case INV_AGPHEADER1:
	case INV_AGPHEADER2:
	case INV_AGPHEADER3:
	case INV_AGPHEADER4:
	case INV_AGPHEADER5:
	case INV_AGPHEADER6:
	case INV_AGPHEADER7:
	case INV_AGPHEADER9:
	case INV_AGPHEADERA:
		return 1;
	default:
		return 0;
	}
}

/*  Header0: 2D */
#define addcmdheader0_invi(p_cmd, qwcount)                       \
do {                                                               \
	while (((unsigned long)(p_cmd)) & 0xf) {                    \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER0;                             \
	*(p_cmd)++ = (qwcount);                                  \
	*(p_cmd)++ = 0;                                          \
	*(p_cmd)++ = (unsigned int)INV_HSWFLAG_ADDR_ENCODE(p_cmd);       \
} while (0)

/* Header1: 2D */
#define addcmdheader1_invi(p_cmd, dwAddr, dwcount)               \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {                    \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER1 | (dwAddr);                  \
	*(p_cmd)++ = (dwcount);                                  \
	*(p_cmd)++ = 0;                                          \
	*(p_cmd)++ = (unsigned int)INV_HSWFLAG_ADDR_ENCODE(p_cmd);       \
} while (0)

/* Header2: CR/3D */
#define addcmdheader2_invi(p_cmd, dwAddr, dwType)                \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {                        \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER2 | ((dwAddr)+4);              \
	*(p_cmd)++ = (dwAddr);                                   \
	*(p_cmd)++ = (dwType);                                   \
	*(p_cmd)++ = (unsigned int)INV_HSWFLAG_ADDR_ENCODE(p_cmd);       \
} while (0)

/* Header2: CR/3D with SW Flag */
#define addcmdheader2_swflag_invi(p_cmd, dwAddr, dwType, dwSWFlag)  \
do {                                                                  \
	/* 4 unsigned int align, insert NULL Command for padding */       \
	while (((unsigned long)(p_cmd)) & 0xF) {			   \
		*(p_cmd)++ = 0xCC000000;                            \
	}                                                          \
	*(p_cmd)++ = INV_AGPHEADER2 | ((dwAddr)+4);                 \
	*(p_cmd)++ = (dwAddr);                                      \
	*(p_cmd)++ = (dwType);                                      \
	*(p_cmd)++ = (dwSWFlag);                                    \
} while (0)


/* Header3: 3D */
#define addcmdheader3_invi(p_cmd, dwType, dwstart, dwcount)      \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {			\
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER3 | INV_REG_3D_TRANS;          \
	*(p_cmd)++ = (dwcount);                                  \
	*(p_cmd)++ = (dwType) | ((dwstart) & 0xFFFF);            \
	*(p_cmd)++ = (unsigned int)INV_HSWFLAG_ADDR_ENCODE(p_cmd);       \
} while (0)

/* Header3: 3D with SW Flag */
#define addcmdheader3_swflag_invi(p_cmd, dwType, dwstart, dwSWFlag, dwcount)  \
do {                                                                         \
	/* 4 unsigned int align, insert NULL Command for padding */          \
	while (((unsigned long)(p_cmd)) & 0xF) {                            \
		*(p_cmd)++ = 0xCC000000;                                      \
	}                                                                    \
	*(p_cmd)++ = INV_AGPHEADER3 | INV_REG_3D_TRANS;                       \
	*(p_cmd)++ = (dwcount);                                               \
	*(p_cmd)++ = (dwType) | ((dwstart) & 0xFFFF);                         \
	*(p_cmd)++ = (dwSWFlag);                                              \
} while (0)

/* Header4: DVD */
#define addcmdheader4_invi(p_cmd, dwAddr, dwcount, id)           \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */ \
	while (((unsigned long)(p_cmd)) & 0xF) {              \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER4 | (dwAddr);                  \
	*(p_cmd)++ = (dwcount);                                  \
	*(p_cmd)++ = (id);                                       \
	*(p_cmd)++ = 0;                                          \
} while (0)

/* Header5: DVD */
#define addcmdheader5_invi(p_cmd, dwQWcount, id)                 \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {              \
		*(p_cmd)++ = 0xCC000000;                                 \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER5;                             \
	*(p_cmd)++ = (dwQWcount);                                \
	*(p_cmd)++ = (id);                                       \
	*(p_cmd)++ = 0;                                          \
} while (0)

/* Header6: DEBUG */
#define addcmdheader6_invi(p_cmd)                                \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {                    \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER6;                             \
	*(p_cmd)++ = 0;                                          \
	*(p_cmd)++ = 0;                                          \
	*(p_cmd)++ = 0;                                          \
} while (0)

/* Header7: DMA */
#define addcmdheader7_invi(p_cmd, dwQWcount, id)                 \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long )(p_cmd)) & 0xF) {                    \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER7;                             \
	*(p_cmd)++ = (dwQWcount);                                \
	*(p_cmd)++ = (id);                                       \
	*(p_cmd)++ = 0;                                          \
} while (0)

/* Header82: Branch buffer */
#define addcmdheader82_invi(p_cmd, dwAddr, dwType);              \
do {                                                               \
	/* 4 unsigned int align, insert NULL Command for padding */    \
	while (((unsigned long)(p_cmd)) & 0xF) {                    \
		*(p_cmd)++ = 0xCC000000;                         \
	}                                                       \
	*(p_cmd)++ = INV_AGPHEADER82 | ((dwAddr)+4);             \
	*(p_cmd)++ = (dwAddr);                                   \
	*(p_cmd)++ = (dwType);                                   \
	*(p_cmd)++ = 0xCC000000;                                 \
} while (0)


#define ADDCmdHeader3X_INVI(p_cmd, dwAddr, dwFenceID, dwWaitEngine, dwFenceCfg, bVideoRelated)  \
{                                                                                              \
    /* 4 DWORD align, insert NULL Command for padding */                                       \
    while ( ((unsigned long)(p_cmd)) & 0xF )                                                        \
    {                                                                                          \
        *(p_cmd)++ = 0xCC000000;                                                                \
    }                                                                                          \
    if(bVideoRelated)                                                                          \
        *(p_cmd)++ = dwFenceCfg | VIDEORELATED;                                                 \
    else                                                                                       \
        *(p_cmd)++ = dwFenceCfg;                                                                \
    *(p_cmd)++ = dwWaitEngine | ((dwFenceID & 0x000000FF) << 24);                               \
    *(p_cmd)++ = (dwFenceID & 0xFFFFFF00) >> 8 |(dwAddr & 0x00000FF0) << 20;                    \
    *(p_cmd)++ = (dwAddr & 0xFFFFF000) >> 12;                                                   \
}

// Header4X: DMA branch command
#define ADDCmdHeader4X_INVI(p_cmd, dwAddr, dwSize, dwAGPHeader, dwHierarchy, bVideoRelated, location)    \
{                                                                                             \
    /* 4 DWORD align, insert NULL Command for padding */                                      \
    while ( ((unsigned long)(p_cmd)) & 0xF )                                                       \
    {                                                                                         \
        *(p_cmd)++ = 0xCC000000;                                                               \
    }                                                                                         \
    if(bVideoRelated)                                                                         \
       *(p_cmd)++ = dwAGPHeader | dwHierarchy | location | VIDEORELATED;               \
    else                                                                                      \
       *(p_cmd)++ = dwAGPHeader | dwHierarchy | location;                              \
    *(p_cmd)++ = (dwSize << 2);   /*((size(DW)) >> 2) <<4 ,[59~36] size in 2QW=128Bit*/        \
    *(p_cmd)++ = (dwAddr & 0xfffffff0);  /*[95~68], A[31:4]*/                                  \
    *(p_cmd)++ = 0;                                                                            \
}
#define add2dcmd_invi(p_cmd, dwAddr, dwCmd)                  \
do {                                                           \
	*(p_cmd)++ = (dwAddr);                               \
	*(p_cmd)++ = (dwCmd);                                \
} while (0)

#define addcmddata_invi(p_cmd, dwCmd)             (*(p_cmd)++ = (dwCmd))

#define addcmddatastream_invi(pCmdBuf, p_cmd, dwcount)       \
do {                                                           \
	memcpy((pCmdBuf), (p_cmd), ((dwcount)<<2));        \
	(pCmdBuf) += (dwcount);                             \
} while (0)

#endif
