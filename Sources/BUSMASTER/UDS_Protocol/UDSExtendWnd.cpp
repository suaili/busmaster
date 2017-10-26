#include "stdafx.h"
#include "UDSExtendWnd.h"
#include "BaseDIL_CAN.h"
#include "IBusMasterKernel.h"
#include "S19File.h"
#include "CodeCRC.h"
#include "SecurityAccess.h"


extern CUDSExtendWnd* omExtendWnd;

#if 1
/**********************************************************************************************************
 Function Name  :   TPLayerThread
 Input(s)       :   -
 Output         :   -
 Description    :   process TP transmit
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   16.05.2017
**********************************************************************************************************/
DWORD WINAPI TPLayerThread(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}


	while(1)
	{
		if(CurrentWnd->IsUpdateThreadExist)
		{

			if(CurrentWnd->IsNeedSendFC)
			{
				CurrentWnd->SendSeveralCF();
				CurrentWnd->IsNeedSendFC = false;
			}
			else if(CurrentWnd->IsNeedSend3EMessage)
			{
				CurrentWnd->SendTesterPresent(FUNCTIONAL , TRUE);
				CurrentWnd->IsNeedSend3EMessage = false;
			}
		}
		else
		{
			return (DWORD)0;
		}
	}
}
#endif

/**********************************************************************************************************
 Function Name  :   UpdateThread_N800BEV
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates JMC N800bev app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   09.10.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_N800BEV(LPVOID pVoid)
{	
	bool IsDataDownloaded = FALSE;
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
			CurrentWnd->AppBlockDownloadIndex = 0xFF;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else
		{
			int i , j;
			for(i = 0 ; i < 5 ; i ++)
			{
				if(CurrentWnd->UdsData[i].valid == true)
				{
					//EnterCriticalSection(&CurrentWnd->UpdateCs);
					#if 1
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					#endif
					
					
					if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x95)
							{
								CString str;
								str.Format("软件版本号: V%d.%d\r\n",CurrentWnd->UdsData[i].data[3],CurrentWnd->UdsData[i].data[4]);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);

								CurrentWnd->RequestReadDID(0xF170);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x93)
							{
								CString str;
								str.Format("硬件版本号: V%d.%d\r\n",CurrentWnd->UdsData[i].data[3],CurrentWnd->UdsData[i].data[4]);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF195);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x70)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("booltoader版本号 : %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestRuntine(0xFF02);
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xC5 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , FALSE);
							CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x85)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x68 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestReadDID(0xF193);
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x28)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}						
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x03);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::JMCCalculateKeyFBL(seed);
							CurrentWnd->SendKey(0x04, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x04)
						{
							CurrentWnd->FlashDriverDownloadIndex = 0;
							CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
							CurrentWnd->DownloadFlashDrvier_Request();
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x5A)
							{
								CurrentWnd->AppBlockDownloadIndex++;
								CurrentWnd->EraseAppRom_N800BEV();
								CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76 && CurrentWnd->UdsData[i].data[1] == CurrentWnd->TransferDataIndex -1)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{	
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->RequestIntergrity_N800BEV();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , FALSE);
									CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									IsDataDownloaded  = TRUE;
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									CurrentWnd->BlockDataFinish = 0;
									CurrentWnd->TransferDataIndex = 1;
									CurrentWnd->DownloadApp_Request();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xF0 && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->RequestWriteFingerPrint_N800BEV();
											CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_N800BEV();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex = 0;
											CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , FALSE);
									CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
									IsDataDownloaded  = TRUE;
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->m_omPrompStr += "正在下载......\r\n";
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->UpdateDelay(1000);
							CurrentWnd->RequestDefaultSession(FUNCTIONAL , FALSE);
							CurrentWnd->m_UpdateStep = WAIT_DEFUALT_SEESION_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x11)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DEFUALT_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->RequestClearDTC(PHYSICAL);
							CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{							
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					CurrentWnd->UdsData[i].valid = false;
					break;
				}
			}

		}
	}
}

/**********************************************************************************************************
 Function Name  :   UpdateThread_H50
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates H50/C30 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   28.09.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_H50(LPVOID pVoid)
{	
	bool IsDataDownloaded = FALSE;
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(PHYSICAL , FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else
		{
			int i , j;
			for(i = 0 ; i < 5 ; i ++)
			{
				if(CurrentWnd->UdsData[i].valid == true)
				{
					//EnterCriticalSection(&CurrentWnd->UpdateCs);
					#if 1
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					#endif
					
					
					if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x90)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("车架号 : %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
								CurrentWnd->UpdateDelay(10);
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF ,TRUE);
								CurrentWnd->UpdateDelay(10);
								CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);
								CurrentWnd->UpdateDelay(10);
								CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x95)
							{
								CString str;
								str.Format("软件版本号: V%d.%d\r\n",CurrentWnd->UdsData[i].data[3],CurrentWnd->UdsData[i].data[4]);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF190);
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0x31)
							{
								CurrentWnd->RequestReadDID(0xF190);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestSeed(0x01);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x09);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x09)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::BAICH50CalculateKeyLevel2(seed);
							CurrentWnd->SendKey(0x0A, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::BAICH50CalculateKeyLevel1(seed);
							CurrentWnd->SendKey(0x02, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x0A)
						{
							CurrentWnd->RequestWriteFingerPrint_H50();
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							
							CurrentWnd->m_omPrompStr += "正在擦除......\r\n";
							PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestReadDID(0xF195);
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76 && CurrentWnd->UdsData[i].data[1] == CurrentWnd->TransferDataIndex -1)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{	
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->RequestIntergrity_H50();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->BlockDataFinish = 0;
									CurrentWnd->TransferDataIndex = 1;
									CurrentWnd->DownloadApp_Request();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
										#if 1
											CurrentWnd->AppBlockDownloadIndex = 0;
											CurrentWnd->EraseAppRom_SGMW();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
											#else
											CurrentWnd->AppBlockDownloadIndex = 0;
											CurrentWnd->BlockDataFinish = 0;
											CurrentWnd->TransferDataIndex = 1;
											CurrentWnd->DownloadApp_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
											CurrentWnd->m_CurrentTransfer = APP_CODE;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
											#endif
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_H50();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->DownloadApp_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , FALSE);
									CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
									IsDataDownloaded  = TRUE;
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->m_omPrompStr += "正在下载......\r\n";
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->UpdateDelay(1000);
							CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
							CurrentWnd->UpdateDelay(10);
							CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
							CurrentWnd->UpdateDelay(10);
							CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON ,TRUE);
							CurrentWnd->UpdateDelay(10);
							CurrentWnd->RequestDefaultSession(PHYSICAL , FALSE);
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x11)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					CurrentWnd->UdsData[i].valid = false;
					break;
				}
			}

		}
	}
}

/**********************************************************************************************************
Function Name  :   UpdateThread_FE_57
Input(s)       :   -
Output         :   -
Description    :   a thread which updates FE_5B-7B app
Member of      :
Author(s)      :    ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_FE_57(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*)pVoid;
	if (CurrentWnd == nullptr)
	{
		return (DWORD)-1;
	}

	bool IsDownloadFinish = FALSE;
	
	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);

	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while (1)
	{		
		if (CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "\r\nupdate failed!!!\r\n";
			
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);

			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestReadDID(0xF193);
			CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "\r\nsuccess to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);

			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else
		{
			int i, j;
			for (i = 0; i < 5; i++)
			{
				if (CurrentWnd->UdsData[i].valid == true)
				{
					if (CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							if (IsDownloadFinish == TRUE)//after download
							{
								CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_EN_RX_EN, APP_MSG, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else//before download
							{
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_OFF, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xC5)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0x02)//before
							{
								CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_EN, APP_MSG, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0x01)//after
							{
								CurrentWnd->RequestDefaultSession(FUNCTIONAL, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_DEFUALT_SEESION_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x85)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x68)
						{
							if(CurrentWnd->UdsData[i].data[1] == TX_DIS_RX_EN)//before
							{
								CurrentWnd->RequestProgramSession(PHYSICAL, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else//after
							{
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_ON, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x28)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x11);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x93)
							{
								CurrentWnd->RequestReadDID(0xF180);
							}
							else if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x80)
							{
								CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x11)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::FE_5B_7B_SeedToKey(seed);
							CurrentWnd->SendKey(0x12, key, 4);
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x12)
						{
							CurrentWnd->RequestWriteRepairCode_FE57();
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x98)
							{
								CurrentWnd->RequestWriteProgramDate_FE57();
								CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x99)
							{
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_FE57();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x04)
								{
									if (CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->DownloadApp_Request();
										CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x04)
								{
									CurrentWnd->RequestResetECU(PHYSICAL, 0x01, FALSE);	
									CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0x02 && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x04)
								{
									if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if ((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->AppBlockDownloadIndex = 0;
											CurrentWnd->m_CurrentTransfer = APP_CODE;
											CurrentWnd->EraseAppRom_FE57();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if ((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_FE57();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom_FE57();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 2000, NULL);
							CurrentWnd->UpdateDelay(1000);
							CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
							IsDownloadFinish = TRUE;
							CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x11)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DEFUALT_SEESION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}					
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}

/**********************************************************************************************************
Function Name  :   UpdateThread_MA501
Input(s)       :   -
Output         :   -
Description    :   a thread which updates MA501 app
Member of      :
Author(s)      :    ukign.zhou
Date Created   :   08.08.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_F507_HEV(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*)pVoid;
	if (CurrentWnd == nullptr)
	{
		return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);

	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while (1)
	{		
		if (CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "\r\nupdate failed!!!\r\n";
			
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);

			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "\r\nsuccess to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);

			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if (CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestProgramSession(PHYSICAL, FALSE);
			CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
		}
		else if (CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_EN_RX_EN, ALL_MSG, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_ON, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestDefaultSession(FUNCTIONAL, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}
		else
		{
			int i, j;
			for (i = 0; i < 5; i++)
			{
				if (CurrentWnd->UdsData[i].valid == true)
				{
					if (CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						 if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_OFF, FALSE);//disable DTC detect
							CurrentWnd->UpdateDelay(50);
							CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_DIS, ALL_MSG, TRUE);
							CurrentWnd->UpdateDelay(50);
							CurrentWnd->RequestReadDID(0xF195);
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x09);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x95)
							{
								CurrentWnd->RequestReadDID(0xF184);
							}
							else if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->RequestReadDID(0xF170);
							}
							else if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x70)
							{
								CurrentWnd->RequestProgramSession(PHYSICAL, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x09)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::MA501SeedToKeyLevelFBL(seed);
							CurrentWnd->SendKey(0x0A, key, 4);
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x0A)
						{
							CurrentWnd->FlashDriverDownloadIndex = 0;
							CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
							CurrentWnd->DownloadFlashDrvier_Request();
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->AppBlockDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = APP_CODE;
								CurrentWnd->EraseAppRom_MA501();
								CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_MA501();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if (CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->DownloadApp_Request();
										CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									CurrentWnd->RequestResetECU(PHYSICAL, 0x01, FALSE);									
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 2000, NULL);
									CurrentWnd->UpdateDelay(1600);
									CurrentWnd->RequestDefaultSession(FUNCTIONAL, FALSE);
									CurrentWnd->m_UpdateStep = WAIT_DEFUALT_SEESION_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_OFF, FALSE);//disable DTC detect
									CurrentWnd->UpdateDelay(50);
									CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_DIS, ALL_MSG, TRUE);
									CurrentWnd->UpdateDelay(50);
									CurrentWnd->RequestReadDID(0xF193);
									CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);

								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
								
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xF0 && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if ((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->RequestWriteFingerPrint_F507HEV();
											CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if ((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_MA501();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom_MA501();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DEFUALT_SEESION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->RequestClearDTC(FUNCTIONAL);
							CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}					
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}

/**********************************************************************************************************
Function Name  :   UpdateThread_MA501
Input(s)       :   -
Output         :   -
Description    :   a thread which updates MA501 app
Member of      :
Author(s)      :    ukign.zhou
Date Created   :   08.08.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_MA501(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*)pVoid;
	if (CurrentWnd == nullptr)
	{
		return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);

	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while (1)
	{		
		if (CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "\r\nupdate failed!!!\r\n";
			
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);

			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
		}
		else if (CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "\r\nsuccess to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd, WM_UDS_UPDATE, 1, 0);

			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if (CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestProgramSession(PHYSICAL, FALSE);
			CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
		}
		else if (CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_EN_RX_EN, ALL_MSG, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_ON, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestDefaultSession(FUNCTIONAL, TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}
		else
		{
			int i, j;
			for (i = 0; i < 5; i++)
			{
				if (CurrentWnd->UdsData[i].valid == true)
				{
					if (CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						 if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestCheckPreCondition_MA501();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x03);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					#if 0
					else if (CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
					{
						CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_DIS, ALL_MSG, TRUE);//disable app rx and tx
						CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
						CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
					{
						CurrentWnd->RequestReadDID(0xF193);
						CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
						CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
					}
					#endif
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x93)
							{
								CurrentWnd->RequestReadDID(0xF1F9);
							}
							else if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0xF9)
							{
								CurrentWnd->RequestReadDID(0xF15A);
							}
							else if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x5A)
							{
								CurrentWnd->RequestProgramSession(PHYSICAL, FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::MA501SeedToKeyLevelFBL(seed);
							CurrentWnd->SendKey(0x04, key, 4);
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x04)
						{
							CurrentWnd->FlashDriverDownloadIndex = 0;
							CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
							CurrentWnd->DownloadFlashDrvier_Request();
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if (CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x5A)
							{
								CurrentWnd->AppBlockDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = APP_CODE;
								CurrentWnd->EraseAppRom_MA501();
								CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if (CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_MA501();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if (CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->DownloadApp_Request();
										CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									CurrentWnd->RequestResetECU(PHYSICAL, 0x01, FALSE);									
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 2000, NULL);
									CurrentWnd->UpdateDelay(1600);
									CurrentWnd->RequestDefaultSession(FUNCTIONAL, FALSE);
									CurrentWnd->m_UpdateStep = WAIT_DEFUALT_SEESION_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_OFF, FALSE);//disable DTC detect
									CurrentWnd->UpdateDelay(50);
									CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_DIS, ALL_MSG, TRUE);
									CurrentWnd->UpdateDelay(50);
									CurrentWnd->RequestReadDID(0xF193);
									CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);

								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
								
							}
							else if (CurrentWnd->UdsData[i].data[2] == 0xF0 && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if (CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if ((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->RequestWriteFingerPrint_MA501();
											CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else if (CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if ((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_MA501();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom_MA501();
											CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DEFUALT_SEESION_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->RequestClearDTC(FUNCTIONAL);
							CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE, 1000, NULL);
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if (CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if (CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if (CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}					
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}

/**********************************************************************************************************
 Function Name  :   UpdateThread_R104
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates r104 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   16.05.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_R104(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
		{
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , TRUE);//disable DTC detect
			CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
		{
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);//disable app rx and tx
								
			CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
			CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestDefaultSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}
		else
		{
			int i , j ;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{		
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					
					if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x21);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x21)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::R104CalculateKey(seed, 2);
							CurrentWnd->SendKey(0x22, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							CurrentWnd->RequestWriteFingerPrint_R104();
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								//CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramDependency_R104();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x01)
								{
									if(CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->DownloadApp_Request();
										CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
										CurrentWnd->m_CurrentTransfer = APP_CODE;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x01)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->CheckProgramIntegrality_R104();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramIntegrality_R104();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0x02 && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
								{
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->EraseAppRom();
									CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , TRUE);
									CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if (CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE, 5000, NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}

					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}

/**********************************************************************************************************
 Function Name  :   UpdateThread_C301
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates C301/B211/C211 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   05.07.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_C301(LPVOID pVoid)
{
	bool IsProgramEnabled = FALSE;
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(PHYSICAL , FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else
		{
			int i , j ;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{					
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					
					if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							if(IsProgramEnabled == FALSE)
							{
								CurrentWnd->RequestSeed(0x01);
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->RequestDefaultSession(FUNCTIONAL , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->RequestClearDTC(PHYSICAL);
								CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x01);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::C301CalculateKey(seed, 0);
							CurrentWnd->SendKey(0x02, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							if(IsProgramEnabled == TRUE)
							{
								CurrentWnd->RequestReadDID(0xF170);
							}
							else
							{
								CurrentWnd->RequestReadDID(0xF18A);
							}
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x8A)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("供应商编号: %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF089);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF0 && CurrentWnd->UdsData[i].data[2] == 0x89)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("硬件版本: %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF189);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x89)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("应用版本: %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestCheckPreCondition_C301();
								CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x70)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("FBL版本 : %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF171);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x71)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("FBL规范版本: %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestWriteFingerPrint_R104();//the fingerprint  of c301 is same as r104
								CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							if(CurrentWnd->UdsData[i].length == 3)
							{
								USHORT CrcBack = (CurrentWnd->UdsData[i].data[1] << 8) + CurrentWnd->UdsData[i].data[2];
								if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
								{
									if(CurrentWnd->FlashDriverBlockCRC[CurrentWnd->FlashDriverDownloadIndex] == CrcBack)
									{
										CurrentWnd->ActiveProgramming_C301();
										CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									if(CurrentWnd->AppBlockCRC[CurrentWnd->AppBlockDownloadIndex] == CrcBack)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_C301();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
								{
									CurrentWnd->DownloadApp_Request();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								CurrentWnd->RequestResetECU(PHYSICAL , 0x03 , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0x02 && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								CurrentWnd->AppBlockDownloadIndex = 0;
								CurrentWnd->EraseAppRom();
								CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0x02 && CurrentWnd->UdsData[i].data[3] == 0x03)
							{
								IsProgramEnabled = TRUE;
								CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);
								CurrentWnd->UpdateDelay(50);
								CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
					{
						 if(CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[1] == 0x03)
						 {
						 	CurrentWnd->UpdateDelay(200);
							CurrentWnd->RequestExtendedSession(PHYSICAL , FALSE);
							CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						 }
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}

/**********************************************************************************************************
 Function Name  :   UpdateThread_EX50
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates r104 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   16.05.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_EX50(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	#if 1
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
		{
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , TRUE);//disable DTC detect
			CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
		{
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);//disable app rx and tx
								
			CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
			CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}	
		else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestDefaultSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}
		else
		{
			int i , j ;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					
					//CurrentWnd->SendDlgItemMessage(IDC_CODE_DATA,WM_VSCROLL, SB_BOTTOM,0);
					
					//if(CurrentWnd->UdsData[i].data[0] == 0x7E && CurrentWnd->UdsData[i].data[1] == 0x00)
					//{
					//	continue;
					//}
					
					if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x09);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x09)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							//UINT key = 0x12345678;
							UINT key  = CSecurityAccess::Ex50CalcKey(seed);
							CurrentWnd->SendKey(0x0A, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x0A)
						{
							CurrentWnd->RequestWriteFingerPrint_R104();
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								//CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_EX50();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if(CurrentWnd->AppBlockDownloadIndex < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->DownloadApp_Request();
										CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
										CurrentWnd->m_CurrentTransfer = APP_CODE;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0x02 && CurrentWnd->UdsData[i].data[3] == 0x02)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->CheckProgramDependency_EX50();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_EX50();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->EraseAppRom();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										CurrentWnd->AppBlockDownloadIndex = 0;
										CurrentWnd->EraseAppRom();
										CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , TRUE);
										CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
	#endif
}

#if 0
/**********************************************************************************************************
 Function Name  :   UDSUpdateThreadFreescale
 Input(s)       :   -
 Output         :   -
 Description    :   when user click on "update" button,this thread be started.
 Member of      :   

 Author(s)      :    ukign.zhou
 Date Created   :   17.05.2016
**********************************************************************************************************/
DWORD WINAPI UpdateThread_HD10(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	USHORT ErasePercent;

	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->m_eUpdateStep = _UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);

	while(1)
	{
		if(CurrentWnd->m_eUpdateStep == _UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "response timeout!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			return 0;
		}
		else if(CurrentWnd->m_eUpdateStep == _UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , false);//step 2
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
			CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 3000, NULL);
			CurrentWnd->m_eUpdateStep = _WAIT_EXTENDED_SEESION_RES;
		}
		else if(CurrentWnd->m_eUpdateStep == _UPDATE_END)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			return (DWORD)0;//update end normal exit
		}
		else if(CurrentWnd->m_eUpdateStep == _WAIT_DISABELE_DTC)
		{
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL, DTC_OFF, TRUE); //step 9
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->m_eUpdateStep = _WAIT_DISABLE_COMM;
		}
		else if(CurrentWnd->m_eUpdateStep == _WAIT_DISABLE_COMM)
		{
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL, TX_DIS_RX_DIS, ALL_MSG, TRUE);//step 10
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestEnableProgram_HD10();						
			CurrentWnd->m_eUpdateStep = _WAIT_ENABLE_PROGRAM_RES;
		}
		else
		{
			int i , j ;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{
					CurrentWnd->UdsData[i].valid = false;
					
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);					

					if(CurrentWnd->m_eUpdateStep == _WAIT_DEFAULT_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);//step 2
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							CurrentWnd->m_eUpdateStep = _WAIT_EXTENDED_SEESION_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							CurrentWnd->RequestReadDID(0xF180);//step 5
							CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = 0;
							if(CurrentWnd->UdsData[i].data[1] == 0x01)
							{
								key  = CSecurityAccess::HD10CalculateKey(seed,1);
								CurrentWnd->SendKey(0x02, key , 2);								
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0x03)
							{
								key = CSecurityAccess::HD10CalculateKey(seed,3);
								CurrentWnd->SendKey(0x04, key , 2); 
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0x07)
							{
								key = CSecurityAccess::HD10CalculateKey(seed,7);
								CurrentWnd->SendKey(0x08, key , 2); //step 14
							}	
							CurrentWnd->m_eUpdateStep = _WAIT_SEND_KEY_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							if(CurrentWnd->UdsData[i].data[1] == 0x02)
							{
								CurrentWnd->RequestReadDID(0xF18A);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0x04)
							{
								CurrentWnd->RequestReadDID(0xF170);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0x08)
							{
								CurrentWnd->RequestWriteBootFP_HD10();//step 15
								CurrentWnd->m_eUpdateStep = _WAIT_WRITE_DID_RES;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x80)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF181);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x81)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF182);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x82)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF190);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x90)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);

								CurrentWnd->RequestReadDID(0xF183);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x83)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF184);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);

								CurrentWnd->RequestReadDID(0xF185);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x85)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);

								CurrentWnd->RequestReadDID(0xF187);
								CurrentWnd->m_eUpdateStep = _WAIT_READ_DID_RES;
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x87)
							{
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = '\r';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 1] = '\n';
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length + 2] = 0;							
								CurrentWnd->m_omPrompStr += (CString)(CurrentWnd->UdsData[i].data + 3);
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);

								CurrentWnd->RequestCheckPreCondition_HD10();
								CurrentWnd->m_eUpdateStep = _WAIT_PRE_CONDITION_RES;//step 9
							}
							else
							{
								CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_PRE_CONDITION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xE5 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->m_eUpdateStep = _WAIT_DISABELE_DTC;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0xA5 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_ENABLE_PROGRAM_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xE5 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
							CurrentWnd->m_eUpdateStep = _WAIT_PROAMGRAM_SESSION_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0xA5 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							CurrentWnd->RequestSeed(0x07);//step 14
							CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_SEED_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							if( CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x83)
							{
								CurrentWnd->RequestWriteAppFP_HD10();//step 15
							}
							else if( CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->RequestWriteDataFP_HD10();//step 15
							}
							else if( CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x85)
							{
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->BlockDataFinish = 0;
								CurrentWnd->TransferDataIndex = 1;
								CurrentWnd->RequestDownFlashDriver_HD10();//step 16
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_DOWNLOAD_RES;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_eUpdateStep == _WAIT_RESET_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x51)
						{
							CurrentWnd->KillTimer(ID_TIMER_UPDATE);
							CurrentWnd->m_eUpdateStep = _UPDATE_END;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x11 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if (CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
					{
						if(CurrentWnd->m_eUpdateStep == _WAIT_REQUEST_DOWNLOAD_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x74)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								//CurrentWnd->FlashDataBlockSize = ((CurrentWnd->UdsData[i].data[2]) << 8) +CurrentWnd->UdsData[i].data[3];
								CurrentWnd->MaxBlockSize = 0x402;
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_eUpdateStep = _TRANSFER_DATA_RES;
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34 && CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _TRANSFER_DATA_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x76)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								if (CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_EXIT_RES;
								}
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
							{
								if(CurrentWnd->UdsData[i].data[2] == 0x78)
								{
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _WAIT_REQUEST_EXIT_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x77)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								CurrentWnd->CheckProgramIntegrality_HD10();
								CurrentWnd->m_eUpdateStep = _WAIT_ROUTINE_RES;
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _WAIT_ROUTINE_RES)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xDF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if ((CurrentWnd->FlashDriverDownloadIndex + 1) < CurrentWnd->FlashDriverBlockCount)
								{
									CurrentWnd->FlashDriverDownloadIndex++;
									CurrentWnd->BlockDataFinish = 0;
									CurrentWnd->TransferDataIndex = 1;
									CurrentWnd->RequestDownFlashDriver_HD10();
									CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_DOWNLOAD_RES;
								}
								else
								{
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->EraseAppRom_HD10(1);
									ErasePercent = 0;
									CurrentWnd->m_eUpdateStep = _WAIT_ROUTINE_RES;
								}
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
							{
								if(CurrentWnd->UdsData[i].data[2] == 0x78)
								{
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								}
								else
								{
									CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;//checksum error
								}
							}
						}
					}
					else
					{
						if(CurrentWnd->m_eUpdateStep == _WAIT_ROUTINE_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x71)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
								{
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->BlockDataFinish = 0;
									CurrentWnd->TransferDataIndex = 1;
									CurrentWnd->DownloadApp_Request_HD10();
									CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_DOWNLOAD_RES;
								}
								else if(CurrentWnd->UdsData[i].data[2] == 0xDF && CurrentWnd->UdsData[i].data[3] == 0x01)
								{
									if ((CurrentWnd->AppBlockDownloadIndex + 1) < CurrentWnd->AppBlockCount)
									{
										CurrentWnd->AppBlockDownloadIndex++;
										CurrentWnd->BlockDataFinish = 0;
										CurrentWnd->TransferDataIndex = 1;
										CurrentWnd->DownloadApp_Request_HD10();
										CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_DOWNLOAD_RES;
									}
									else
									{
										CurrentWnd->CheckProgramDependency_HD10();//step 17
										CurrentWnd->m_eUpdateStep = _WAIT_ROUTINE_RES;
									}
								}
								else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
								{
									CurrentWnd->RequestResetECU(PHYSICAL, 0x01, FALSE);
									CurrentWnd->m_eUpdateStep = _WAIT_RESET_RES;
								}
								else
								{
									CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
							{
								if(CurrentWnd->UdsData[i].data[2] == 0x78)
								{
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
									ErasePercent += 125;
									CurrentWnd->m_omBlockPercent.SetPos(ErasePercent);
								}
								else
								{
									CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;//checksum error
								}
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _WAIT_REQUEST_DOWNLOAD_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x74)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								//CurrentWnd->FlashDataBlockSize = ((CurrentWnd->UdsData[i].data[2]) << 8) +CurrentWnd->UdsData[i].data[3];
								CurrentWnd->MaxBlockSize = 0x402;
								CurrentWnd->DownloadApp_Transfer();//step 16
								CurrentWnd->m_eUpdateStep = _TRANSFER_DATA_RES;
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34 && CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _TRANSFER_DATA_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x76)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								if (CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();//step 16
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();//step 16
									CurrentWnd->m_eUpdateStep = _WAIT_REQUEST_EXIT_RES;
								}
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
							{
								if(CurrentWnd->UdsData[i].data[2] == 0x78)
								{
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								}
								else
								{
									CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _WAIT_REQUEST_EXIT_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x77)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
								CurrentWnd->CheckProgramIntegrality_HD10();//step 17
								CurrentWnd->m_eUpdateStep = _WAIT_ROUTINE_RES;
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34 && CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->m_eUpdateStep == _WAIT_RESET_RES)
						{
							if(CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[0] == 0x01)
							{	
								CurrentWnd->KillTimer(ID_TIMER_UPDATE);
								CurrentWnd->m_eUpdateStep = _UPDATE_END;
							}
							else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34 && CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
				}
			}

		}
	}
}
#else

/**********************************************************************************************************
 Function Name  :   UpdateThread_HD10
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates r104 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   05.06.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_HD10(LPVOID pVoid)
{
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(PHYSICAL , FALSE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
		{
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);//disable app rx and tx
								
			CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestEnableProgram_HD10();		
			CurrentWnd->m_UpdateStep = WAIT_ENABLE_PROGRAM_RES;
			
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}		
		else
		{
			int i;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{
					#if 1
					CString str;
					UINT j;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					#endif
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					
					if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestReadDID(0xF180);
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x80)
							{								
								CurrentWnd->RequestReadDID(0xF181);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x81)
							{
								CurrentWnd->RequestReadDID(0xF182);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x82)
							{
								CurrentWnd->RequestReadDID(0xF190);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x90)
							{
								CurrentWnd->RequestReadDID(0xF183);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x83)
							{
								CurrentWnd->RequestReadDID(0xF184);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->RequestReadDID(0xF185);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x85)
							{
								CurrentWnd->RequestReadDID(0xF187);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x87)
							{
								CurrentWnd->RequestCheckPreCondition_HD10();
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								CurrentWnd->m_UpdateStep = WAIT_PRE_CONDITION_RES;//step 9
							}
							else
							{
								CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PRE_CONDITION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xE5 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
							CurrentWnd->UpdateDelay(50);
							CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , TRUE);
							CurrentWnd->UpdateDelay(50);
							CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0xA5 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ENABLE_PROGRAM_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xE5 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
							CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0xA5 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_eUpdateStep = _UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x07);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x07)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::HD10CalculateKey(seed,7);
							CurrentWnd->SendKey(0x08, key , 2);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x08)
						{
							CurrentWnd->RequestWriteBootFP_HD10();//step 15
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x83)
							{
								CurrentWnd->RequestWriteAppFP_HD10();//step 15
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->RequestWriteDataFP_HD10();//step 15
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x85)
							{
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->RequestDownFlashDriver_HD10();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74)
						{
							CurrentWnd->MaxBlockSize = 0x402;
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_HD10();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x01)
								{
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->DownloadApp_Request_HD10();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xDF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
								{
									if(CurrentWnd->UdsData[i].data[4] == CurrentWnd->FlashDriverDownloadIndex)
									{
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->EraseAppRom_HD10(1);
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->RequestDownFlashDriver_HD10();	
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;	
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									if(CurrentWnd->UdsData[i].data[4] == CurrentWnd->AppBlockDownloadIndex)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_HD10();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->DownloadApp_Request_HD10();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;							
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , TRUE);
									CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}
#endif


/**********************************************************************************************************
 Function Name  :   UpdateThread_R104
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates r104 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   16.05.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_F516(LPVOID pVoid)
{
	UCHAR RoutineType;//0xAA: 0x55:
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
		{
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , TRUE);//disable DTC detect
			CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
		{
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , TRUE);//disable app rx and tx
								
			CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
			CurrentWnd->UpdateDelay(50);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
		{
			CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
			CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
		{
			CurrentWnd->UpdateDelay(200);
			CurrentWnd->RequestExtendedSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_EN_RX_EN , ALL_MSG , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_ON , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestDefaultSession(FUNCTIONAL , TRUE);
			CurrentWnd->UpdateDelay(50);
			CurrentWnd->RequestClearDTC(PHYSICAL);
			CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
		}
		else
		{
			int i , j ;
			for(i = 0 ; i < 5 ; i ++)
			{
				//EnterCriticalSection(&CurrentWnd->UpdateCs);
				if(CurrentWnd->UdsData[i].valid == true)
				{					
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					
					//CurrentWnd->SendDlgItemMessage(IDC_CODE_DATA,WM_VSCROLL, SB_BOTTOM,0);
					
					//if(CurrentWnd->UdsData[i].data[0] == 0x7E && CurrentWnd->UdsData[i].data[1] == 0x00)
					//{
					//	continue;
					//}
					
					if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x09);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x09)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::F516SeedToKeyLevelFBL(seed);
							CurrentWnd->SendKey(0x0A, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x0A)
						{
							CurrentWnd->EraseAppRom_F516();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					#if 0
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								//CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
								CurrentWnd->FlashDriverDownloadIndex = 0;
								CurrentWnd->m_CurrentTransfer = FLASH_DRIVER;
								CurrentWnd->DownloadFlashDrvier_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					#endif
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								#if 0
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								#endif
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{	
								#if 0
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								#endif
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] != 0x78)
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
							else
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramDependency_F516();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							RoutineType = 0xAA;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->BlockDataFinish = 0;
									CurrentWnd->TransferDataIndex = 1;
									CurrentWnd->DownloadApp_Request();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
									CurrentWnd->m_CurrentTransfer = APP_CODE;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										#if 0
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->CheckProgramIntegrality_R104();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										#endif
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if(RoutineType == 0xAA)
										{
											if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
											{
												CurrentWnd->CheckProgramIntegrality_F516();
												CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
												CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
												RoutineType = 0x55;
											}
											else
											{
												CurrentWnd->AppBlockDownloadIndex++;
												CurrentWnd->DownloadApp_Request();
												CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
												CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
											}
										}
										else if(RoutineType == 0x55)
										{
											CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , TRUE);
											CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->m_UpdateStep = UPDATE_IDLE;
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							#if 0
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
								{
									#if 0
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->EraseAppRom();
									CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									#endif
								}
								else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									if(CurrentWnd->UdsData[i].data[4] == 0x00)
									{
										CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , TRUE);
										CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							#endif
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}


/**********************************************************************************************************
 Function Name  :   UpdateThread_SGMW
 Input(s)       :   -
 Output         :   -
 Description    :   a thread which updates r104 app
 Member of      :   
 Author(s)      :    ukign.zhou
 Date Created   :   16.05.2017
**********************************************************************************************************/
DWORD WINAPI UpdateThread_SGMW(LPVOID pVoid)
{	
	bool IsDataDownloaded = FALSE;
	CUDSExtendWnd* CurrentWnd = (CUDSExtendWnd*) pVoid;
	if (CurrentWnd == nullptr)
	{
	    return (DWORD)-1;
	}

	CurrentWnd->IsUpdateThreadExist = true;
	CurrentWnd->m_UpdateStep = UPDATE_INIT;
	CurrentWnd->m_omStartUpdate.EnableWindow(FALSE);
	
	CurrentWnd->SetTimer(ID_TIMER_S3_SERVER, 1000, NULL);
	while(1)
	{
		if(CurrentWnd->m_UpdateStep == UPDATE_IDLE)
		{
			CurrentWnd->m_omPrompStr += "update failed!!!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return 0;
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_INIT)
		{
			//CurrentWnd->RequestDefaultSession(PHYSICAL , FALSE);
			//CurrentWnd->m_UpdateStep = WAIT_DEFAULT_SEESION_RES;
			CurrentWnd->RequestReadDID(0xF190);
			CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
			CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
		}
		else if(CurrentWnd->m_UpdateStep == UPDATE_SUCCESS)
		{
			CurrentWnd->m_omPrompStr += "success to down flash code!\r\n";
			PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
			CurrentWnd->m_omStartUpdate.EnableWindow(true);
			CurrentWnd->KillTimer(ID_TIMER_S3_SERVER);
			CurrentWnd->IsUpdateThreadExist = false;
			return (DWORD)0;//update end normal exit
		}
		else
		{
			int i , j;
			for(i = 0 ; i < 5 ; i ++)
			{
				if(CurrentWnd->UdsData[i].valid == true)
				{
					//EnterCriticalSection(&CurrentWnd->UpdateCs);
					#if 1
					CString str;
					str.Format("Rx : ID %x data",CurrentWnd->UdsData[i].ID);
					CurrentWnd->m_omPrompStr += str;
					for(j = 0 ; j < CurrentWnd->UdsData[i].length; j ++)
					{
						str.Format(" %x",CurrentWnd->UdsData[i].data[j]);
						CurrentWnd->m_omPrompStr += str;
					}
					
					CurrentWnd->m_omPrompStr += "\r\n";
					PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
					#endif
					
					if(CurrentWnd->m_UpdateStep == WAIT_DEFAULT_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->UpdateDelay(50);
							CurrentWnd->RequestReadDID(0xF190);
							CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_READ_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x62)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x90)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("车架号 : %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF18A);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x8A)
							{
								CString str;
								CurrentWnd->UdsData[i].data[CurrentWnd->UdsData[i].length] = 0;
								str.Format("供应商编号 : %s\r\n",CurrentWnd->UdsData[i].data + 3);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF193);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x93)
							{
								CString str;
								str.Format("硬件版本号: V%d.%d\r\n",CurrentWnd->UdsData[i].data[3],CurrentWnd->UdsData[i].data[4]);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF195);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x95)
							{
								CString str;
								str.Format("软件版本号: V%d.%d\r\n",CurrentWnd->UdsData[i].data[3],CurrentWnd->UdsData[i].data[4]);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								
								CurrentWnd->RequestReadDID(0xF1CB);
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0xCB)
							{
								UINT32 PartNumber = (UINT32)(CurrentWnd->UdsData[i].data[3] << 24) + (UINT32)(CurrentWnd->UdsData[i].data[4] << 16) + (UINT32)(CurrentWnd->UdsData[i].data[5] << 8) + CurrentWnd->UdsData[i].data[6];
								CString str;
								str.Format("零件号: %d\r\n",PartNumber);
								CurrentWnd->m_omPrompStr += str;
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								if(IsDataDownloaded == FALSE)
								{
									CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
									CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_SUCCESS;
								}
							}
							else if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								if(IsDataDownloaded == TRUE)
								{
									CString str;
									str.Format("\r\n下载完成\r\n下载设备: %c%c%c%c%c\r\n",CurrentWnd->UdsData[i].data[3]
										, CurrentWnd->UdsData[i].data[4] 
										, CurrentWnd->UdsData[i].data[5] 
										, CurrentWnd->UdsData[i].data[6]
										, CurrentWnd->UdsData[i].data[7]);
									CurrentWnd->m_omPrompStr += str;

									str.Format("下载日期: %d%d年%d月%d日\r\n",CurrentWnd->UdsData[i].data[8]
										, CurrentWnd->UdsData[i].data[9] 
										, CurrentWnd->UdsData[i].data[10] 
										, CurrentWnd->UdsData[i].data[11]);
									CurrentWnd->m_omPrompStr += str;
									PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
									
									CurrentWnd->RequestReadDID(0xF190);
								}
							}
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x22)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_EXTENDED_SEESION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							#if 0
							CurrentWnd->RequestCheckPreCondition_SGMW();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							#else
							if(IsDataDownloaded == FALSE)
							{
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->RequestSeed(0x01);
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							#endif
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_DTC_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0xC5 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->ReqeustCommControl(FUNCTIONAL , TX_DIS_RX_DIS , ALL_MSG , FALSE);//disable app rx and tx		
							CurrentWnd->m_UpdateStep = WAIT_COMM_CONTROL_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x85)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_COMM_CONTROL_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x68 && CurrentWnd->UdsData[i].data[1] == 0x03)
						{
							CurrentWnd->RequestProgramSession(PHYSICAL , FALSE);	
							CurrentWnd->m_UpdateStep = WAIT_PROAMGRAM_SESSION_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x28)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_PROAMGRAM_SESSION_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x50 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestSeed(0x05);
							CurrentWnd->m_UpdateStep = WAIT_REQUEST_SEED_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x10)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_SEED_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x05)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::CN210CalculateKey(3 , seed);
							CurrentWnd->SendKey(0x06, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							UINT seed = (CurrentWnd->UdsData[i].data[2] << 24) + (CurrentWnd->UdsData[i].data[3] << 16) + (CurrentWnd->UdsData[i].data[4] << 8) + CurrentWnd->UdsData[i].data[5];
							UINT key = CSecurityAccess::CN210CalculateKey(1 , seed);
							CurrentWnd->SendKey(0x02, key , 4);		
							CurrentWnd->m_UpdateStep = WAIT_SEND_KEY_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_SEND_KEY_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x06)
						{
							CurrentWnd->EraseAppRom_SGMW();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							
							CurrentWnd->m_omPrompStr += "正在擦除......\r\n";
							PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x67 && CurrentWnd->UdsData[i].data[1] == 0x02)
						{
							CurrentWnd->RequestWriteFingerPrint_SGMW();
							CurrentWnd->m_UpdateStep = WAIT_WRITE_DID_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x27) 
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_WRITE_DID_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x6E)
						{
							if(CurrentWnd->UdsData[i].data[1] == 0xF1 && CurrentWnd->UdsData[i].data[2] == 0x84)
							{
								CurrentWnd->RequestReadDID(0xF184);
								CurrentWnd->m_UpdateStep = WAIT_READ_DID_RES;								
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x2E)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_DOWNLOAD_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x74 && CurrentWnd->UdsData[i].data[1] == 0x20)
						{
							CurrentWnd->MaxBlockSize = (CurrentWnd->UdsData[i].data[2] << 8) + CurrentWnd->UdsData[i].data[3];
							CurrentWnd->TransferDataIndex = 1;
							CurrentWnd->BlockDataFinish = 0;
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{
								#if 0
								CurrentWnd->DownloadFlashDrvier_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								#endif
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								CurrentWnd->DownloadApp_Transfer();
								CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x34)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}	
					else if(CurrentWnd->m_UpdateStep == TRANSFER_DATA_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x76 && CurrentWnd->UdsData[i].data[1] == CurrentWnd->TransferDataIndex -1)
						{
							if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
							{	
								#if 0
								if(CurrentWnd->BlockDataFinish < CurrentWnd->FlashDriverBlockLengh[CurrentWnd->FlashDriverDownloadIndex])
								{
									CurrentWnd->DownloadFlashDrvier_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadFlashDrvier_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								#endif
							}
							else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
							{
								if(CurrentWnd->BlockDataFinish < CurrentWnd->AppBlockLengh[CurrentWnd->AppBlockDownloadIndex])
								{
									CurrentWnd->DownloadApp_Transfer();
									CurrentWnd->m_UpdateStep = TRANSFER_DATA_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
								else
								{
									CurrentWnd->DownloadApp_Exit();
									CurrentWnd->m_UpdateStep = WAIT_REQUEST_EXIT_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
								}
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x36)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_REQUEST_EXIT_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x77)
						{
							CurrentWnd->CheckProgramIntegrality_SGMW();
							CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x37 && CurrentWnd->UdsData[i].data[2] != 0x78)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_ROUTINE_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x71)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x00 && CurrentWnd->UdsData[i].length == 4)
							{
								CurrentWnd->AppBlockDownloadIndex = 0;
								CurrentWnd->BlockDataFinish = 0;
								CurrentWnd->TransferDataIndex = 1;
								CurrentWnd->DownloadApp_Request();
								CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;
								CurrentWnd->m_CurrentTransfer = APP_CODE;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);								
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xDF && CurrentWnd->UdsData[i].data[3] == 0xFF)
							{
								if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
								{
									if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
									{
										#if 0
										if((CurrentWnd->FlashDriverDownloadIndex + 1) >= CurrentWnd->FlashDriverBlockCount)
										{
											CurrentWnd->CheckProgramIntegrality_R104();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->FlashDriverDownloadIndex++;
											CurrentWnd->DownloadFlashDrvier_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										#endif
									}
									else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
									{
										if((CurrentWnd->AppBlockDownloadIndex + 1) >= CurrentWnd->AppBlockCount)
										{
											CurrentWnd->CheckProgramDependency_SGMW();
											CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
										else
										{
											CurrentWnd->AppBlockDownloadIndex++;
											CurrentWnd->DownloadApp_Request();
											CurrentWnd->m_UpdateStep = WAIT_REQUEST_DOWNLOAD_RES;								
											CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										}
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							else if(CurrentWnd->UdsData[i].data[2] == 0xFF && CurrentWnd->UdsData[i].data[3] == 0x01)
							{
								if(CurrentWnd->m_CurrentTransfer == FLASH_DRIVER)
								{
									#if 0
									CurrentWnd->AppBlockDownloadIndex = 0;
									CurrentWnd->EraseAppRom();
									CurrentWnd->m_UpdateStep = WAIT_ROUTINE_RES;
									CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
									#endif
								}
								else if(CurrentWnd->m_CurrentTransfer == APP_CODE)
								{
									if(CurrentWnd->UdsData[i].data[4] == 0x00 && CurrentWnd->UdsData[i].length == 5)
									{
										CurrentWnd->RequestResetECU(PHYSICAL , 0x01 , FALSE);
										CurrentWnd->m_UpdateStep = WAIT_RESET_RES;
										CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
										IsDataDownloaded  = TRUE;
									}
									else
									{
										CurrentWnd->m_UpdateStep = UPDATE_IDLE;
									}
								}
								else
								{
									CurrentWnd->m_UpdateStep = UPDATE_IDLE;
								}
							}
							#if 0
							else if(CurrentWnd->UdsData[i].data[2] == 0xDF && CurrentWnd->UdsData[i].data[3] == 0x00)
							{
								CurrentWnd->ReqeustDTCControl(FUNCTIONAL , DTC_OFF , FALSE);
								CurrentWnd->m_UpdateStep = WAIT_DTC_CONTROL_RES;
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
							}
							#endif
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x31)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->m_omPrompStr += "正在下载......\r\n";
								PostMessage(CurrentWnd->m_hWnd , WM_UDS_UPDATE , 1 , 0);
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}	
					else if(CurrentWnd->m_UpdateStep == WAIT_RESET_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x51 && CurrentWnd->UdsData[i].data[1] == 0x01)
						{
							CurrentWnd->UpdateDelay(500);
							CurrentWnd->RequestClearDTC(FUNCTIONAL);
							CurrentWnd->m_UpdateStep = WAIT_CLEAR_DTC_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x11)
						{
							if(CurrentWnd->UdsData[i].data[2] == 0x78)
							{
								CurrentWnd->SetTimer(ID_TIMER_UPDATE ,5000 , NULL);
							}
							else
							{
								CurrentWnd->m_UpdateStep = UPDATE_IDLE;
							}
						}
					}
					else if(CurrentWnd->m_UpdateStep == WAIT_CLEAR_DTC_RES)
					{
						if(CurrentWnd->UdsData[i].data[0] == 0x54)
						{
							CurrentWnd->RequestExtendedSession(FUNCTIONAL, FALSE);
							CurrentWnd->m_UpdateStep = WAIT_EXTENDED_SEESION_RES;
							CurrentWnd->SetTimer(ID_TIMER_UPDATE ,1000 , NULL);
						}
						else if(CurrentWnd->UdsData[i].data[0] == 0x7F && CurrentWnd->UdsData[i].data[1] == 0x14)
						{
							CurrentWnd->m_UpdateStep = UPDATE_IDLE;
						}
					}
					//LeaveCriticalSection(&CurrentWnd->UpdateCs);
					CurrentWnd->UdsData[i].valid = false;
				}
			}

		}
	}
}


static CBaseDIL_CAN* l_pouDIL_CAN_Interface;
static DWORD l_dwClientID = 0;

IMPLEMENT_DYNAMIC(CUDSExtendWnd, CDialog)
CUDSExtendWnd::CUDSExtendWnd(CWnd* pParent) : CDialog(CUDSExtendWnd::IDD, pParent)
{
}

CUDSExtendWnd::~CUDSExtendWnd()
{
}

BOOL CUDSExtendWnd::OnInitDialog()
{
	CDialog::OnInitDialog();
	/* Get CAN DIL interface */
	DIL_GetInterface(CAN, (void**)&l_pouDIL_CAN_Interface);
	l_pouDIL_CAN_Interface->DILC_RegisterClient(TRUE, l_dwClientID, _("CAN_MONITOR"));

	
	if (psSendCanMsgUds ==NULL)
	{
		psSendCanMsgUds  = new msTXMSGDATA;
		
		psSendCanMsgUds->m_unCount = 1;
		psSendCanMsgUds->m_psTxMsg = new STCAN_MSG[1];

		psSendCanMsgUds->m_psTxMsg->m_ucRTR = FALSE;
		//psSendCanMsgUds->m_psTxMsg->m_bCANFD = FALSE;
	}
	
	vInitUDSConfigfFields();
	
	CurrTPStep = TP_IDLE;
	m_UpdateStep = UPDATE_IDLE;
	AppBlockCount = 0;
	FlashDriverBlockCount = 0;
	IsUpdateThreadExist = false;
	IsNeedSend3EMessage = false;
	IsNeedSendFC = false;
	
	UdsData[0].valid = false;
	UdsData[1].valid = false;
	UdsData[2].valid = false;
	UdsData[3].valid = false;
	UdsData[4].valid = false;
	
	InitializeCriticalSection(&UpdateCs);
	
	return TRUE;
}

void CUDSExtendWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REQ_ID, m_omUDSReqID);
	DDX_Control(pDX, IDC_RES_ID, m_omUDSResID); 
	DDX_Control(pDX, IDC_FUN_ID, m_omUDSFunID);
	DDX_Control(pDX, IDC_BLOCK_SIZE, m_omBlockSize); 
	DDX_Control(pDX, IDC_ST_MIN, m_omSTmin);
	DDX_Control(pDX, IDC_PROJECT, m_omProjectDiag);
	DDX_Control(pDX, ID_SELECT_DRIVER, m_omLoaderDriver);
	DDX_Control(pDX, ID_LOAD_APP, m_omLoaderApp);
	DDX_Control(pDX, ID_UDS_UPDATE, m_omStartUpdate);
	DDX_Control(pDX, IDC_BLOCK_PERCENT, m_omBlockPercent);
	DDX_Control(pDX, IDC_UPDATE_PERCENT, m_omUpdatePercent);
	DDX_Control(pDX, IDC_PROMPT_BOX, m_omPromptBox);
	DDX_Control(pDX, IDC_UPDATE_CHANNEL, m_omUpdateChannel);
	DDX_Text(pDX, IDC_PROMPT_BOX, m_omPrompStr);
}

BEGIN_MESSAGE_MAP(CUDSExtendWnd, CDialog)
	ON_BN_CLICKED(ID_SELECT_DRIVER, OnBnClickedSelectDriver)
	ON_BN_CLICKED(ID_LOAD_APP, OnBnClickedLoadApp)
	ON_BN_CLICKED(ID_UDS_UPDATE, OnBnClickedUpdate)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_MESSAGE(WM_UDS_UPDATE, OnUpdateMsgHandler)
	ON_CBN_SELCHANGE(IDC_PROJECT, &CUDSExtendWnd::OnCbnSelchangeProject)
END_MESSAGE_MAP()

void CUDSExtendWnd::vInitUDSConfigfFields()
{
	m_omUDSReqID.vSetBase(BASE_HEXADECIMAL);
	m_omUDSResID.vSetBase(BASE_HEXADECIMAL);
	m_omUDSFunID.vSetBase(BASE_HEXADECIMAL);
	m_omBlockSize.vSetBase(BASE_DECIMAL);
	m_omSTmin.vSetBase(BASE_DECIMAL);

	m_omUDSReqID.LimitText(8);
	m_omUDSResID.LimitText(8);
	m_omUDSFunID.LimitText(8);
	m_omBlockSize.LimitText(3);
	m_omSTmin.LimitText(3);

	m_omProjectDiag.InsertString(0, "SGMW");
	m_omProjectDiag.InsertString(1, "C301/B211/C211");
	m_omProjectDiag.InsertString(2, "R104");
	m_omProjectDiag.InsertString(3, "HD10");
	m_omProjectDiag.InsertString(4, "EX50");
	m_omProjectDiag.InsertString(5, "F516");
	m_omProjectDiag.InsertString(6, "MA501");
	m_omProjectDiag.InsertString(7, "NL-4DC");
	m_omProjectDiag.InsertString(8, "DX3-EV");
	m_omProjectDiag.InsertString(9, "F507_HEV");
	m_omProjectDiag.InsertString(10, "FE-5B-7B");
	m_omProjectDiag.InsertString(11, "H50");
	m_omProjectDiag.InsertString(12, "N800_BEV");
	m_omProjectDiag.SetCurSel(2);

	m_omUpdateChannel.InsertString(0 , "channel 1");
	m_omUpdateChannel.InsertString(1 , "channel 2");
	m_omUpdateChannel.InsertString(2 , "channel 3");
	m_omUpdateChannel.InsertString(3 , "channel 4");
	m_omUpdateChannel.InsertString(4 , "channel 5");
	m_omUpdateChannel.InsertString(5 , "channel 6");
	m_omUpdateChannel.InsertString(6 , "channel 7");
	m_omUpdateChannel.InsertString(7 , "channel 8");
	m_omUpdateChannel.InsertString(8 , "channel 9");
	m_omUpdateChannel.InsertString(9 , "channel 10");
	m_omUpdateChannel.InsertString(10 , "channel 11");
	m_omUpdateChannel.InsertString(11, "channel 12");
	m_omUpdateChannel.InsertString(12 , "channel 13");
	m_omUpdateChannel.InsertString(13 , "channel 14");
	m_omUpdateChannel.InsertString(14 , "channel 15");
	m_omUpdateChannel.InsertString(15 , "channel 16");
	m_omUpdateChannel.SetCurSel(0);

	m_omUDSReqID.vSetValue(0x705);
	m_omUDSResID.vSetValue(0x70D);
	m_omUDSFunID.vSetValue(0x7DF);
	m_omBlockSize.vSetValue(0);
	m_omSTmin.vSetValue(5);
}


/**********************************************************************************************************
Function Name  :   OnBnClickedUpdate
Input(s)       :   -
Output         :   -
Description    :   called when user click on the button 'load driver'
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::OnBnClickedSelectDriver(void)
{
	CFileDialog drvierFileDlg(TRUE , NULL , NULL , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,	(LPCTSTR)_TEXT("s19 (*.s19)|*.s19|hex (*.hex)|*.hex|All Files (*.*)|*.*||") , NULL);

	PostMessage(WM_UDS_UPDATE , 4 , 0);
	if(drvierFileDlg.DoModal()==IDOK)
	{
		TotalDriverOfByte = 0;
		CString FilePathName;
		FilePathName = drvierFileDlg.GetPathName(); //文件名保存在了FilePathName里
		if(drvierFileDlg.GetFileExt() == "s19" || drvierFileDlg.GetFileExt() == "S19")
		{
			UINT i;
			CString PromptInfo;
			CS19File m_S19File = CS19File(FilePathName);

			m_omBlockPercent.SetRange32(1,500);
			m_omBlockPercent.SetPos(0);
			
			m_omPrompStr = "\r\n" + FilePathName + "\r\n";
			//m_omPrompStr += m_S19File.GetFileHeader() + "\r\n";
			
			PostMessage(WM_UDS_UPDATE , 1 , 0);
			FlashDriverBlockCount = m_S19File.GetBlockCount();
			if(FlashDriverBlockCount > 2)
			{
				PromptInfo.Format("FLASH DRIVER OVERFLOW [buf = 2 | size = %d]\r\n" , FlashDriverBlockCount);
				m_omPrompStr += PromptInfo;
				PostMessage(WM_UDS_UPDATE , 1 , 0);	
				return;
			}
			
			for(i = 0; i < FlashDriverBlockCount; i ++)
			{
				FlashDriverBlockLengh[i] = m_S19File.GetBlockLength(i);
				FlashDriverStartAddr[i] = m_S19File.GetBlockStartAddr(i);
				memcpy(FlashDriverBlockData[i],m_S19File.GetBlockData(i),FlashDriverBlockLengh[i]);
				
				CalculateBlockCRC(i , 0);
				TotalDriverOfByte += FlashDriverBlockLengh[i];
				
				PromptInfo.Format("block%d  address : %x  size : %d , CRC:%04x;\r\n",i + 1,FlashDriverStartAddr[i],FlashDriverBlockLengh[i],FlashDriverBlockCRC[i]);
				m_omPrompStr += PromptInfo;
				PostMessage(WM_UDS_UPDATE , 1 , 0);			

				m_omBlockPercent.SetRange32(1,500);
				m_omBlockPercent.SetPos(500);
			}

			m_omStartUpdate.EnableWindow(false);
		}
		else
		{
			m_omStartUpdate.EnableWindow(false);
			m_omPrompStr = "file[";
			m_omPrompStr += FilePathName;
			m_omPrompStr += "] has a incorrect file format!";

			PostMessage(WM_UDS_UPDATE , 1 , 0);
		}
	}
}

/**********************************************************************************************************
Function Name  :   OnBnClickedUpdate
Input(s)       :   -
Output         :   -
Description    :   called when user click on the button 'load app'
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::OnBnClickedLoadApp(void)
{
	CFileDialog appFileDlg(TRUE , NULL , NULL , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,	(LPCTSTR)_TEXT("s19 (*.s19)|*.s19|hex (*.hex)|*.hex|All Files (*.*)|*.*||") , NULL);
	PostMessage(WM_UDS_UPDATE , 4 , 0);
	if(appFileDlg.DoModal()==IDOK)
	{
		TotalDataOfByte = 0;
		CString FilePathName;
		FilePathName = appFileDlg.GetPathName(); //文件名保存在了FilePathName里
		if(appFileDlg.GetFileExt() == "s19" || appFileDlg.GetFileExt() == "S19")
		{
			UINT i;
			CString PromptInfo;
			CS19File m_S19File = CS19File(FilePathName);

			m_omBlockPercent.SetRange32(1,500);
			m_omBlockPercent.SetPos(0);
			
			AppBlockCount = 0;

			if(FlashDriverBlockCount == 0)
			{
				m_omPrompStr = FilePathName + "\r\n";
				//m_omPrompStr += m_S19File.GetFileHeader() + "\r\n";
				PostMessage(WM_UDS_UPDATE , 1 , 0);
			}
			else
			{
				m_omPrompStr += "\r\n"+FilePathName + "\r\n";
				//m_omPrompStr += m_S19File.GetFileHeader() + "\r\n";
				PostMessage(WM_UDS_UPDATE , 1 , 0);
			}
			
			for(i = 0; i < m_S19File.GetBlockCount() ; i ++)
			{
				AppBlockLengh[i] = m_S19File.GetBlockLength(i);
				AppStartAddr[i] = m_S19File.GetBlockStartAddr(i);
				memcpy(AppBlockData[i],m_S19File.GetBlockData(i),AppBlockLengh[i]);
				
				CalculateBlockCRC(i , 1);	
				TotalDataOfByte += AppBlockLengh[i];
				
				AppBlockCount ++;
				PromptInfo.Format("block%d  address : %x  size : %d , CRC:%04x;\r\n",i + 1,AppStartAddr[i],AppBlockLengh[i],AppBlockCRC[i]);
				m_omPrompStr += PromptInfo;
				PostMessage(WM_UDS_UPDATE , 1 , 0);			

				m_omBlockPercent.SetRange32(1,500);
				m_omBlockPercent.SetPos(500);
			}

			if(m_S19File.IsParseFinish())
			{
				m_omStartUpdate.EnableWindow(true);
			}
			else
			{
				m_omStartUpdate.EnableWindow(false);
			}
		}
		else
		{
			m_omStartUpdate.EnableWindow(false);
			m_omPrompStr = "file[";
			m_omPrompStr += FilePathName;
			m_omPrompStr += "] has a incorrect file format!";

			PostMessage(WM_UDS_UPDATE , 1 , 0);
		}
	}
}

/**********************************************************************************************************
Function Name  :   OnBnClickedUpdate
Input(s)       :   -
Output         :   -
Description    :   called when user click on the button 'start update'
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::OnBnClickedUpdate(void)
{
	CString temp;
	
	PhyReqID = (UINT)m_omUDSReqID.lGetValue();
	PhyResID = (UINT)m_omUDSResID.lGetValue();
	FunReqID = (UINT)m_omUDSFunID.lGetValue();	
	psSendCanMsgUds->m_psTxMsg->m_ucChannel = (UCHAR)m_omUpdateChannel.GetCurSel()+1;
	CompleteDataOfByte = 0;
	
	if(m_omProjectDiag.GetCurSel() == 0)
	{
		m_omPrompStr = "select SGMW CN180S/CN210M/CN210R project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_SGMW , this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 1)
	{
		m_omPrompStr = "select C301/B211/C211 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_C301 , this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 2)
	{
		m_omPrompStr = "select R104 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_R104 , this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 3)
	{
		m_omPrompStr = "select HD10 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_HD10 , this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 4)
	{
		m_omPrompStr = "select EX50 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_EX50, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 5)
	{
		m_omPrompStr = "select F516 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_F516, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 6)
	{
		m_omPrompStr = "select MA501 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_MA501, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 7)
	{
		m_omPrompStr = "select NL-4DC project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_R104, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 8)
	{
		m_omPrompStr = "select DX3-EV project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_R104, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 9)
	{
		m_omPrompStr = "select F507_HEV project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_F507_HEV, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 10)
	{
		m_omPrompStr = "select FE-5B-7B project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_FE_57, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 11)
	{
		m_omPrompStr = "select H50 project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_H50, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	else if(m_omProjectDiag.GetCurSel() == 12)
	{
		m_omPrompStr = "select N800_BEV project\r\n";
		DWORD dwThreadID;
		HANDLE  m_hThreadUpdate = CreateThread(nullptr, 0, UpdateThread_N800BEV, this, 0, &dwThreadID);	
		HANDLE  m_hThreadTP = CreateThread(nullptr, 0, TPLayerThread, this, 0, &dwThreadID);
	}
	
	m_omPrompStr += "request id:";
	temp.Format("0x%X\r\n",PhyReqID);
	m_omPrompStr += temp;

	m_omPrompStr += "response id:";
	temp.Format("0x%X\r\n",PhyResID);
	m_omPrompStr += temp;

	m_omPrompStr += "function id:";
	temp.Format("0x%X\r\n",FunReqID);
	m_omPrompStr += temp;
	m_omPrompStr +="preparing for update!\r\n";
	
	PostMessage(WM_UDS_UPDATE , 1 , 0);
	
}


/**********************************************************************************************************
Function Name  :   CalculateBlockCRC
Input(s)       :   -
Output         :   -
Description    :   calculate the a block crc of the bin code,differrent poject has different way
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::CalculateBlockCRC(UCHAR blockIndex , UCHAR codeType)
{
	if(m_omProjectDiag.GetCurSel() == 0)
	{
		if(codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::SGMWCalcCRC32Slow(FlashDriverBlockLengh[blockIndex],FlashDriverBlockData[blockIndex]);
		}
		else if(codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::SGMWCalcCRC32Slow(AppBlockLengh[blockIndex],AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 1)
	{
		if(codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::ChanganCalcCRC16Slow(FlashDriverBlockLengh[blockIndex],FlashDriverBlockData[blockIndex]);
		}
		else if(codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::ChanganCalcCRC16Slow(AppBlockLengh[blockIndex],AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 2)
	{
		
	}
	else if(m_omProjectDiag.GetCurSel() == 3)
	{
		
		
	}
	else if(m_omProjectDiag.GetCurSel() == 4)
	{
		if(codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::H50CalcCRC32Slow(FlashDriverBlockLengh[blockIndex],FlashDriverBlockData[blockIndex]);
		}
		else if(codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::H50CalcCRC32Slow(AppBlockLengh[blockIndex],AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 5)
	{
		if(codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::SGMWCalcCRC32Slow(FlashDriverBlockLengh[blockIndex],FlashDriverBlockData[blockIndex]);
		}
		else if(codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::SGMWCalcCRC32Slow(AppBlockLengh[blockIndex],AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 6)
	{
		if (codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(FlashDriverBlockLengh[blockIndex], FlashDriverBlockData[blockIndex]);
		}
		else if (codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(AppBlockLengh[blockIndex], AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 9)
	{
		if (codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(FlashDriverBlockLengh[blockIndex], FlashDriverBlockData[blockIndex]);
		}
		else if (codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(AppBlockLengh[blockIndex], AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 10)
	{
		if (codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(FlashDriverBlockLengh[blockIndex], FlashDriverBlockData[blockIndex]);
		}
		else if (codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(AppBlockLengh[blockIndex], AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 11)
	{
		if (codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::H50CalcCRC32Slow(FlashDriverBlockLengh[blockIndex], FlashDriverBlockData[blockIndex]);
		}
		else if (codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::H50CalcCRC32Slow(AppBlockLengh[blockIndex], AppBlockData[blockIndex]);
		}
	}
	else if(m_omProjectDiag.GetCurSel() == 12)
	{
		if (codeType == 0)
		{
			FlashDriverBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(FlashDriverBlockLengh[blockIndex], FlashDriverBlockData[blockIndex]);
		}
		else if (codeType == 1)
		{
			AppBlockCRC[blockIndex] = CCodeCRC::GeneralCalcCrc32(AppBlockLengh[blockIndex], AppBlockData[blockIndex]);
		}
	}
}

	
/**********************************************************************************************************
Function Name  :   ParseUDSData
Input(s)       :   -
Output         :   -
Description    :   This function is called by protocol to parse UDS network layer data.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::ParseUDSData(UINT id, UCHAR arrayMsg[], UCHAR dataLen, UCHAR extendFrame, UCHAR rtr, UCHAR channel)
{
	if (PhyResID == id && IsUpdateThreadExist == true)/*check if id match the set*/
	{
		static UINT length = 0;
		static UCHAR RxData[100];
		static UINT RxLen = 0;

		if ((arrayMsg[0] & 0xF0) == 0x10)
		{
			length = ((arrayMsg[0] & 0x0F) << 8) + arrayMsg[1];
			RxData[0] = arrayMsg[2];
			RxData[1] = arrayMsg[3];
			RxData[2] = arrayMsg[4];
			RxData[3] = arrayMsg[5];
			RxData[4] = arrayMsg[6];
			RxData[5] = arrayMsg[7];
			RxLen = 6;
			SendFC();
			CurrTPStep = RX_WAIT_CF;
			TPRxCFIndex = 1;
			TPRxCFTimes = 0;
		}
		else if ((arrayMsg[0] & 0xF0) == 0x20)
		{
			if(CurrTPStep == RX_WAIT_CF)
			{
				if(TPRxCFIndex == (arrayMsg[0] & 0x0F))
				{
					if (RxLen + 7 < length)
					{
						RxData[RxLen++] = arrayMsg[1];
						RxData[RxLen++] = arrayMsg[2];
						RxData[RxLen++] = arrayMsg[3];
						RxData[RxLen++] = arrayMsg[4];
						RxData[RxLen++] = arrayMsg[5];
						RxData[RxLen++] = arrayMsg[6];
						RxData[RxLen++] = arrayMsg[7];

						TPRxCFIndex++;
						if(TPRxCFIndex > 15)
						{
							TPRxCFIndex = 0;
						}
						
						if(RX_BlockSize != 0)
						{
							TPRxCFTimes++;
							if(TPRxCFTimes  >= RX_BlockSize)
							{
								SendFC();
								CurrTPStep = RX_WAIT_CF;
								TPRxCFTimes = 0;
							}
						}
					}
					else
					{
						unsigned int index = 1;
						while (RxLen < length)
						{
							RxData[RxLen++] = arrayMsg[index];
							index++;
						}

						DataToUDSBuffer(id, length, RxData);
						TPRxCFTimes = 0;
						TPRxCFIndex = 0;
						CurrTPStep = TP_IDLE;
					}
				}
			}
		}
		else if ((arrayMsg[0] & 0xF0) == 0x00)
		{
			unsigned int index = 1;
			length = (arrayMsg[0] & 0x0F);
			RxLen = 0;
			while (RxLen < length)
			{
				RxData[RxLen++] = arrayMsg[index];
				index++;
			}

			DataToUDSBuffer(id, length, RxData);
		}
		else if ((arrayMsg[0] & 0xF0) == 0x30)
		{
			//m_omPrompStr += "\r\nRX FC\r\n";
			if((arrayMsg[0] & 0x0F) == 0x00)
			{
				if(CurrTPStep == TX_WAIT_FC)
				{
					TX_BlockSize = *(arrayMsg + 1);
					if(arrayMsg[2]<=0x7F)                  // Evaluate all the possible Values for TX_STmin
					{
						TX_STmin  = arrayMsg[2];
					}
					else if (0xF1 <= arrayMsg[2] && arrayMsg[2] <= 0xF9)
					{
						TX_STmin  = (UCHAR)((arrayMsg[2]&0x0F)*0.1);
					}
					else
					{
						TX_STmin  = 127;                                 //According to the ISO-TP
					}
					
					CurrTPStep = TX_WAIT_CF;
					IsNeedSendFC = true;
					//m_omPrompStr += "\r\nset CF flg\r\n";
					KillTimer(ID_TIMER_TP_EXT);
					//SetTimer(ID_TIMER_TP_CF , 1 , NULL);
					//SendSeveralCF();
				}
			}
			else if((arrayMsg[0] & 0x0F) == 0x01)
			{
				
			}
			else if((arrayMsg[0] & 0x0F) == 0x02)
			{
				if(CurrTPStep == TX_WAIT_FC)
				{
					CurrTPStep = TP_IDLE;
				}
			}
		}
	}
}

/**********************************************************************************************************
Function Name  :   DataToUDSBuffer
Input(s)       :   -
Output         :   -
Description    :   -
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   22.03.2017
**********************************************************************************************************/
void CUDSExtendWnd::DataToUDSBuffer(UINT id, UINT length, UCHAR *data)
{
	int i;
	for (i = 0; i < 5; i++)
	{
		if (UdsData[i].valid == false)
		{
			UdsData[i].ID = id;
			UdsData[i].length = length;
			memcpy(UdsData[i].data, data, length);
			UdsData[i].valid = true;
			break;
		}
	}
}

/**********************************************************************************************************
Function Name  :   SendSF
Input(s)       :   -
Output         :   -
Description    :   send a single frame.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :    15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendSF(UDSAddressType addrType, UCHAR length, UCHAR* data)
{
	CString str;
	int i;

	if(length > 7)
	{
		str.Format("\r\n length %d overflow ",  length);
		m_omPrompStr += str;
		PostMessage(WM_UDS_UPDATE , 1 , 0);
		return;
	}
	
	FillRequestID(addrType);

	psSendCanMsgUds->m_psTxMsg->m_ucDataLen = 8;
	psSendCanMsgUds->m_psTxMsg->m_ucData[0] = length;
	
	//str.Format("Tx: ID %x length %x", psSendCanMsgUds->m_psTxMsg->m_unMsgID, length);
	//m_omPrompStr += str;
	for (i = 0; i < 7; i++)
	{
		if (i < length)
		{
			psSendCanMsgUds->m_psTxMsg->m_ucData[i + 1] = data[i];
		}
		else
		{
			psSendCanMsgUds->m_psTxMsg->m_ucData[i + 1] = 0x55;
		}
		//str.Format(" %x", data[i]);
		//m_omPrompStr += str;
	}
	
	SendSimpleCanMessage();
	
	//m_omPrompStr += "\r\n";
	if(addrType == PHYSICAL)
	{
		CurrTPStep = TP_IDLE;		
	}
}

/**********************************************************************************************************
Function Name  :   SendFC
Input(s)       :   -
Output         :   -
Description    :   send a flowcontrol frame within receiving.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :    15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendFC(void)
{
	FillRequestID(PHYSICAL);

	RX_STmin = (UCHAR)m_omSTmin.lGetValue();
	RX_BlockSize = (UCHAR)m_omBlockSize.lGetValue();
	
	psSendCanMsgUds->m_psTxMsg->m_ucDataLen = 8;
	psSendCanMsgUds->m_psTxMsg->m_ucData[0] = 0x30;
	psSendCanMsgUds->m_psTxMsg->m_ucData[1] = RX_BlockSize;
	psSendCanMsgUds->m_psTxMsg->m_ucData[2] = RX_STmin;
	psSendCanMsgUds->m_psTxMsg->m_ucData[3] = 0x55;
	psSendCanMsgUds->m_psTxMsg->m_ucData[4] = 0x55;
	psSendCanMsgUds->m_psTxMsg->m_ucData[5] = 0x55;
	psSendCanMsgUds->m_psTxMsg->m_ucData[6] = 0x55;
	psSendCanMsgUds->m_psTxMsg->m_ucData[7] = 0x55;
	
	SendSimpleCanMessage();
}

/**********************************************************************************************************
Function Name  :   SendFF
Input(s)       :   -
Output         :   -
Description    :   send a first frame to start transmit.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :    15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendFF(void)
{
	if(LargeFrameLength <= MAX_UDS_LARGE_FRAME_LENGTH)
	{
		FillRequestID(PHYSICAL);

		psSendCanMsgUds->m_psTxMsg->m_ucDataLen = 8;
		psSendCanMsgUds->m_psTxMsg->m_ucData[0] = 0x10 + ((LargeFrameLength >> 8) & 0x0F);
		psSendCanMsgUds->m_psTxMsg->m_ucData[1] = (LargeFrameLength & 0xFF);
		memcpy(psSendCanMsgUds->m_psTxMsg->m_ucData + 2 , LargeFrameBuf , 6);
		LargeFrameFinishLength = 6;
		CurrTPStep = TX_WAIT_FC;
		CFIndex = 1;
		SendSimpleCanMessage();
	}
	else
	{
		m_omPrompStr += "\r\nFF size overflow\r\n";
		PostMessage(WM_UDS_UPDATE , 1 , 0);
	}
}

/**********************************************************************************************************
Function Name  :   SendCF
Input(s)       :   -
Output         :   -
Description    :   send a CF frame .
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :    15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendCF(void)
{
	FillRequestID(PHYSICAL);

	psSendCanMsgUds->m_psTxMsg->m_ucDataLen = 8;
	psSendCanMsgUds->m_psTxMsg->m_ucData[0] = 0x20 + CFIndex;
	if(LargeFrameFinishLength + 7 <= LargeFrameLength)
	{
		memcpy(psSendCanMsgUds->m_psTxMsg->m_ucData + 1 , LargeFrameBuf + LargeFrameFinishLength , 7);
		LargeFrameFinishLength += 7;
	}
	else
	{
		UCHAR i;
		memcpy(psSendCanMsgUds->m_psTxMsg->m_ucData + 1 , LargeFrameBuf + LargeFrameFinishLength , LargeFrameLength - LargeFrameFinishLength);
		for(i = LargeFrameLength - LargeFrameFinishLength ; i < 8 ; i++)
		{
			psSendCanMsgUds->m_psTxMsg->m_ucData[i + 1] = 0x55;
		}
		LargeFrameFinishLength += (LargeFrameLength - LargeFrameFinishLength);
	}
	CFIndex++;
	if(CFIndex > 0x0F)
	{
		CFIndex = 0;
	}
	SendSimpleCanMessage();
}

/**********************************************************************************************************
Function Name  :   FillRequestID
Input(s)       :   -
Output         :   -
Description    :   fill tx msg ID with setting.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::FillRequestID(UDSAddressType addrType)
{
	if (addrType == FUNCTIONAL)
	{
		psSendCanMsgUds->m_psTxMsg->m_unMsgID = FunReqID;
	}
	else
	{
		psSendCanMsgUds->m_psTxMsg->m_unMsgID = PhyReqID;
	}
	
	if(psSendCanMsgUds->m_psTxMsg->m_unMsgID > 0x7FF)
	{
		psSendCanMsgUds->m_psTxMsg->m_ucEXTENDED = 1;
	}
	else
	{
		psSendCanMsgUds->m_psTxMsg->m_ucEXTENDED = 0;
	}
}


/**********************************************************************************************************
Function Name  :   RequestDefaultSession
Input(s)       :   -
Output         :   -
Description    :   send a 10 01/81 reqeust.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestDefaultSession(UDSAddressType addressType, bool suppress)
{
	UCHAR data[8];
	data[0] = 0x10;
	data[1] = 0x01 | (suppress ? 0x80 : 0x00);
	
	SendSF(addressType ,2 , data);
}

/**********************************************************************************************************
Function Name  :   RequestProgramSession
Input(s)       :   -
Output         :   -
Description    :   send a 10 02/82 reqeust.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestProgramSession(UDSAddressType addressType, bool suppress)
{
	UCHAR data[8];
	data[0] = 0x10;
	data[1] = 0x02 | (suppress ? 0x80 : 0x00);
	
	SendSF(addressType ,2 , data);
}

/**********************************************************************************************************
Function Name  :   RequestExtendedSession
Input(s)       :   -
Output         :   -
Description    :   send a 10 03/83 reqeust.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestExtendedSession(UDSAddressType addressType, bool suppress)
{
	UCHAR data[8];
	data[0] = 0x10;
	data[1] = 0x03 | (suppress ? 0x80 : 0x00);
	
	SendSF(addressType ,2 , data);
}

/**********************************************************************************************************
Function Name  :   ReqeustDTCControl
Input(s)       :   -
Output         :   -
Description    :   send a 85 01/81/02/82 reqeust.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::ReqeustDTCControl(UDSAddressType addressType , DtcOnOff OnOff , bool suppress)
{
	UCHAR data[8];
	data[0] = 0x85;
	if(OnOff == DTC_OFF)
	{
		data[1] = 0x02 | (suppress ? 0x80 : 0x00);
	}
	else
	{
		data[1] = 0x01 | (suppress ? 0x80 : 0x00);
	}
	
	SendSF(addressType ,2 , data);
}

/**********************************************************************************************************
Function Name  :   ReqeustCommControl
Input(s)       :   -
Output         :   -
Description    :   send a 28 msg control reqeust.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::ReqeustCommControl(UDSAddressType addressType , CommControlType controlType  , CommMsgType msgType, bool suppress)
{
	UCHAR data[8];
	data[0] = 0x28;
	data[1] = controlType | (suppress ? 0x80 : 0x00);
	data[2] = msgType;
		
	SendSF(addressType ,3 , data);
}

/**********************************************************************************************************
Function Name  :   ReqeustSeed
Input(s)       :   -
Output         :   -
Description    :   send a 27 reqeust to get random seed.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestSeed(UCHAR subFunction)
{
	UCHAR data[8];
	data[0] = 0x27;
	data[1] = subFunction;
		
	SendSF(PHYSICAL , 2 , data);
}


/**********************************************************************************************************
Function Name  :   SendKey
Input(s)       :   -
Output         :   -
Description    :   send a 27 reqeust to send a key ECU,a valid key can unlock ECU.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendKey(UCHAR subFunction , UINT32 key , UCHAR keyLength)
{
	UCHAR data[8];
	data[0] = 0x27;
	data[1] = subFunction;
	data[2] = (UCHAR)(key >> 24);
	data[3] = (UCHAR)(key >> 16);
	data[4] = (UCHAR)(key >> 8);
	data[5] = (UCHAR)(key);
		
	SendSF(PHYSICAL , keyLength + 2, data);
}


/**********************************************************************************************************
Function Name  :   ReuestWriteFingerPrint_R104
Input(s)       :   -
Output         :   -
Description    :   send a 2E F1 84 request 
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteFingerPrint_R104(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x84;
	LargeFrameBuf[3] = m_time.GetYear() % 100;
	LargeFrameBuf[4] = m_time.GetMonth();
	LargeFrameBuf[5] = m_time.GetDay();
	LargeFrameBuf[6] = 'c';
	LargeFrameBuf[7] = 'q';
	LargeFrameBuf[8] = 'r';
	LargeFrameBuf[9] = 'y';
	
	LargeFrameLength = 10;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   DownloadFlashDrvier_Request
Input(s)       :   -
Output         :   -
Description    :   send 34 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadFlashDrvier_Request(void)
{
	LargeFrameBuf[0] = 0x34;
	LargeFrameBuf[1] = 0x00;
	LargeFrameBuf[2] = 0x44;
	LargeFrameBuf[3] = (UCHAR)(FlashDriverStartAddr[FlashDriverDownloadIndex] >> 24);
	LargeFrameBuf[4] = (UCHAR)(FlashDriverStartAddr[FlashDriverDownloadIndex] >> 16);
	LargeFrameBuf[5] = (UCHAR)(FlashDriverStartAddr[FlashDriverDownloadIndex] >> 8);
	LargeFrameBuf[6] = (UCHAR)(FlashDriverStartAddr[FlashDriverDownloadIndex]);
	LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockLengh[FlashDriverDownloadIndex] >> 24);
	LargeFrameBuf[8] = (UCHAR)(FlashDriverBlockLengh[FlashDriverDownloadIndex] >> 16);
	LargeFrameBuf[9] = (UCHAR)(FlashDriverBlockLengh[FlashDriverDownloadIndex] >> 8);
	LargeFrameBuf[10] = (UCHAR)(FlashDriverBlockLengh[FlashDriverDownloadIndex]);
	
	LargeFrameLength = 11;
	SendFF();

	//PostMessage(WM_UDS_UPDATE , 2 , 0);
}

/**********************************************************************************************************
Function Name  :   DownloadFlashDrvier_Transfer
Input(s)       :   -
Output         :   -
Description    :   send 36 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadFlashDrvier_Transfer(void)
{
	if(BlockDataFinish < FlashDriverBlockLengh[FlashDriverDownloadIndex])
	{
		UINT length;
		LargeFrameBuf[0] = 0x36;
		LargeFrameBuf[1] = TransferDataIndex;
		if((BlockDataFinish + MaxBlockSize - 2) <= FlashDriverBlockLengh[FlashDriverDownloadIndex])
		{
			length = MaxBlockSize - 2;
		}
		else
		{
			length = FlashDriverBlockLengh[FlashDriverDownloadIndex] - BlockDataFinish;
		}
		
		memcpy(LargeFrameBuf + 2 , FlashDriverBlockData[FlashDriverDownloadIndex] + BlockDataFinish , length);
		BlockDataFinish += length;
		CompleteDataOfByte += length;
		if(length + 2 > 7)
		{
			LargeFrameLength = length + 2;
			SendFF();
		}
		else
		{
			SendSF(PHYSICAL , length + 2, LargeFrameBuf);
		}
		TransferDataIndex++;
		RefreshProgress(FLASH_DRIVER);
		//PostMessage(WM_UDS_UPDATE , 2 , 0);
	}
}

/**********************************************************************************************************
Function Name  :   DownloadFlashDrvier_Exit
Input(s)       :   -
Output         :   -
Description    :   send 37 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadFlashDrvier_Exit(void)
{
	UCHAR data[8];
	data[0] = 0x37;
		
	SendSF(PHYSICAL , 1 , data);
	//PostMessage(WM_UDS_UPDATE , 2 , 0);
}

/**********************************************************************************************************
Function Name  :   EraseAppRom
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 00 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xFF;
	LargeFrameBuf[3] = 0x00;
	LargeFrameBuf[4] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[5] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[6] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[7] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex]);
	LargeFrameBuf[8] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[9] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[10] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[11] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex]);
	LargeFrameLength = 12;
		
	SendFF();
}

/**********************************************************************************************************
Function Name  :   DownloadApp_Request
Input(s)       :   -
Output         :   -
Description    :   send 34 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadApp_Request(void)
{
	LargeFrameBuf[0] = 0x34;
	LargeFrameBuf[1] = 0x00;
	LargeFrameBuf[2] = 0x44;
	LargeFrameBuf[3] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[4] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[5] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[6] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex]);
	LargeFrameBuf[7] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[8] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[9] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[10] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex]);
	LargeFrameLength = 11;
	
	SendFF();

	//PostMessage(WM_UDS_UPDATE , 3 , 0);
}

/**********************************************************************************************************
Function Name  :   DownloadApp_Transfer
Input(s)       :   -
Output         :   -
Description    :   send 36 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadApp_Transfer(void)
{
	if(BlockDataFinish < AppBlockLengh[AppBlockDownloadIndex])
	{
		UINT length;
		LargeFrameBuf[0] = 0x36;
		LargeFrameBuf[1] = TransferDataIndex;
		if((BlockDataFinish + MaxBlockSize - 2) <= AppBlockLengh[AppBlockDownloadIndex])
		{
			length = MaxBlockSize - 2;
		}
		else
		{
			length =AppBlockLengh[AppBlockDownloadIndex] - BlockDataFinish;
		}
		
		memcpy(LargeFrameBuf + 2 , AppBlockData[AppBlockDownloadIndex] + BlockDataFinish , length);
		BlockDataFinish += length;
		CompleteDataOfByte += length;
		if(length + 2 > 7)
		{
			LargeFrameLength = length + 2;
			SendFF();
		}
		else
		{
			SendSF(PHYSICAL , length + 2, LargeFrameBuf);
		}
		TransferDataIndex++;
		//PostMessage(WM_UDS_UPDATE , 3 , 0);
		RefreshProgress(APP_CODE);
		//SetTimer(ID_TIMER_S3_SERVER , 2000 , NULL);
	}
}

/**********************************************************************************************************
Function Name  :   DownloadApp_Exit
Input(s)       :   -
Output         :   -
Description    :   send 37 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadApp_Exit(void)
{
	UCHAR data[8];
	data[0] = 0x37;
		
	SendSF(PHYSICAL , 1 , data);
	//PostMessage(WM_UDS_UPDATE , 3 , 0);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_R104
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_R104(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_R104
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 02 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_R104(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0x02;
	data[3] = 0x02;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   RequestResetECU
Input(s)       :   -
Output         :   -
Description    :   send 11 01/03 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestResetECU(UDSAddressType addressType , UCHAR ResetType , bool suppress)
{
	UCHAR data[8];
	data[0] = 0x11;
	data[1] = (ResetType | (suppress ? 0x80 : 0x00));
		
	SendSF(addressType , 2 , data);
}

/**********************************************************************************************************
Function Name  :   RequestClearDTC
Input(s)       :   -
Output         :   -
Description    :   send 14 FF FF FF request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestClearDTC(UDSAddressType addressType)
{
	UCHAR data[8];
	data[0] = 0x14;
	data[1] = 0xFF;
	data[2] = 0xFF;
	data[3] = 0xFF;
		
	SendSF(addressType , 4 , data);
}

/**********************************************************************************************************
Function Name  :   SendTesterPresent
Input(s)       :   -
Output         :   -
Description    :   send 3E 00/80 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendTesterPresent(UDSAddressType addressType , bool suppress)
{
	UCHAR data[8];
	data[0] = 0x3E;
	data[1] = (suppress ? 0x80 : 0x00);
		
	SendSF(addressType , 2 , data);
}

/**********************************************************************************************************
Function Name  :   RequestReadDID
Input(s)       :   -
Output         :   -
Description    :   Request Read DID
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestReadDID(USHORT did)
{
	UCHAR CanData[8];
	CanData[0] = 0x22;
	CanData[1] = (UCHAR)(did >> 8);
	CanData[2] = (UCHAR)(did);
	SendSF(PHYSICAL , 3 , CanData);
}

/**********************************************************************************************************
Function Name  :   RequestHD10EnableProgram
Input(s)       :   -
Output         :   -
Description    :   send A5 03 Request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestEnableProgram_HD10(void)
{
	UCHAR CanData[8];
	CanData[0] = 0xA5;
	CanData[1] = 0x03;
	SendSF(PHYSICAL , 2 , CanData);
}

/**********************************************************************************************************
Function Name  :   RequestCheckPreCondition_HD10
Input(s)       :   -
Output         :   -
Description    :   send A5 01 Request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestCheckPreCondition_HD10(void)
{
	UCHAR CanData[8];
	CanData[0] = 0xA5;
	CanData[1] = 0x01;
	SendSF(PHYSICAL , 2 , CanData);
}


/**********************************************************************************************************
Function Name  :   RequestWriteBootFP_HD10
Input(s)       :   -
Output         :   -
Description    :   write DID F183
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteBootFP_HD10(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x83;
	LargeFrameBuf[3] = 0x7F;
	LargeFrameBuf[4] = 0xFF;
	LargeFrameBuf[5] = 0xFF;
	LargeFrameBuf[6] = m_time.GetYear() /100;
	LargeFrameBuf[7] = m_time.GetYear() % 100;
	LargeFrameBuf[8] = m_time.GetMonth();
	LargeFrameBuf[9] = m_time.GetDay();
	
	LargeFrameLength = 10;
	
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestWriteAppFP_HD10
Input(s)       :   -
Output         :   -
Description    :   write DID F184
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteAppFP_HD10(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x84;
	LargeFrameBuf[3] = 0x7F;
	LargeFrameBuf[4] = 0xFF;
	LargeFrameBuf[5] = 0xFF;
	LargeFrameBuf[6] = m_time.GetYear() /100;
	LargeFrameBuf[7] = m_time.GetYear() % 100;
	LargeFrameBuf[8] = m_time.GetMonth();
	LargeFrameBuf[9] = m_time.GetDay();
	
	LargeFrameLength = 10;
	SendFF();
}


/**********************************************************************************************************
Function Name  :   RequestWriteDataFP_HD10
Input(s)       :   -
Output         :   -
Description    :   write DID F185
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteDataFP_HD10(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x85;
	LargeFrameBuf[3] = 0x7F;
	LargeFrameBuf[4] = 0xFF;
	LargeFrameBuf[5] = 0xFF;
	LargeFrameBuf[6] = m_time.GetYear() /100;
	LargeFrameBuf[7] = m_time.GetYear() % 100;
	LargeFrameBuf[8] = m_time.GetMonth();
	LargeFrameBuf[9] = m_time.GetDay();
	
	LargeFrameLength = 10;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestDownFlashDriver_HD10
Input(s)       :   -
Output         :   -
Description    :   send 34 01/02/03...
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestDownFlashDriver_HD10(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x34;
	CanData[1] = FlashDriverDownloadIndex;
	SendSF(PHYSICAL , 2 , CanData);
}

/**********************************************************************************************************
Function Name  :   RequestReadDID
Input(s)       :   -
Output         :   -
Description    :   Request Read DID
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom_HD10(UCHAR index)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xFF;
	CanData[3] = 0x00;
	CanData[4] = index;
	SendSF(PHYSICAL , 5 , CanData);
	m_omBlockPercent.SetPos(0);
	m_omBlockPercent.SetRange32(1,500);
}

/**********************************************************************************************************
Function Name  :   DownloadApp_Request_HD10
Input(s)       :   -
Output         :   -
Description    :   send 34 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::DownloadApp_Request_HD10(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x34;
	CanData[1] = AppBlockDownloadIndex;
	SendSF(PHYSICAL , 2 , CanData);
}


/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_HD10
Input(s)       :   -
Output         :   -
Description    :   send 31 01 DF 01
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_HD10(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xDF;
	CanData[3] = 0x01;
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		CanData[4] = FlashDriverDownloadIndex;
	}
	else
	{
		CanData[4] = AppBlockDownloadIndex;
	}
	SendSF(PHYSICAL , 5 , CanData);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_HD10
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_HD10(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_EX50
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_EX50(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_EX50
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 02 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_EX50(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0x02;
	LargeFrameBuf[3] = 0x02;
	
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		LargeFrameBuf[4] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex]);
	}
	else if(m_CurrentTransfer == APP_CODE)
	{
		LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);
	}
	
	LargeFrameLength = 8;
	SendFF();
}


/**********************************************************************************************************
Function Name  :   EraseAppRom_F516
Input(s)       :   -
Output         :   -
Description    :   Request Erase app rom
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   06.06.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom_F516(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xFF;
	CanData[3] = 0x00;
	CanData[4] = 0x02;
	SendSF(PHYSICAL , 5 , CanData);
	m_omBlockPercent.SetPos(0);
	m_omBlockPercent.SetRange32(1,500);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_F516
Input(s)       :   -
Output         :   -
Description    :   send 31 01 F0 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_F516(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xFF;
	LargeFrameBuf[3] = 0x01;
	LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);
		
	LargeFrameLength = 8;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_F516
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 02 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_F516(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   EraseAppRom_SGMW
Input(s)       :   -
Output         :   -
Description    :   Request Erase app rom
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   06.07.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom_SGMW(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xFF;
	CanData[3] = 0x00;
	SendSF(PHYSICAL , 4 , CanData);
	m_omBlockPercent.SetPos(0);
	m_omBlockPercent.SetRange32(1,500);
}

/**********************************************************************************************************
Function Name  :   RequestCheckPreCondition_SGMW
Input(s)       :   -
Output         :   -
Description    :   send 31 01 DF 00 Request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   22.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestCheckPreCondition_SGMW(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xDF;
	CanData[3] = 0x00;
	SendSF(FUNCTIONAL , 4 , CanData);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_SGMW
Input(s)       :   -
Output         :   -
Description    :   send 31 01 DF FF request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_SGMW(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xDF;
	LargeFrameBuf[3] = 0xFF;
	LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);
		
	LargeFrameLength = 8;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_SGMW
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_SGMW(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xFF;
	CanData[3] = 0x01;
	SendSF(PHYSICAL , 4 , CanData);
}

/**********************************************************************************************************
Function Name  :   RequestWriteFingerPrint_SGMW
Input(s)       :   -
Output         :   -
Description    :   send a 2E F1 84 request 
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteFingerPrint_SGMW(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x84;
	LargeFrameBuf[3] = 'R';
	LargeFrameBuf[4] = 'Y';
	LargeFrameBuf[5] = '-';
	LargeFrameBuf[6] = 'P';
	LargeFrameBuf[7] = 'C';
	LargeFrameBuf[8] = m_time.GetYear() /100;
	LargeFrameBuf[9] = m_time.GetYear() % 100;
	LargeFrameBuf[10] = m_time.GetMonth();
	LargeFrameBuf[11] = m_time.GetDay();
	
	LargeFrameLength = 12;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestCheckPreCondition_SGMW
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 03 Request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   03.07.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestCheckPreCondition_C301(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0x02;
	CanData[3] = 0x03;
	SendSF(PHYSICAL , 4 , CanData);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_R104
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_C301(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;
		
	SendSF(PHYSICAL , 4 , data);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_R104
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 02 request
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::ActiveProgramming_C301(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0x02;
	LargeFrameBuf[3] = 0x02;
	LargeFrameBuf[4] = (UCHAR)(FlashDriverStartAddr[0] >> 24);
	LargeFrameBuf[5] = (UCHAR)(FlashDriverStartAddr[0] >> 16);
	LargeFrameBuf[6] = (UCHAR)(FlashDriverStartAddr[0] >> 8);
	LargeFrameBuf[7] = (UCHAR)(FlashDriverStartAddr[0]);
	
	LargeFrameLength = 8;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestCheckPreCondition_MA501
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 02 Request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   08.08.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestCheckPreCondition_MA501(void)
{
	UCHAR CanData[8];
	CanData[0] = 0x31;
	CanData[1] = 0x01;
	CanData[2] = 0xFF;
	CanData[3] = 0x02;
	SendSF(PHYSICAL, 4, CanData);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_MA501
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   09.08.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_MA501(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;

	SendSF(PHYSICAL, 4, data);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_MA501
Input(s)       :   -
Output         :   -
Description    :   send 31 01 F0 01 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   09.08.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_MA501(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xF0;
	LargeFrameBuf[3] = 0x01;
	
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		LargeFrameBuf[4] = (UCHAR)(FlashDriverBlockCRC[0] >> 24);
		LargeFrameBuf[5] = (UCHAR)(FlashDriverBlockCRC[0] >> 16);
		LargeFrameBuf[6] = (UCHAR)(FlashDriverBlockCRC[0] >> 8);
		LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockCRC[0]);
	}
	else
	{
		LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);

		CString PromptStr;
		PromptStr.Format("chek CRC of block %d \r\n",AppBlockDownloadIndex + 1);
		m_omPrompStr += PromptStr;
		PostMessage(WM_UDS_UPDATE, 1, 0);
	}

	LargeFrameLength = 8;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestWriteFingerPrint_MA501
Input(s)       :   -
Output         :   -
Description    :   send a 2E F1 5A request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteFingerPrint_MA501(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x5A;
	LargeFrameBuf[3] = m_time.GetYear() / 100;
	LargeFrameBuf[4] = m_time.GetYear() % 100;
	LargeFrameBuf[5] = m_time.GetMonth();
	LargeFrameBuf[6] = m_time.GetDay();
	LargeFrameBuf[7] = 'c';
	LargeFrameBuf[8] = 'q';
	LargeFrameBuf[9] = 'r';
	LargeFrameBuf[10] = 'y';
	LargeFrameBuf[11] = 'k';
	LargeFrameBuf[12] = 'j';

	LargeFrameLength = 13;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   EraseAppRom_MA501
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 00 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   16.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom_MA501(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xFF;
	LargeFrameBuf[3] = 0x00;
	LargeFrameBuf[4] = 0x44;
	LargeFrameBuf[5] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[6] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[7] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[8] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex]);
	LargeFrameBuf[9] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[10] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[11] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[12] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex]);
	LargeFrameLength = 13;

	SendFF();

	CString PromptStr;
	PromptStr.Format("\r\nstart erase block %d ...\r\n",AppBlockDownloadIndex + 1);
	m_omPrompStr += PromptStr;
	PostMessage(WM_UDS_UPDATE, 1, 0);
}

/**********************************************************************************************************
Function Name  :   RequestWriteFingerPrint_F507HEV
Input(s)       :   -
Output         :   -
Description    :   send 2E F1 84 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   29.08.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteFingerPrint_F507HEV(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x84;
	LargeFrameBuf[3] = m_time.GetYear() % 100;
	LargeFrameBuf[4] = m_time.GetMonth();
	LargeFrameBuf[5] = m_time.GetDay();
	LargeFrameBuf[6] = 'c';
	LargeFrameBuf[7] = 'q';
	LargeFrameBuf[8] = 'r';
	LargeFrameBuf[9] = 'y';
	
	LargeFrameLength = 10;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestWriteRepairCode_FE57
Input(s)       :   -
Output         :   -
Description    :   send 2E F1 98request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteRepairCode_FE57(void)
{
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x98;
	LargeFrameBuf[3] = 'R';
	LargeFrameBuf[4] = 'Y';
	LargeFrameBuf[5] = '_';
	LargeFrameBuf[6] = 'B';
	LargeFrameBuf[7] = 'u';
	LargeFrameBuf[8] = 's';
	LargeFrameBuf[9] = 'm';
	LargeFrameBuf[10] = 'a';
	LargeFrameBuf[11] = 's';
	LargeFrameBuf[12] = 't';
	LargeFrameBuf[13] = 'e';
	LargeFrameBuf[14] = 'r';
	LargeFrameBuf[15] = '_';
	LargeFrameBuf[16] = '3';
	LargeFrameBuf[17] = '.';
	LargeFrameBuf[18] = '1';
	
	LargeFrameLength = 19;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   RequestWriteProgramDate_FE57
Input(s)       :   -
Output         :   -
Description    :   send 2E F1 99request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
void CUDSExtendWnd::RequestWriteProgramDate_FE57(void)
{
	CTime m_time = CTime::GetCurrentTime();
	UCHAR l_year_h = m_time.GetYear() / 100;
	UCHAR l_year_l = m_time.GetYear() % 100;
	UCHAR l_month = m_time.GetMonth();
	UCHAR l_day = m_time.GetDay();
	
	UCHAR CanData[8];
	CanData[0] = 0x2E;
	CanData[1] = 0xF1;
	CanData[2] = 0x99;
	CanData[3] = ((l_year_h / 10) << 4) + (l_year_h % 10);//to BCD code
	CanData[4] = ((l_year_l / 10) << 4) + (l_year_l % 10);//to BCD code
	CanData[5] = ((l_month / 10) << 4) + (l_month % 10);//to BCD code
	CanData[6] = ((l_day / 10) << 4) + (l_day % 10);//to BCD code
	SendSF(PHYSICAL , 7 , CanData);
}

/**********************************************************************************************************
Function Name  :   CheckProgramIntegrality_FE57
Input(s)       :   -
Output         :   -
Description    :   send 31 01 02 02 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramIntegrality_FE57(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0x02;
	LargeFrameBuf[3] = 0x02;
	
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		LargeFrameBuf[4] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex]);
	}
	else
	{
		LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);

		CString PromptStr;
		PromptStr.Format("chek CRC of block %d \r\n",AppBlockDownloadIndex + 1);
		m_omPrompStr += PromptStr;
		PostMessage(WM_UDS_UPDATE, 1, 0);
	}

	LargeFrameLength = 8;
	SendFF();
}

/**********************************************************************************************************
Function Name  :   EraseAppRom_FE57
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 00 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
void CUDSExtendWnd::EraseAppRom_FE57(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xFF;
	LargeFrameBuf[3] = 0x00;
	LargeFrameBuf[4] = 0x44;
	LargeFrameBuf[5] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[6] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[7] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[8] = (UCHAR)(AppStartAddr[AppBlockDownloadIndex]);
	LargeFrameBuf[9] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 24);
	LargeFrameBuf[10] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 16);
	LargeFrameBuf[11] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex] >> 8);
	LargeFrameBuf[12] = (UCHAR)(AppBlockLengh[AppBlockDownloadIndex]);
	LargeFrameLength = 13;

	SendFF();

	CString PromptStr;
	PromptStr.Format("\r\nstart erase block %d ...\r\n",AppBlockDownloadIndex + 1);
	m_omPrompStr += PromptStr;
	PostMessage(WM_UDS_UPDATE, 1, 0);
}

/**********************************************************************************************************
Function Name  :   CheckProgramDependency_FE57
Input(s)       :   -
Output         :   -
Description    :   send 31 01 FF 01 request
Member of      :   CFBLHostDlg
Author(s)      :   ukign.zhou
Date Created   :   25.09.2017
**********************************************************************************************************/
void CUDSExtendWnd::CheckProgramDependency_FE57(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;

	SendSF(PHYSICAL, 4, data);
}

void CUDSExtendWnd::RequestWriteFingerPrint_H50(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x84;
	LargeFrameBuf[3] = 0xFE;
	LargeFrameBuf[4] = m_time.GetYear() % 100;
	LargeFrameBuf[5] = m_time.GetMonth();
	LargeFrameBuf[6] = m_time.GetDay();
	LargeFrameBuf[7] = 'c';
	LargeFrameBuf[8] = 'q';
	LargeFrameBuf[9] = 'r';
	LargeFrameBuf[10] = 'y';
	LargeFrameBuf[11] = 'k';
	LargeFrameBuf[12] = 'j';
	
	LargeFrameLength = 13;
	SendFF();
}

void CUDSExtendWnd::RequestIntergrity_H50(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xFF;
	LargeFrameBuf[3] = 0x01;
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		LargeFrameBuf[4] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex]);
	}
	else
	{
		LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);

		CString PromptStr;
		PromptStr.Format("chek CRC of block %d \r\n",AppBlockDownloadIndex + 1);
		m_omPrompStr += PromptStr;
		PostMessage(WM_UDS_UPDATE, 1, 0);
	}
	LargeFrameLength = 8;
	SendFF();
}

void CUDSExtendWnd::CheckProgramDependency_H50(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x02;

	SendSF(PHYSICAL, 4, data);
}

void CUDSExtendWnd::RequestRuntine(USHORT rid)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = (UCHAR)(rid >> 8);
	data[3] = (UCHAR)(rid);

	SendSF(PHYSICAL, 4, data);
}

void CUDSExtendWnd::RequestIntergrity_N800BEV(void)
{
	LargeFrameBuf[0] = 0x31;
	LargeFrameBuf[1] = 0x01;
	LargeFrameBuf[2] = 0xF0;
	LargeFrameBuf[3] = 0x01;
	if(m_CurrentTransfer == FLASH_DRIVER)
	{
		LargeFrameBuf[4] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(FlashDriverBlockCRC[FlashDriverDownloadIndex]);
	}
	else
	{
		LargeFrameBuf[4] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 24);
		LargeFrameBuf[5] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 16);
		LargeFrameBuf[6] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex] >> 8);
		LargeFrameBuf[7] = (UCHAR)(AppBlockCRC[AppBlockDownloadIndex]);

		CString PromptStr;
		PromptStr.Format("chek CRC of block %d \r\n",AppBlockDownloadIndex + 1);
		m_omPrompStr += PromptStr;
		PostMessage(WM_UDS_UPDATE, 1, 0);
	}
	LargeFrameLength = 8;
	SendFF();
}

void CUDSExtendWnd::RequestWriteFingerPrint_N800BEV(void)
{
	CTime m_time = CTime::GetCurrentTime();
	LargeFrameBuf[0] = 0x2E;
	LargeFrameBuf[1] = 0xF1;
	LargeFrameBuf[2] = 0x5A;
	LargeFrameBuf[3] = m_time.GetYear() % 100;
	LargeFrameBuf[4] = m_time.GetMonth();
	LargeFrameBuf[5] = m_time.GetDay();
	LargeFrameBuf[6] = 'c';
	LargeFrameBuf[7] = 'q';
	LargeFrameBuf[8] = 'r';
	LargeFrameBuf[9] = 'y';
	LargeFrameBuf[10] = 'k';
	LargeFrameBuf[11] = 'j';
	LargeFrameBuf[12] = 0;
	
	LargeFrameLength = 13;
	SendFF();
}

void CUDSExtendWnd::EraseAppRom_N800BEV(void)
{
	EraseAppRom_MA501();
}

void CUDSExtendWnd::CheckProgramDependency_N800BEV(void)
{
	UCHAR data[8];
	data[0] = 0x31;
	data[1] = 0x01;
	data[2] = 0xFF;
	data[3] = 0x01;

	SendSF(PHYSICAL, 4, data);
}

#define PROJECT_FUNCTION_END

/**********************************************************************************************************
Function Name  :   SendSimpleDiagnosticMessage
Input(s)       :   -
Output         :   -
Description    :   send a can frame.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :   12.05.2017
**********************************************************************************************************/
int CUDSExtendWnd::SendSimpleCanMessage(void)
{
	int nReturn = l_pouDIL_CAN_Interface->DILC_SendMsg(l_dwClientID, psSendCanMsgUds->m_psTxMsg[0]);
	return nReturn;
}

/**********************************************************************************************************
Function Name  :   GetSystemTimeMS
Input(s)       :   -
Output         :   -
Description    :   get the system time.
Member of      :   CUDSExtendWnd
Author(s)      :   ukign.zhou
Date Created   :   12.05.2017
**********************************************************************************************************/

int CUDSExtendWnd::GetSystemTimeMS(void)
{
	SYSTEMTIME CurrSysTimes;
	GetLocalTime(&CurrSysTimes);

	int nResult = (CurrSysTimes.wHour * 3600000 + CurrSysTimes.wMinute * 60000 + CurrSysTimes.wSecond) * 1000 + CurrSysTimes.wMilliseconds *1;
	return nResult;                  // Milliseconds
}

/**********************************************************************************************************
Function Name  :   SendSeveralCF
Input(s)       :   -
Output         :   -
Description    :   send CF frame in STmin,until BlockSize end.
Member of      :   CUDSExtendWnd

Author(s)      :   ukign.zhou
Date Created   :    15.05.2017
**********************************************************************************************************/
void CUDSExtendWnd::SendSeveralCF(void)
{
	if(CurrTPStep == TX_WAIT_CF)
	{
		TPTxBlockIndex = 0;
		while(LargeFrameFinishLength < LargeFrameLength)
		{
			if(TX_BlockSize == 0)
			{
				SendCF();
			}
			else if(TPTxBlockIndex < TX_BlockSize)
			{
				SendCF();
				TPTxBlockIndex++;
				if(TPTxBlockIndex >= TX_BlockSize)
				{
					CurrTPStep = TX_WAIT_FC;
					//SetTimer(ID_TIMER_TP_CF, 1, NULL);
					SetTimer(ID_TIMER_TP_EXT, 1000, NULL);
					break;
				}
			}
			int CurrentTime = GetSystemTimeMS();
			for( ;GetSystemTimeMS() < (CurrentTime  + TX_STmin); );
			
		}
		if (LargeFrameFinishLength >= LargeFrameLength)
		{
			CurrTPStep = TP_IDLE;			
		}
	}
}

/**********************************************************************************************************
Function Name  : RefreshProgress
Input(s)       : -
Output         : -
Functionality  : Called when you need refresh update percent;
Member of      : CUDSExtendWnd
Author(s)      : ukign.zhou
Date Created   : 12.05.2017
Modifications  :
/**********************************************************************************************************/
void CUDSExtendWnd::RefreshProgress(CurrentTransfer currentTransfer)
{
	m_omUpdatePercent.SetRange32(0, TotalDataOfByte + TotalDriverOfByte);
	m_omUpdatePercent.SetPos(CompleteDataOfByte);
	if(currentTransfer == FLASH_DRIVER)
	{
		m_omBlockPercent.SetRange32(0 , FlashDriverBlockLengh[FlashDriverDownloadIndex]);
		m_omBlockPercent.SetPos(BlockDataFinish);
	}
	else if(currentTransfer == APP_CODE)
	{
		m_omBlockPercent.SetRange32(0 , AppBlockLengh[AppBlockDownloadIndex]);
		m_omBlockPercent.SetPos(BlockDataFinish);
	}
}

/**********************************************************************************************************
Function Name  : OnTimer
Input(s)       : nIDEvent
Output         : -
Functionality  : Called during Timer events UDS send and receiving;
Member of      : CUDSExtendWnd
Author(s)      : ukign.zhou
Date Created   : 12.05.2017
Modifications  :
/**********************************************************************************************************/

void  CUDSExtendWnd::OnTimer(UINT_PTR nIDEvent)
{
	if(ID_TIMER_TP_EXT == nIDEvent)
	{
		CurrTPStep = TP_IDLE;
		KillTimer(ID_TIMER_TP_EXT);
	}
	else if(ID_TIMER_UPDATE == nIDEvent)
	{
		m_UpdateStep = UPDATE_IDLE;
		m_eUpdateStep = _UPDATE_IDLE;
		KillTimer(ID_TIMER_UPDATE);
	}
	else if(ID_TIMER_S3_SERVER == nIDEvent)
	{
		//SendTesterPresent(FUNCTIONAL , TRUE);
		IsNeedSend3EMessage = true;
		SetTimer(ID_TIMER_S3_SERVER, 2000, NULL);
	}
	else if(ID_TIMER_TP_CF == nIDEvent)
	{
		//SendSeveralCF();
		//IsNeedSendFC = true;
		//KillTimer(ID_TIMER_TP_CF);
	}
}

void CUDSExtendWnd::UpdateDelay(int timeMS)
{
	int time = GetSystemTimeMS();
	int realTime = time;
	while((time + timeMS) > realTime)
	{
		realTime = GetSystemTimeMS();
	}
}

LRESULT CUDSExtendWnd::OnUpdateMsgHandler(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 1)
	{
		omExtendWnd->UpdateData(FALSE);		
		SendDlgItemMessage(IDC_PROMPT_BOX , WM_VSCROLL , SB_BOTTOM,0);
	}
	else if (wParam == 2)
	{
		RefreshProgress(FLASH_DRIVER);
	}
	else if (wParam == 3)
	{
		RefreshProgress(APP_CODE);
	}
	else if (wParam == 4)
	{
		omExtendWnd->UpdateData(TRUE);		
	}
	return 0;
}

#if 0
void CUDSExtendWnd::OnWndMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	switch(uMsg)
	{
		case WM_UDS_UPDATE:
			if (wParam == 1)
			{
				omExtendWnd->UpdateData(FALSE);
			}
			break;
		default:
			break;
	}
}
#endif

void CUDSExtendWnd::OnCbnSelchangeProject()
{
	if(m_omProjectDiag.GetCurSel() == 0)//SGMW CN180S
	{
		m_omUDSReqID.vSetValue(0x751);
		m_omUDSResID.vSetValue(0x759);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);
		
		m_omLoaderDriver.EnableWindow(FALSE);
		m_omLoaderApp.EnableWindow(TRUE);
		//m_omStartUpdate.EnableWindow(FALSE);
	}
	else if(m_omProjectDiag.GetCurSel() == 1)//h50
	{
		m_omUDSReqID.vSetValue(0x705);
		m_omUDSResID.vSetValue(0x70D);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
		//m_omStartUpdate.EnableWindow(FALSE);
	}
	else if(m_omProjectDiag.GetCurSel() == 2)//R104
	{
		m_omUDSReqID.vSetValue(0x705);
		m_omUDSResID.vSetValue(0x70D);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(5);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 3)//HD10
	{
		m_omUDSReqID.vSetValue(0x18DA19FA);
		m_omUDSResID.vSetValue(0x18DAFA19);
		m_omUDSFunID.vSetValue(0x18DBFFFA);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(5);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 4)//EX50
	{
		m_omUDSReqID.vSetValue(0x708);
		m_omUDSResID.vSetValue(0x788);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 5)//F516
	{
		m_omUDSReqID.vSetValue(0x726);
		m_omUDSResID.vSetValue(0x7A6);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(5);

		m_omLoaderDriver.EnableWindow(FALSE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 6)//MA501
	{
		m_omUDSReqID.vSetValue(0x752);
		m_omUDSResID.vSetValue(0x75A);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 7)//NL-4DC
	{
		m_omUDSReqID.vSetValue(0x7C6);
		m_omUDSResID.vSetValue(0x7CE);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 8)//DX3-EV
	{
		m_omUDSReqID.vSetValue(0x733);
		m_omUDSResID.vSetValue(0x734);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 9)//F507_HEV
	{
		m_omUDSReqID.vSetValue(0x708);
		m_omUDSResID.vSetValue(0x788);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 10)//FE-5B-7B
	{
		m_omUDSReqID.vSetValue(0x7C6);
		m_omUDSResID.vSetValue(0x7CE);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 11)//H50
	{
		m_omUDSReqID.vSetValue(0x726);
		m_omUDSResID.vSetValue(0x7A6);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
	else if(m_omProjectDiag.GetCurSel() == 12)//N800_BEV
	{
		m_omUDSReqID.vSetValue(0x741);
		m_omUDSResID.vSetValue(0x749);
		m_omUDSFunID.vSetValue(0x7DF);
		m_omBlockSize.vSetValue(0);
		m_omSTmin.vSetValue(1);

		m_omLoaderDriver.EnableWindow(TRUE);
		m_omLoaderApp.EnableWindow(TRUE);
	}
}
