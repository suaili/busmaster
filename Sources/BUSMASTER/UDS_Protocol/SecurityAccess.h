#pragma once

#include "afxcmn.h"

const UINT nc_uds_keymul[32] = { 

0x7678,0x9130,0xd753,0x750f,0x72cb,0x55f7,0x13da,0x786b,
                                         

0x372a,0x4932,0x0e7c,0x3687,0x3261,0xa82c,0x8935,0xd00c,
                                         

0x1995,0x4311,0xb854,0x0d8d,0x9863,0x1a21,0xf753,0xd6d3,
                                         

0xb15d,0x7f3d,0x6821,0x791c,0x26c5,0x2e37,0x0e69,0x64a0 };

	
const UINT m_NCDefaultSeed = 0xa548fd85;
const UINT m_NCUDSKeyMaskLv1 = 0x45672456;
const UINT m_NCUDSKeyMaskLv3 = 0x553567DC;

class CSecurityAccess
{
public:
	CSecurityAccess(void);
	~CSecurityAccess(void);

private:
	static UINT32 CroLeft(UINT32 c, UINT32 b);
	static USHORT CroShortRight(USHORT c, USHORT b);
	static UINT32 Mulu32(UINT mask,UINT32 val1,UINT32 val2);
	static UINT32 SwapBit(UINT32 source);

public:
	static UINT32 CN210CalculateKey(UCHAR level, UINT32 seed);
	static UINT32 FawCalculateKey(UCHAR *seed, UINT length, UCHAR *key, UINT *retLen);
	static UINT32 JMCCalculateKeyApp(UINT32 seed);
	static UINT32 JMCCalculateKeyFBL(UINT32 seed);
	static UINT32 F507CalculateKey(UINT32 seed);
	static UINT32 BAICH50CalculateKeyLevel1(UINT32 seed);
	static UINT32 BAICH50CalculateKeyLevel2(UINT32 seed);
	static UINT32 HD10CalculateKey(UINT32 seed,UCHAR level);
	static UINT32 C301CalculateKey(UINT32 seed,UCHAR level);
	static UINT32 R104CalculateKey(UINT32 seed , UCHAR level);
	static UINT32 FactorySecuritySeedToKey(UINT32 seed);
	static UINT32 F516SeedToKeyLevelFBL(UINT32 seed);
	static UINT32 MA501SeedToKeyLevelFBL(UINT32 seed );
	static UINT32 Ex50CalcKey(UINT32 seed);
	static UINT32 FE_5B_7B_SeedToKey(UINT32 seed );
};

