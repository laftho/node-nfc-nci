#include <stdlib.h>
#include <string.h>

#include "deviceinterface.h"
#include "tools.h"
#include <linux_nfc_api.h>
#include "state.h"

const unsigned char T4T_NDEF_EMU_APP_Select[] = {0x00,0xA4,0x04,0x00,0x07,0xD2,0x76,0x00,0x00,0x85,0x01,0x01};
const unsigned char T4T_NDEF_EMU_CC[] = {0x00, 0x0F, 0x20, 0x00, 0xFF, 0x00, 0xFF, 0x04, 0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0xFF};
const unsigned char T4T_NDEF_EMU_CC_Select[] = {0x00,0xA4,0x00,0x0C,0x02,0xE1,0x03};
const unsigned char T4T_NDEF_EMU_NDEF_Select[] = {0x00,0xA4,0x00,0x0C,0x02,0xE1,0x04};
const unsigned char T4T_NDEF_EMU_Read[] = {0x00,0xB0};
const unsigned char T4T_NDEF_EMU_OK[] = {0x90, 0x00};
const unsigned char T4T_NDEF_EMU_NOK[] = {0x6A, 0x82};

unsigned char *pT4T_NdefRecord = NULL;
unsigned short T4T_NdefRecord_size = 0;

static State::T4T_NDEF_EMU_state_t eT4T_NDEF_EMU_State = State::Ready;
static T4T_NDEF_EMU_Callback_t *pT4T_NDEF_EMU_PushCb = NULL;

/********************************** HCE **********************************/
static void T4T_NDEF_EMU_FillRsp (unsigned char *pRsp, unsigned short offset, unsigned char length)
{
    if (offset == 0)
    {
        pRsp[0] = (T4T_NdefRecord_size & 0xFF00) >> 8;
        pRsp[1] = (T4T_NdefRecord_size & 0x00FF);
        memcpy(&pRsp[2], &pT4T_NdefRecord[0], length-2);
    }
    else if (offset == 1)
    {
        pRsp[0] = (T4T_NdefRecord_size & 0x00FF);
        memcpy(&pRsp[1], &pT4T_NdefRecord[0], length-1);
    }
    else
    {
        memcpy(pRsp, &pT4T_NdefRecord[offset-2], length);
    }
    /* Did we reached the end of NDEF record ?*/
    if ((offset + length) >= (T4T_NdefRecord_size + 2))
    {
        /* Notify application of the NDEF send */
        if(pT4T_NDEF_EMU_PushCb != NULL) pT4T_NDEF_EMU_PushCb(pT4T_NdefRecord, T4T_NdefRecord_size);
    }
}

void T4T_NDEF_EMU_SetRecord(unsigned char *pRecord, unsigned short Record_size, T4T_NDEF_EMU_Callback_t *cb)
{
    pT4T_NdefRecord = pRecord;
    T4T_NdefRecord_size = Record_size;
    pT4T_NDEF_EMU_PushCb =  cb;
}
void T4T_NDEF_EMU_Reset(void)
{
    eT4T_NDEF_EMU_State = State::Ready;
}
void T4T_NDEF_EMU_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *pRsp, unsigned short *pRsp_size)
{
    unsigned char eStatus = 0x00;
    if (!memcmp(pCmd, T4T_NDEF_EMU_APP_Select, sizeof(T4T_NDEF_EMU_APP_Select)))
    {
        *pRsp_size = 0;
        eStatus = 0x01;
        eT4T_NDEF_EMU_State = State::NDEF_Application_Selected;
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_CC_Select, sizeof(T4T_NDEF_EMU_CC_Select)))
    {
        if(eT4T_NDEF_EMU_State == State::NDEF_Application_Selected)
        {
            *pRsp_size = 0;
            eStatus = 0x01;
            eT4T_NDEF_EMU_State = State::CC_Selected;
        }
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_NDEF_Select, sizeof(T4T_NDEF_EMU_NDEF_Select)))
    {
        *pRsp_size = 0;
        eStatus = 0x01;
        eT4T_NDEF_EMU_State = State::NDEF_Selected;
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_Read, sizeof(T4T_NDEF_EMU_Read)))
    {
        if(eT4T_NDEF_EMU_State == State::CC_Selected)
        {
            memcpy(pRsp, T4T_NDEF_EMU_CC, sizeof(T4T_NDEF_EMU_CC));
            *pRsp_size = sizeof(T4T_NDEF_EMU_CC);
            eStatus = 0x01;
        }
        else if (eT4T_NDEF_EMU_State == State::NDEF_Selected)
        {
            unsigned short offset = (pCmd[2] << 8) + pCmd[3];
            unsigned char length = pCmd[4];
            if(length <= (T4T_NdefRecord_size + offset + 2))
            {
                T4T_NDEF_EMU_FillRsp(pRsp, offset, length);
                *pRsp_size = length;
                eStatus = 0x01;
            }
        }
    }
    if (eStatus == 0x01)
    {
        memcpy(&pRsp[*pRsp_size], T4T_NDEF_EMU_OK, sizeof(T4T_NDEF_EMU_OK));
        *pRsp_size += sizeof(T4T_NDEF_EMU_OK);
    } 
    else
    {
        memcpy(pRsp, T4T_NDEF_EMU_NOK, sizeof(T4T_NDEF_EMU_NOK));
        *pRsp_size = sizeof(T4T_NDEF_EMU_NOK);
        T4T_NDEF_EMU_Reset();
    }
}
/********************************** CallBack **********************************/

void onDataReceived(unsigned char *data, unsigned int data_length)
{
    framework_LockMutex(State::g_HCELock);
    
    State::HCE_dataLenght = data_length;
    State::HCE_data = (unsigned char*)malloc(State::HCE_dataLenght * sizeof(unsigned char));
    memcpy(State::HCE_data, data, data_length);
    
    if(State::eHCEState_NONE == State::g_HCEState)
    {
        State::g_HCEState = State::eHCEState_DATA_RECEIVED;
    }
    else if (State::eHCEState_WAIT_DATA == State::g_HCEState)
    {
        State::g_HCEState = State::eHCEState_DATA_RECEIVED;
        framework_NotifyMutex(State::g_HCELock, 0);
    }
    
    framework_UnlockMutex(State::g_HCELock);
}
void onHostCardEmulationActivated(unsigned char mode)
{
    framework_LockMutex(State::g_devLock);
    
    T4T_NDEF_EMU_Reset();
    
    if(State::eDevState_WAIT_ARRIVAL == State::g_DevState)
    {
        // printf("\tNFC Reader Found, mode=0x%.2x\n\n", mode); //TODO fixme
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_READER;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_WAIT_DEPARTURE == State::g_DevState)
    {
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_READER;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_EXIT == State::g_DevState)
    {
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else
    {
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_READER;
    }
    framework_UnlockMutex(State::g_devLock);
}
void onHostCardEmulationDeactivated()
{
    framework_LockMutex(State::g_devLock);
    
    if(State::eDevState_WAIT_DEPARTURE == State::g_DevState)
    {
        // printf("\tNFC Reader Lost\n\n"); //TODO fixme
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_PRESENT == State::g_DevState)
    {
        // printf("\tNFC Reader Lost\n\n");  //TODO fixme
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
    }
    else if(State::eDevState_WAIT_ARRIVAL == State::g_DevState)
    {
    }
    else if(State::eDevState_EXIT == State::g_DevState)
    {
    }
    else
    {
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
    }
    framework_UnlockMutex(State::g_devLock);
    
    framework_LockMutex(State::g_HCELock);
    if(State::eHCEState_WAIT_DATA == State::g_HCEState)
    {
        State::g_HCEState = State::eHCEState_NONE;
        framework_NotifyMutex(State::g_HCELock, 0x00);
    }
    else if(State::eHCEState_EXIT == State::g_HCEState)
    {
        
    }
    else
    {
        State::g_HCEState = State::eHCEState_NONE;
    }
    framework_UnlockMutex(State::g_HCELock);
}
 
void onTagArrival(nfc_tag_info_t *pTagInfo)
{
    framework_LockMutex(State::g_devLock);
    
    if(State::eDevState_WAIT_ARRIVAL == State::g_DevState)
    {    
        // printf("\tNFC Tag Found\n\n"); //TODO fixme
        memcpy(&State::g_TagInfo, pTagInfo, sizeof(nfc_tag_info_t));
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_TAG;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_WAIT_DEPARTURE == State::g_DevState)
    {    
        memcpy(&State::g_TagInfo, pTagInfo, sizeof(nfc_tag_info_t));
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_TAG;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_EXIT == State::g_DevState)
    {
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else
    {
        State::g_DevState = State::eDevState_PRESENT;
        State::g_Dev_Type = State::eDevType_TAG;
    }
    framework_UnlockMutex(State::g_devLock);
}
void onTagDeparture(void)
{    
    framework_LockMutex(State::g_devLock);
    
    
    if(State::eDevState_WAIT_DEPARTURE == State::g_DevState)
    {
        // printf("\tNFC Tag Lost\n\n"); //TODO fixme
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else if(State::eDevState_WAIT_ARRIVAL == State::g_DevState)
    {
    }    
    else if(State::eDevState_EXIT == State::g_DevState)
    {
    }
    else
    {
        State::g_DevState = State::eDevState_DEPARTED;
        State::g_Dev_Type = State::eDevType_NONE;
    }
    framework_UnlockMutex(State::g_devLock);
}
void onDeviceArrival (void)
{
    framework_LockMutex(State::g_devLock);
    
    switch(State::g_DevState)
    {
        case State::eDevState_WAIT_DEPARTURE:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_EXIT:
        {
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_NONE:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_WAIT_ARRIVAL:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_PRESENT:
        {
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_DEPARTED:
        {
            State::g_Dev_Type = State::eDevType_P2P;
            State::g_DevState = State::eDevState_PRESENT;
        } break;
    }
    
    framework_UnlockMutex(State::g_devLock);
}

void onDeviceDeparture (void)
{
    framework_LockMutex(State::g_devLock);
    
    switch(State::g_DevState)
    {
        case State::eDevState_WAIT_DEPARTURE:
        {
            State::g_DevState = State::eDevState_DEPARTED;
            State::g_Dev_Type = State::eDevType_NONE;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_EXIT:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_NONE:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_WAIT_ARRIVAL:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_PRESENT:
        {
            State::g_Dev_Type = State::eDevType_NONE;
            State::g_DevState = State::eDevState_DEPARTED;
        } break;
        case State::eDevState_DEPARTED:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
    }
    framework_UnlockMutex(State::g_devLock);
    
    framework_LockMutex(State::g_SnepClientLock);
    
    switch(State::g_SnepClientState)
    {
        case State::eSnepClientState_WAIT_OFF:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_OFF:
        {
        } break;
        case State::eSnepClientState_WAIT_READY:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_READY:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
        } break;
        case State::eSnepClientState_EXIT:
        {
        } break;
    }
    
    framework_UnlockMutex(State::g_SnepClientLock);
}

void onSnepClientReady()
{
    framework_LockMutex(State::g_devLock);
    
    switch(State::g_DevState)
    {
        case State::eDevState_WAIT_DEPARTURE:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_EXIT:
        {
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_NONE:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_WAIT_ARRIVAL:
        {
            State::g_DevState = State::eDevState_PRESENT;
            State::g_Dev_Type = State::eDevType_P2P;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_PRESENT:
        {
            State::g_Dev_Type = State::eDevType_P2P;
        } break;
        case State::eDevState_DEPARTED:
        {
            State::g_Dev_Type = State::eDevType_P2P;
            State::g_DevState = State::eDevState_PRESENT;
        } break;
    }
    framework_UnlockMutex(State::g_devLock);
    
    framework_LockMutex(State::g_SnepClientLock);
    
    switch(State::g_SnepClientState)
    {
        case State::eSnepClientState_WAIT_OFF:
        {
            State::g_SnepClientState = State::eSnepClientState_READY;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_OFF:
        {
            State::g_SnepClientState = State::eSnepClientState_READY;
        } break;
        case State::eSnepClientState_WAIT_READY:
        {
            State::g_SnepClientState = State::eSnepClientState_READY;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_READY:
        {
        } break;
        case State::eSnepClientState_EXIT:
        {
        } break;
    }
    
    framework_UnlockMutex(State::g_SnepClientLock);
}

void onSnepClientClosed()
{
    framework_LockMutex(State::g_devLock);
    
    switch(State::g_DevState)
    {
        case State::eDevState_WAIT_DEPARTURE:
        {
            State::g_DevState = State::eDevState_DEPARTED;
            State::g_Dev_Type = State::eDevType_NONE;
            framework_NotifyMutex(State::g_devLock, 0);
        } break;
        case State::eDevState_EXIT:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_NONE:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_WAIT_ARRIVAL:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
        case State::eDevState_PRESENT:
        {
            State::g_Dev_Type = State::eDevType_NONE;
            State::g_DevState = State::eDevState_DEPARTED;
        } break;
        case State::eDevState_DEPARTED:
        {
            State::g_Dev_Type = State::eDevType_NONE;
        } break;
    }
    framework_UnlockMutex(State::g_devLock);
    
    framework_LockMutex(State::g_SnepClientLock);
    
    switch(State::g_SnepClientState)
    {
        case State::eSnepClientState_WAIT_OFF:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_OFF:
        {
        } break;
        case State::eSnepClientState_WAIT_READY:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
            framework_NotifyMutex(State::g_SnepClientLock, 0);
        } break;
        case State::eSnepClientState_READY:
        {
            State::g_SnepClientState = State::eSnepClientState_OFF;
        } break;
        case State::eSnepClientState_EXIT:
        {
        } break;
    }
    
    framework_UnlockMutex(State::g_SnepClientLock);
}
