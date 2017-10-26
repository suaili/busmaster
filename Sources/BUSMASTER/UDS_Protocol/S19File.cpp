#include "S19File.h"


CS19File::CS19File(void)
{
	BinBlockCount = 0;
}

CS19File::CS19File(CString FilePathName)
{
	ParseFile(FilePathName);
}


void CS19File::ParseFile(CString fileName)
{
	ParseFinish = true;
	CFile S19File(fileName,CFile::modeRead);
	char fileBbuff[524288];
	ULONG t_fileLength = S19File.GetLength();
	UINT length = 0;
	UINT offAddress = 0;
	char lineData[300],lineChecksum;
	UINT index = 0;

	UCHAR tempValue[4];
	UINT i;
	UCHAR header[600];
	UCHAR AddressLength;
	static UINT PrevAddr = 0;
	CString BlockInfo;

	if(t_fileLength <= 524288)
	{
		BinBlockCount = 0;
		S19File.Read(fileBbuff,t_fileLength);
		while(index < t_fileLength)
		{
			if(fileBbuff[index++] == 'S')
			{
				if(fileBbuff[index] == '0')//start of file
				{
					index++;
					lineChecksum = 0;
					
					if(sscanf(fileBbuff + index,"%2x",&length) != 1)
					{
						break;
					}
					lineChecksum = length;
					index += 2;

					if(sscanf(fileBbuff + index,"%2x",tempValue) != 1)
					{
						break;
					}
					lineChecksum += tempValue[0];
					index += 2;

					if(sscanf(fileBbuff + index,"%2x",tempValue) != 1)
					{
						break;
					}
					lineChecksum += tempValue[0];
					index += 2;
							
					for(i = 0; i < length - 3 ; i ++)
					{
						if(sscanf(fileBbuff + index,"%2x",header + i) != 1)
						{
							break;
						}
						lineChecksum += header[i];
						index += 2;
					}
					
							
					if(sscanf(fileBbuff + index,"%2x",tempValue) != 1)
					{
						break;
					}
					lineChecksum += tempValue[0];
					index += 2;
					if((lineChecksum & 0xFF) !=0xFF)
					{
						break;
					}

					header[i] = '\r';
					header[i + 1] = '\n';
					header[i + 2] = '\0';

					FileHeader = header;

					if(fileBbuff[index] == '\r' && fileBbuff[index + 1] == '\n')
					{
						index += 2;
					}
					else
					{
						break;
					}
				}
				else if(fileBbuff[index] == '1' || fileBbuff[index] == '2' || fileBbuff[index] == '3')//data record with 2 3 4 bytes address
				{
					AddressLength = fileBbuff[index] - 0x30 + 1;
					
														
					index++;
					lineChecksum = 0;
					if(sscanf(fileBbuff + index,"%2x",&length) != 1)
					{
						break;
					}
					lineChecksum = length;
					index += 2;

					if(AddressLength == 2)
					{
						if(sscanf(fileBbuff + index,"%4x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						index += 4;
					}
					else if(AddressLength == 3)
					{
						if(sscanf(fileBbuff + index,"%6x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						lineChecksum += ((offAddress >> 16) & 0xFF);
						index += 6;
					}
					else if(AddressLength == 4)
					{
						if(sscanf(fileBbuff + index,"%8x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						lineChecksum += ((offAddress >> 16) & 0xFF);
						lineChecksum += ((offAddress >> 24) & 0xFF);
						index += 8;
					}
							
					if(BinBlockCount == 0)//first block
					{
						BinBlockLengh[0] = 0;
						BinStartAddr[0] = PrevAddr = offAddress;
						BinBlockCount = 1;

						PrevAddr += length - AddressLength - 1;
					}
					else 
					{
						if(PrevAddr != offAddress)
						{
							BinBlockLengh[BinBlockCount] = 0;
							BinStartAddr[BinBlockCount] = PrevAddr = offAddress;
							BinBlockCount++;							
							PrevAddr += length - AddressLength - 1;
						}
						else
						{									
							PrevAddr += length - AddressLength - 1;
						}
					}
							
					for(i = 0; i < length - AddressLength -1 ; i ++)
					{
						if(sscanf(fileBbuff + index,"%2x",lineData + i) != 1)
						{
							break;
						}
						lineChecksum += lineData[i];
						index += 2;
					}

					memcpy((BinBlockData[BinBlockCount - 1] + BinBlockLengh[BinBlockCount - 1]) , lineData , (length - AddressLength -1));

					if(sscanf(fileBbuff + index,"%2x",tempValue) != 1)
					{
						break;
					}
					lineChecksum += tempValue[0];
					index += 2;
					if((lineChecksum & 0xFF) !=0xFF)
					{
						break;
					}

					if(fileBbuff[index] == '\r' && fileBbuff[index + 1] == '\n')
					{
						index += 2;
						BinBlockLengh[BinBlockCount - 1] +=  length - AddressLength -1;
					}
					else
					{
						break;
					}
				}
				else if(fileBbuff[index] == '5')// 2 bytes address,contain all S1 S2 S3 record count,no data record this type
				{
					index++;
				}
				else if(fileBbuff[index] == '7' || fileBbuff[index] == '8' || fileBbuff[index] == '9')//end of file, 4 3 2 bytes address means program start address
				{
					AddressLength = '9' - fileBbuff[index] + 2;
					index++;
					
					if(sscanf(fileBbuff + index,"%2x",&length) != 1)
					{
						break;
					}
					lineChecksum = length;
					index += 2;

					if(AddressLength == 2)
					{
						if(sscanf(fileBbuff + index,"%4x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						index += 4;
					}
					else if(AddressLength == 3)
					{
						if(sscanf(fileBbuff + index,"%6x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						lineChecksum += ((offAddress >> 16) & 0xFF);
						index += 6;
					}
					else if(AddressLength == 4)
					{
						if(sscanf(fileBbuff + index,"%8x",&offAddress) != 1)
						{
							break;
						}
						lineChecksum += (offAddress & 0xFF);
						lineChecksum += ((offAddress >> 8) & 0xFF);
						lineChecksum += ((offAddress >> 16) & 0xFF);
						lineChecksum += ((offAddress >> 24) & 0xFF);
						index += 8;
					}

					if(sscanf(fileBbuff + index,"%2x",tempValue) != 1)
					{
						break;
					}
					lineChecksum += tempValue[0];
					index += 2;
					if((lineChecksum & 0xFF) !=0xFF)
					{
						break;
					}

					if(fileBbuff[index] == '\r' && fileBbuff[index + 1] == '\n')
					{
						index += 2;
						ParseFinish = true;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
	}
}


CS19File::~CS19File(void)
{

}

UCHAR CS19File::GetBlockCount(void)
{
	return BinBlockCount;
}

CString CS19File::GetFileHeader(void)
{
	return FileHeader;
}

UCHAR* CS19File::GetBlockData(UCHAR BlockIndex)
{
	if(BlockIndex < BinBlockCount)
	{
		return BinBlockData[BlockIndex];
	}
	else
	{
		return NULL;
	}
}

UINT CS19File::GetBlockLength(UCHAR BlockIndex)
{
	if(BlockIndex < BinBlockCount)
	{
		return BinBlockLengh[BlockIndex];
	}
	else
	{
		return 0;
	}
}

UINT CS19File::GetBlockStartAddr(UCHAR BlockIndex)
{
	if(BlockIndex < BinBlockCount)
	{
		return BinStartAddr[BlockIndex];
	}
	else
	{
		return 0;
	}
}

bool CS19File::IsParseFinish(void)
{
	return ParseFinish;
}