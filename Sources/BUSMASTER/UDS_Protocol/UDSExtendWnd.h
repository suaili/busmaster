#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "UDS_Resource.h"
#include "UDS_Protocol.h"
#include "Utility/RadixEdit.h"
#include "UDSWnd_Defines.h"
#include "DataTypes/UDS_DataTypes.h"
#include "MsgBufFSE.h"

typedef struct{
	UINT ID;
	bool valid;
	UCHAR length;
	UCHAR data[100];
}UDSData;

typedef enum{
	FUNCTIONAL,
	PHYSICAL,
}UDSAddressType;

typedef enum{
	UPDATE_IDLE = 0,
	UPDATE_INIT = 1,
	WAIT_EXTENDED_SEESION_RES = 2,
	WAIT_READ_DID_RES = 3,
	WAIT_DTC_CONTROL_RES = 4,
	WAIT_COMM_CONTROL_RES = 5,
	WAIT_PROAMGRAM_SESSION_RES = 6,
	WAIT_REQUEST_SEED_RES = 7,
	WAIT_SEND_KEY_RES = 8,
	WAIT_ROUTINE_RES = 9,
	WAIT_REQUEST_DOWNLOAD_RES = 10,
	TRANSFER_DATA_RES = 11,
	WAIT_REQUEST_EXIT_RES = 12,
	WAIT_CHECK_DATA_RES = 13,
	WAIT_RESET_RES = 14,
	WAIT_WRITE_DID_RES = 15,
	WAIT_FLASH_DRVIER_RES = 16,
	WAIT_DEFAULT_SEESION_RES = 17,
	WAIT_CLEAR_DTC_RES = 18,
	UPDATE_SUCCESS = 19,
	WAIT_PRE_CONDITION_RES = 20,
	WAIT_ENABLE_PROGRAM_RES = 21,
	WAIT_DEFUALT_SEESION_RES = 22,
}UpdateStep;

typedef enum{
	_UPDATE_IDLE,
	_UPDATE_INIT,
	_WAIT_EXTENDED_SEESION,
	_WAIT_EXTENDED_SEESION_RES = 2,
	_WAIT_READ_DID_RES,
	_WAIT_DISABELE_DTC,
	_WAIT_DISABELE_DTC_RES = 4,
	_WAIT_DISABLE_COMM,
	_WAIT_DISABLE_COMM_RES = 5,
	_WAIT_PROAMGRAM_SESSION,
	_WAIT_PROAMGRAM_SESSION_RES = 6,
	_WAIT_REQUEST_SEED,
	_WAIT_REQUEST_SEED_RES = 7,
	_WAIT_SEND_KEY,
	_WAIT_SEND_KEY_RES = 8,
	_WAIT_ERASE,
	_WAIT_ROUTINE_RES = 9,
	_WAIT_REQUEST_DOWNLOAD,
	_WAIT_REQUEST_DOWNLOAD_RES = 10,
	_TRANSFER_DATA,
	_TRANSFER_DATA_RES = 11,
	_WAIT_REQUEST_EXIT,
	_WAIT_REQUEST_EXIT_RES = 12,
	_WAIT_CHECK_DATA,
	_WAIT_CHECK_DATA_RES = 13,
	_WAIT_RESET,
	_WAIT_RESET_RES = 14,
	_WAIT_WRITE_DID_RES,
	_WAIT_FLASH_DRVIER_RES,
	_UPDATE_END,
	_WAIT_DEFAULT_SEESION_RES,
	_WAIT_PRE_CONDITION_RES,
	_WAIT_ENABLE_PROGRAM_RES,
}UpdateStep_OLD;

typedef enum{
	TP_IDLE,
	TX_WAIT_FC,
	TX_WAIT_CF,
	RX_WAIT_FC,
	RX_WAIT_CF,
}TPStep;

typedef enum{
	DTC_OFF,
	DTC_ON,
}DtcOnOff;

typedef enum{
	TX_EN_RX_EN = 0,
	TX_DIS_RX_EN = 1,
	TX_EN_RX_DIS = 2,
	TX_DIS_RX_DIS = 3,
}CommControlType;

typedef enum{
	APP_MSG = 1,
	NM_MSG = 2,
	ALL_MSG = 3,
}CommMsgType;

typedef enum{
	FLASH_DRIVER,
	APP_CODE,
}CurrentTransfer;

#define MAX_UDS_LARGE_FRAME_LENGTH	4095

class CUDSExtendWnd :
	public CDialog
{
	DECLARE_DYNAMIC(CUDSExtendWnd);
public:
	CUDSExtendWnd(CWnd* pParent);
	virtual ~CUDSExtendWnd();

	CRadixEdit m_omUDSReqID;
	CRadixEdit m_omUDSResID;
	CRadixEdit m_omUDSFunID;
	CRadixEdit m_omBlockSize;
	CRadixEdit m_omSTmin;
	CComboBox m_omProjectDiag;
	CButton m_omLoaderDriver;
	CButton m_omLoaderApp;
	CButton m_omStartUpdate;
	CProgressCtrl m_omBlockPercent;
	CProgressCtrl m_omUpdatePercent;
	CRadixEdit m_omPromptBox;
	CString m_omPrompStr;
	CComboBox m_omUpdateChannel;

	UDSData UdsData[5];
	UCHAR LargeFrameBuf[MAX_UDS_LARGE_FRAME_LENGTH];
	USHORT LargeFrameLength;
	USHORT LargeFrameFinishLength;
	USHORT MaxBlockSize;
	UCHAR TransferDataIndex;
	UINT BlockDataFinish;
	CurrentTransfer m_CurrentTransfer;

	UCHAR AppBlockCount;
	UCHAR AppBlockDownloadIndex;
	UINT AppStartAddr[10];
	UINT AppBlockLengh[10];
	UINT AppBlockCRC[10];
	UCHAR AppBlockData[10][102400];

	UCHAR FlashDriverBlockCount;
	UCHAR FlashDriverDownloadIndex;
	UINT FlashDriverStartAddr[2];
	UINT FlashDriverBlockLengh[2];
	UINT FlashDriverBlockCRC[2];
	UCHAR FlashDriverBlockData[2][4096];

	UINT TotalDataOfByte;
	UINT TotalDriverOfByte;
	UINT CompleteDataOfByte;

	void vInitUDSConfigfFields();
	void ParseUDSData(UINT id, UCHAR arrayMsg[], UCHAR dataLen, UCHAR extendFrame, UCHAR rtr, UCHAR channel);
	void DataToUDSBuffer(UINT id, UINT length, UCHAR *data);
	void OnBnClickedSelectDriver(void);
	void OnBnClickedLoadApp(void);
	void OnBnClickedUpdate(void);
	void CalculateBlockCRC(UCHAR blockIndex , UCHAR codeType);
	int GetSystemTimeMS(void);
	void RefreshProgress(CurrentTransfer currentTransfer);

	enum { IDD = IDM_UDS_EXTEND };
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnUpdateMsgHandler(WPARAM, LPARAM);
	void UpdateDelay(int timeMS);
	CRITICAL_SECTION UpdateCs;	
	void SendSeveralCF(void);
	

private:
	msTXMSGDATA* psSendCanMsgUds = NULL;
	UINT PhyReqID;
	UINT PhyResID;
	UINT FunReqID;
	int SendSimpleCanMessage(void);
	void FillRequestID(UDSAddressType addrType);
	void SendSF(UDSAddressType addrType, UCHAR length, UCHAR* data);
	void SendFC(void);
	void SendFF(void);
	void SendCF(void);

private:
	UCHAR TX_STmin;
	UCHAR TX_BlockSize;
	UCHAR CFIndex;
	UCHAR TPTxBlockIndex;
	UCHAR RX_STmin;
	UCHAR RX_BlockSize;
	UCHAR TPRxCFIndex;
	UCHAR TPRxCFTimes;

public:	
	TPStep CurrTPStep;

	bool IsUpdateThreadExist;
	bool IsNeedSend3EMessage;
	bool IsNeedSendFC;

/*=========================R104 update fun protocol=============================*/
public:
	UpdateStep m_UpdateStep;
	UpdateStep_OLD m_eUpdateStep;
	void RequestExtendedSession(UDSAddressType addressType, bool suppress);
	void RequestDefaultSession(UDSAddressType addressType, bool suppress);
	void RequestProgramSession(UDSAddressType addressType, bool suppress);
	void ReqeustDTCControl(UDSAddressType addressType , DtcOnOff OnOff , bool suppress);
	void ReqeustCommControl(UDSAddressType addressType , CommControlType controlType  , CommMsgType msgType, bool suppress);
	void RequestSeed(UCHAR subFunction);
	void SendKey(UCHAR subFunction , UINT32 key , UCHAR keyLength);
	void RequestWriteFingerPrint_R104(void);
	void DownloadFlashDrvier_Request(void);
	void DownloadFlashDrvier_Transfer(void);
	void DownloadFlashDrvier_Exit(void);
	void EraseAppRom(void);
	void DownloadApp_Request(void);
	void DownloadApp_Transfer(void);
	void DownloadApp_Exit(void);
	void CheckProgramDependency_R104(void);
	void CheckProgramIntegrality_R104(void);
	void RequestResetECU(UDSAddressType addressType , UCHAR ResetType , bool suppress);
	void RequestClearDTC(UDSAddressType addressType);
	void SendTesterPresent(UDSAddressType addressType , bool suppress);
/*=========================R104 update fun protocol=============================*/
	
	void RequestReadDID(USHORT did);
	void RequestEnableProgram_HD10(void);
	void RequestCheckPreCondition_HD10(void);
	void RequestWriteBootFP_HD10(void);
	void RequestWriteAppFP_HD10(void);
	void RequestWriteDataFP_HD10(void);
	void RequestDownFlashDriver_HD10(void);
	void EraseAppRom_HD10(UCHAR index);
	void DownloadApp_Request_HD10(void);
	void CheckProgramIntegrality_HD10(void);
	void CheckProgramDependency_HD10(void);
	void CheckProgramIntegrality_EX50(void);
	void CheckProgramDependency_EX50(void);

	void EraseAppRom_F516(void);
	void CheckProgramDependency_F516(void);
	void CheckProgramIntegrality_F516(void);

	void EraseAppRom_SGMW(void);
	void RequestCheckPreCondition_SGMW(void);
	void CheckProgramIntegrality_SGMW(void);
	void CheckProgramDependency_SGMW(void);
	void RequestWriteFingerPrint_SGMW(void);

	void RequestCheckPreCondition_C301(void);
	void CheckProgramDependency_C301(void);
	void ActiveProgramming_C301(void);

	void RequestCheckPreCondition_MA501(void);
	void CheckProgramIntegrality_MA501(void);
	void CheckProgramDependency_MA501(void);
	void RequestWriteFingerPrint_MA501(void);
	void EraseAppRom_MA501(void);

	void RequestWriteFingerPrint_F507HEV(void);

	void RequestWriteRepairCode_FE57(void);
	void RequestWriteProgramDate_FE57(void);
	void CheckProgramIntegrality_FE57(void);
	void EraseAppRom_FE57(void);
	void CheckProgramDependency_FE57(void);

	void RequestWriteFingerPrint_H50(void);
	void RequestIntergrity_H50(void);
	void CheckProgramDependency_H50(void);

	void RequestRuntine(USHORT rid);

	void RequestIntergrity_N800BEV(void);
	void RequestWriteFingerPrint_N800BEV(void);
	void EraseAppRom_N800BEV(void);
	void CheckProgramDependency_N800BEV(void);

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP();
public:
	afx_msg void OnCbnSelchangeProject();
};

