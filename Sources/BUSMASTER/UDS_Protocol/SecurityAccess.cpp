#include "SecurityAccess.h"
#include "HD10_AC.h"

CSecurityAccess::CSecurityAccess(void)
{

}


CSecurityAccess::~CSecurityAccess(void)
{
}

UINT32 CSecurityAccess::CroLeft(UINT32 c, UINT32 b)
{
	UINT32 left=c<<b;
	UINT32 right=c>>(32-b);  
	UINT32 croleftvalue=left|right;  
	return croleftvalue;  
}

USHORT CSecurityAccess::CroShortRight(USHORT c, USHORT b)
{
	USHORT right=c>>b;
	USHORT left=c<<(16-b);  
	USHORT crorightvalue=left|right;  
	return crorightvalue;
}
	
UINT32 CSecurityAccess::Mulu32(UINT mask,UINT32 val1,UINT32 val2)
{
	UINT32 x,y,z,p;
	x = (val1&mask)|((~val1)&val2);
	y = ((CroLeft(val1,1))&(CroLeft(val2,14)))|((CroLeft(mask,21))&(~(CroLeft(val1,30))));
	z = (CroLeft(val1,17))^(CroLeft(val2,4))^(CroLeft(mask,11));
	p = x^y^z;
	return p;
}


/**********************************************************************************************************
 Function Name  :   UDSUpdateThread
 Input(s)       :   level 1:lv1 3:lv3,   seed 
 Output         :   key  
 Description    :   service 27 will use this Algorithm
 Member of      :   CSecurityAccess

 Author(s)      :    ukign.zhou
 Date Created   :   19.05.2016
**********************************************************************************************************/
UINT32 CSecurityAccess::CN210CalculateKey(UCHAR level,UINT32 seed)
{
	UINT32 temp;
	USHORT index;
	USHORT mult1;
	USHORT mult2;
	UINT32 mask;

	if(seed == 0)
	{
		seed = m_NCDefaultSeed;
	}

	if(level == 1)
	{
		mask = m_NCUDSKeyMaskLv1;
	}
	else if(level == 3)
	{
		mask = m_NCUDSKeyMaskLv3;
	}

	for (index=0x5D39, temp=0x80000000; temp; temp>>=1)
	{
		if (temp & seed)
		{
			index = CroShortRight(index, 1);
			if (temp & mask)
			{
				index ^= 0x74c9;
			}
		}
	}
	mult1 = (nc_uds_keymul[(index>>2) & ((1<<5)-1)]^index);
	mult2 = (nc_uds_keymul[(index>>8) & ((1<<5)-1)]^index);
	temp = (((unsigned long)mult1)<<16)|((unsigned long)mult2);
	temp = Mulu32(mask,seed,temp);
	return temp;
}

UINT32 CSecurityAccess::FawCalculateKey(UCHAR *seed, UINT length, UCHAR *key, UINT *retLen)
{
	UINT i;
	union { 
		UCHAR byte[4];
		UINT wort;
	} seedlokal;
	const UINT mask = 0x17244617;
	/* The array for SEED starts with [1], the array for KEY starts with [0] */
	/* seed contains the SEED from the ECU */
	/* length contains the number of bytes of seed */
	/* key contains the KEY to send to ECU */
	/* retLen contains the number of bytes to send to ECU as key */
	if (seed[1] == 0 && seed[2] == 0)
	{
		*retLen = 0;
	}
	else
	{
		seedlokal.wort = ((UINT)seed[1] << 24) + ((UINT)seed[2] << 16) +((UINT)seed[3] << 8) + (UINT)seed[4];
		for (i=0; i<35; i++)
		{
			if (seedlokal.wort & 0x80000000)
			{
				seedlokal.wort = seedlokal.wort << 1;
				seedlokal.wort = seedlokal.wort ^ mask;
			}
			else 
			{
				seedlokal.wort = seedlokal.wort << 1; 
			}
		}
		for (i=0; i<4; i++)
		{
			key[i] = seedlokal.byte[i]; /* A */
			//key[3-i] = seedlokal.byte[i];/* B */
		} 
		*retLen = length - 1;
	}
	return TRUE;
}

UINT32 CSecurityAccess::JMCCalculateKeyApp(UINT32 seed)
{
	UINT32 i;
	union { 
		UINT8 byte[4];
		UINT32 wort;
	} seedlokal;
	const UINT32 mask = 0x8E9BBD8E;
	UINT32 key;
	UINT8* keyPtr = (UINT8*)(&key);
	
	if (seed <= 0xFFFF)
	{
		return 0x12345678;
	}
	else
	{
		seedlokal.wort = seed;
		for (i=0; i<35; i++)
		{
			if (seedlokal.wort & 0x80000000)
			{
				seedlokal.wort = seedlokal.wort << 1;
				seedlokal.wort = seedlokal.wort ^ mask;
			}
			else 
			{
				seedlokal.wort = seedlokal.wort << 1; 
			}
		}
		for (i=0; i<4; i++)
		{
			keyPtr[i] = seedlokal.byte[i]; /* A */
			//keyPtr[3-i] = seedlokal.byte[i];/* B */
		} 
		
	}
	return key;
}

UINT32 CSecurityAccess::JMCCalculateKeyFBL(UINT32 seed)
{
	UINT32 i;
	union { 
		UINT8 byte[4];
		UINT32 wort;
	} seedlokal;
	const UINT32 mask = 0x88B7A3F2;
	UINT32 key;
	UINT8* keyPtr = (UINT8*)(&key);
	
	if (seed <= 0xFFFF)
	{
		return 0x12345678;
	}
	else
	{
		seedlokal.wort = seed;
		for (i=0; i<35; i++)
		{
			if (seedlokal.wort & 0x80000000)
			{
				seedlokal.wort = seedlokal.wort << 1;
				seedlokal.wort = seedlokal.wort ^ mask;
			}
			else 
			{
				seedlokal.wort = seedlokal.wort << 1; 
			}
		}
		for (i=0; i<4; i++)
		{
			keyPtr[i] = seedlokal.byte[i]; /* A */
			//keyPtr[3-i] = seedlokal.byte[i];/* B */
		} 
		
	}
	return key;
}

UINT32 CSecurityAccess::F507CalculateKey(UINT32 seed)
{
	UCHAR cal[4];
	UCHAR xor[] = {0x41, 0x16 , 0x71 , 0x24};
	UCHAR key[4];
	UCHAR tempSeed[4];
	UCHAR index;
	tempSeed[0] = (UCHAR)(seed >> 24);
	tempSeed[1] = (UCHAR)(seed >> 16);
	tempSeed[2] = (UCHAR)(seed >> 8);
	tempSeed[3] = (UCHAR)(seed);
	for(index = 0; index < 4; index++)
	{
		cal[index] = tempSeed[index] ^ xor[index];
	}

	key[0] = ((cal[0]&0x0f) << 4)|(cal[1]&0xf0);
	key[1] = ((cal[1]&0x0f) << 4)|((cal[2]&0xf0) >> 4);
	key[2] = (cal[2]&0xf0)|((cal[3]&0xf0) >> 4);
	key[3] = ((cal[3]&0x0f) << 4)|(cal[0]&0x0f);

	return (key[0] << 24) + (key[1] << 16) + (key[2] << 8) + key[3];
}

UINT32 CSecurityAccess::F516SeedToKeyLevelFBL(UINT32 seed)
 {
  	UCHAR cal[4];
  	UCHAR xor[] = {0x41, 0x16 , 0x71 , 0x24};
  	UCHAR key[4];
  	UCHAR tempSeed[4];
  	UCHAR index;
  	tempSeed[0] = (UCHAR)(seed >> 24);
  	tempSeed[1] = (UCHAR)(seed >> 16);
  	tempSeed[2] = (UCHAR)(seed >> 8);
  	tempSeed[3] = (UCHAR)(seed);
  	for(index = 0; index < 4; index++)
  	{
  		cal[index] = tempSeed[index] ^ xor[index];
  	}

  	key[0] = ((cal[0]&0x0f) << 4)|(cal[1]&0x0f);
  	key[1] = ((cal[1]&0xf0) >> 4)|((cal[2]&0x0f) << 4);
  	key[2] = ((cal[2]&0xf0) >> 4)|(cal[3]&0xf0);
  	key[3] = (cal[3]&0x0f)|((cal[0]&0xf0) >> 4);

  	return (key[0] << 24) + (key[1] << 16) + (key[2] << 8) + key[3];
  	//return *((uint32_t*)key);
 }

UINT32 CSecurityAccess::BAICH50CalculateKeyLevel1(UINT32 seed)
{
	UINT key1,key2,seed2;
	UINT AppKeyConst = 0x48793953;
	key1 = seed ^ AppKeyConst;
	seed2 = (seed >> 16) | (seed << 16);//交换12<->34 ，变为3412
	seed2 = ((seed2 >> 8) & 0x00FF00FF) | ((seed2 << 8) & 0xFF00FF00);//字节1<->2, 3<->4，变为4321
	seed2 = ((seed2 >> 4) & 0x0F0F0F0F) | ((seed2 << 4) & 0xF0F0F0F0);//字节内高四位与低四位交换
	seed2 = ((seed2 >> 2) & 0x33333333) | ((seed2 << 2) & 0xCCCCCCCC);//字节内位交换12<->34,56<->78
	seed2 = ((seed2 >> 1) & 0x55555555) | ((seed2 << 1) & 0xAAAAAAAA);//字节内位交换1<->2,3<->4
	seed2 = ((seed2 + 0x53000000) & 0xFF000000) + ((seed2 + 0x390000) & 0xFF0000) + ((seed2 + 0x7900) & 0xFF00) + ((seed2 + 0x48) & 0xFF);
	key2 = seed2 ^ AppKeyConst;
	return (key1 + key2);
}

UINT32 CSecurityAccess::BAICH50CalculateKeyLevel2(UINT32 seed)
{
	UINT key1,key2,seed2;
	UINT AppKeyConst = 0x686B5954;
	key1 = seed ^ AppKeyConst;
	seed2 = (seed >> 16) | (seed << 16);//交换12<->34 ，变为3412
	seed2 = ((seed2 >> 8) & 0x00FF00FF) | ((seed2 << 8) & 0xFF00FF00);//字节1<->2, 3<->4，变为4321
	seed2 = ((seed2 >> 4) & 0x0F0F0F0F) | ((seed2 << 4) & 0xF0F0F0F0);//字节内高四位与低四位交换
	seed2 = ((seed2 >> 2) & 0x33333333) | ((seed2 << 2) & 0xCCCCCCCC);//字节内位交换12<->34,56<->78
	seed2 = ((seed2 >> 1) & 0x55555555) | ((seed2 << 1) & 0xAAAAAAAA);//字节内位交换1<->2,3<->4
	seed2 = ((seed2 + 0x53000000) & 0xFF000000) + ((seed2 + 0x390000) & 0xFF0000) + ((seed2 + 0x7900) & 0xFF00) + ((seed2 + 0x48) & 0xFF);
	key2 = seed2 ^ AppKeyConst;
	return (key1 + key2);
}

UINT32 CSecurityAccess::SwapBit(UINT32 source)
{
	UINT32 dest;
	dest = (source >> 16) | (source << 16);//交换12<->34 ，变为3412
	dest = ((dest >> 8) & 0x00FF00FF) | ((dest << 8) & 0xFF00FF00);//字节1<->2, 3<->4，变为4321
	dest = ((dest >> 4) & 0x0F0F0F0F) | ((dest << 4) & 0xF0F0F0F0);//字节内高四位与低四位交换
	dest = ((dest >> 2) & 0x33333333) | ((dest << 2) & 0xCCCCCCCC);//字节内位交换12<->34,56<->78
	dest = ((dest >> 1) & 0x55555555) | ((dest << 1) & 0xAAAAAAAA);//字节内位交换1<->2,3<->4
	
	return dest;
}

UINT32 CSecurityAccess::HD10CalculateKey(UINT32 seed,UCHAR level)
{
	UCHAR key[4] = {0,0,0,0};
	UCHAR seed1[4];
	seed1[0] = (UCHAR)(seed >> 24);
	seed1[1] = (UCHAR)(seed >> 16);
	seed1[2] = (UCHAR)(seed >> 8);
	seed1[3] = (UCHAR)(seed);
	unsigned int keyLen = 2;
	CString name = "level 1";
	HMODULE hDll = LoadLibrary("HD10_AC.dll");
	if (hDll != NULL)
	{
		keyGen keygen = (keyGen)GetProcAddress(hDll, "GenerateKeyEx");	//函数
		if (keygen != NULL)
		{
			keygen(seed1,2,level,name,key,2,*&keyLen);
		}
		FreeLibrary(hDll);
	}
	return (key[0] << 24) + (key[1] << 16) + (key[2] << 8) + key[3];
}

UINT32 CSecurityAccess::C301CalculateKey(UINT32 seed , UCHAR level)
{
	UCHAR Key1[4];
	UCHAR Key2[4];
	UINT32 Key;
	UINT32 Seed2;
	UCHAR AppKeyConst[4] = {0x7C , 0xE4 , 0x12 , 0x12};

	Seed2 = SwapBit(seed);

	Key1[0] = (seed >> 24) ^ AppKeyConst[0];
	Key1[1] = (seed >> 16) ^ AppKeyConst[1];
	Key1[2] = (seed >> 8) ^ AppKeyConst[2];
	Key1[3] = (seed) ^ AppKeyConst[3];

	Key2[0] = (Seed2 >> 24) ^ AppKeyConst[0];
	Key2[1] = (Seed2 >> 16) ^ AppKeyConst[1];
	Key2[2] = (Seed2 >> 8) ^ AppKeyConst[2];
	Key2[3] = (Seed2) ^ AppKeyConst[3];

	Key = ((Key1[0] << 24) + (Key1[1] << 16) + (Key1[2] << 8) + Key1[3]) + ((Key2[0] << 24) + (Key2[1] << 16) + (Key2[2] << 8) + Key2[3]);
	
	return Key;
}

UINT32 CSecurityAccess::R104CalculateKey(UINT32 seed , UCHAR level)
{
	UCHAR Key1[4];
	UCHAR Key2[4];
	UINT32 Key;
	UINT32 Seed2;
	UCHAR AppKeyConst[4];// = {0x7C , 0xE4 , 0x12 , 0x12};
	
	if(level == 0)
	{
		AppKeyConst[0] = 0x1E;
		AppKeyConst[1] = 0x2D;
		AppKeyConst[2] = 0x7F;
		AppKeyConst[3] = 0x3B;
	}
	else if(level == 2)
	{
		AppKeyConst[0] = 0x83;
		AppKeyConst[1] = 0x94;
		AppKeyConst[2] = 0xA2;
		AppKeyConst[3] = 0xD6;
	}

	Seed2 = SwapBit(seed);

	Key1[0] = ((UCHAR)(seed >> 24)) ^ AppKeyConst[0];
	Key1[1] = ((UCHAR)(seed >> 16)) ^ AppKeyConst[1];
	Key1[2] = ((UCHAR)(seed >> 8)) ^ AppKeyConst[2];
	Key1[3] = ((UCHAR)(seed)) ^ AppKeyConst[3];

	Key2[0] = ((UCHAR)(Seed2 >> 24)) ^ AppKeyConst[0];
	Key2[1] = ((UCHAR)(Seed2 >> 16)) ^ AppKeyConst[1];
	Key2[2] = ((UCHAR)(Seed2 >> 8)) ^ AppKeyConst[2];
	Key2[3] = ((UCHAR)(Seed2)) ^ AppKeyConst[3];

	Key = ((Key1[0] << 24) + (Key1[1] << 16) + (Key1[2] << 8) + Key1[3]) + ((Key2[0] << 24) + (Key2[1] << 16) + (Key2[2] << 8) + Key2[3]);
	
	return Key;
}

UINT32  CSecurityAccess::FactorySecuritySeedToKey(UINT32 seed)
{
	UCHAR i;
	UINT32 xor = 0x52756959;// asscii "RuiY"
	UINT32 key;

	key = seed ^ xor;

	for (i=0; i < 32; i++)
	{
		if (key & 0x80000000)
		{
			key = key << 1;
			key = key ^ xor;
		}
		else 
		{
			key = key << 1; 
		}
	}

	return key;
}

UINT32 CSecurityAccess::MA501SeedToKeyLevelFBL(UINT32 seed )
{
	UCHAR i = 0;
	UINT32 k = 0;
	UINT32 mask_JunMa_MA501 = 0xF8410221;

	if (!((seed == 0) || (seed == 0xFFFFFFFF)))
	{
		for (i=0; i<35; i++)
		{
			if (seed & 0x80000000)
			{
				seed = seed <<1;
				seed = seed ^ mask_JunMa_MA501;
			}
			else
			{
				seed = seed <<1;
			}
			k = seed;
		}
	}
	return  (k);
}

UINT32 CSecurityAccess::Ex50CalcKey(UINT32 seed)
{
	UINT32 SK[4] = {0x59474d4f,0x4b4c4446,0x45583530,0x5544534b};
	UINT32 v0 = seed;
	UINT32 v1 = ~seed;
	UINT32 sum = 0,i;
	UINT32 delta = 0x9e3779b9;
	UINT32 result;

	for(i=0;i<2;i++)
	{
		v0  += (((v1 << 4)^(v1 >> 5)) + v1)^(sum + SK[sum & 3]);
		sum += delta;
		v1  += (((v0 << 4)^(v0 >> 5)) + v0)^(sum + SK[(sum >> 11)& 3]);
	}
	result = (v0 & 0xffff);
	result = (result<<16) + (v1 & 0xffff);

	return result;
}

UINT32 CSecurityAccess::FE_5B_7B_SeedToKey(UINT32 seed )
{
	UCHAR i = 0;
	UCHAR xorarray[4] = {0x65, 0x67, 0x77, 0xE9}; 
	UCHAR seedarray[4] = {0};
	UCHAR caldata[4] = {0};
	UCHAR returnkey[4] = {0};

	seedarray[0] = (UCHAR)(seed >> 24);
	seedarray[1] = (UCHAR)(seed >> 16);
	seedarray[2] = (UCHAR)(seed >> 8);
	seedarray[3] = (UCHAR)(seed);

	for(i = 0; i < 4; i++)
	{
	    caldata[i] = seedarray[i] ^ xorarray[i];
	}

	returnkey[0] = (((caldata[2] & 0x03) << 6) | ((caldata[3] & 0xFC) >> 2));
	returnkey[1] = (((caldata[3] & 0x03) << 6) | (caldata[0] & 0x3F));
	returnkey[2] = ((caldata[0] & 0xFC) | ((caldata[1] & 0xC0) >> 6));
	returnkey[3] = ((caldata[1] & 0xFC) | (caldata[2] & 0x03));

	return (((UINT32)returnkey[0] << 24) | ((UINT32)returnkey[1] << 16) | ((UINT32)returnkey[2] << 8) | ((UINT32)returnkey[3]));
}

