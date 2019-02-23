#ifndef DEVICE_INTERFACE_H
#define DEVICE_INTERFACE_H

#include <linux_nfc_api.h>

typedef void T4T_NDEF_EMU_Callback_t (unsigned char*, unsigned short);

static void T4T_NDEF_EMU_FillRsp (unsigned char *pRsp, unsigned short offset, unsigned char length);

void T4T_NDEF_EMU_SetRecord(unsigned char *pRecord, unsigned short Record_size, T4T_NDEF_EMU_Callback_t *cb);

void T4T_NDEF_EMU_Reset(void);

void T4T_NDEF_EMU_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *pRsp, unsigned short *pRsp_size);

void onDataReceived(unsigned char *data, unsigned int data_length);

void onHostCardEmulationActivated(unsigned char mode);

void onHostCardEmulationDeactivated();
void onTagArrival(nfc_tag_info_t *pTagInfo);
void onTagDeparture(void);

void onDeviceArrival (void);
void onDeviceDeparture (void);
void onSnepClientReady();
void onSnepClientClosed();

#endif
