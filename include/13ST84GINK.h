#ifndef _13ST84GINK_H
#define _13ST84GINK_H

void VFDWriteData(unsigned char dat);
void VFDPowerCtrl(unsigned char set);
void SetLuminance(unsigned char lum);
void VFDDisplaySwitch(unsigned char set);
void VFDDStandby(unsigned char set);
void VFDInit(unsigned char slum);
void WriteCGRAM(unsigned char x, char (*arr)[5], unsigned char n);
void WriteDisplayCMD();
void VFDWriteOneDIYChar(unsigned char x, unsigned char chr);
void VFDWriteOneDIYCharAndShow(unsigned char x, unsigned char chr);
void VFDWriteStr(unsigned char x, char* str);
void VFDWriteStrAndShow(unsigned char x, char* str);
void VFDWriteOneADRAMAndShow(unsigned char x, unsigned char dat);
void VFDWriteAllADRAMAndShow(unsigned char* dat);

#endif
