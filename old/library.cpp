#include "napi.h"
#include "linux_nfc_api.h"



/******************************************************************************
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License")
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tools.h"
#include <stdint.h>

#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "state.h"
#include "deviceinterface.h"


Napi::Env pollEnv = NULL;
Napi::Function pollCB;

Napi::Env errorEnv = NULL;
Napi::Function errorCB;

void onError(char* message) {
  if(errorEnv != NULL) {  
    errorCB.Call(errorEnv.Global(), { Napi::String::New(errorEnv, message) });
  }
}








void help(int mode);
int InitEnv();
int LookForTag(char** args, int args_len, char* tag, char** data, int format);




void PrintNDEFContent(nfc_tag_info_t* TagInfo, ndef_info_t* NDEFinfo, unsigned char* ndefRaw, unsigned int ndefRawLen)
{
    unsigned char* NDEFContent = NULL;
    nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;
    unsigned int res = 0x00;
    unsigned int i = 0x00;
    char* TextContent = NULL;
    char* URLContent = NULL;
    nfc_handover_select_t HandoverSelectContent;
    nfc_handover_request_t HandoverRequestContent;
    if(NULL != NDEFinfo)
    {
        ndefRawLen = NDEFinfo->current_ndef_length;
        NDEFContent = (unsigned char*)malloc(ndefRawLen * sizeof(unsigned char));
        res = nfcTag_readNdef(TagInfo->handle, NDEFContent, ndefRawLen, &lNDEFType);
    }
    else if (NULL != ndefRaw && 0x00 != ndefRawLen)
    {
        NDEFContent = (unsigned char*)malloc(ndefRawLen * sizeof(unsigned char));
        memcpy(NDEFContent, ndefRaw, ndefRawLen);
        res = ndefRawLen;
        if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x55 == NDEFContent[3])
        {
            lNDEFType = NDEF_FRIENDLY_TYPE_URL;
        }
        if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x54 == NDEFContent[3])
        {
            lNDEFType = NDEF_FRIENDLY_TYPE_TEXT;
        }
    }
    else
    {
        printf("\t\t\t\tError : Invalid Parameters\n");
    }
    
    if(res != ndefRawLen)
    {
        printf("\t\t\t\tRead NDEF Content Failed\n");
    }
    else
    {
        switch(lNDEFType)
        {
            case NDEF_FRIENDLY_TYPE_TEXT:
            {
                TextContent = (char*)malloc(res * sizeof(char));
                res = ndef_readText(NDEFContent, res, TextContent, res);
                if(0x00 <= res)
                {
                    printf("\t\t\t\tType :                 'Text'\n");
                    printf("\t\t\t\tText :                 '%s'\n\n", TextContent);
                }
                else
                {
                    printf("\t\t\t\tRead NDEF Text Error\n");
                }
                if(NULL != TextContent)
                {
                    free(TextContent);
                    TextContent = NULL;
                }
            } break;
            case NDEF_FRIENDLY_TYPE_URL:
            {
                /*NOTE : + 27 = Max prefix lenght*/
                URLContent = (char*)malloc(res * sizeof(unsigned char) + 27 );
                memset(URLContent, 0x00, res * sizeof(unsigned char) + 27);
                res = ndef_readUrl(NDEFContent, res, URLContent, res + 27);
                if(0x00 <= res)
                {
                    printf("                Type :                 'URI'\n");
                    printf("                URI :                 '%s'\n\n", URLContent);
                    /*NOTE: open url in browser*/
                    /*open_uri(URLContent);*/
                }
                else
                {
                    printf("                Read NDEF URL Error\n");
                }
                if(NULL != URLContent)
                {
                    free(URLContent);
                    URLContent = NULL;
                }
            } break;
            case NDEF_FRIENDLY_TYPE_HS:
            {
                res = ndef_readHandoverSelectInfo(NDEFContent, res, &HandoverSelectContent);
                if(0x00 <= res)
                {
                    printf("\n\t\tHandover Select : \n");
                    
                    printf("\t\tBluetooth : \n\t\t\t\tPower state : ");
                    switch(HandoverSelectContent.bluetooth.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    if(HANDOVER_TYPE_BT == HandoverSelectContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BT'\n");
                    }
                    else if(HANDOVER_TYPE_BLE == HandoverSelectContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BLE'\n");
                    }
                    else
                    {
                        printf("\t\t\t\tType :            'Unknown'\n");
                    }
                    printf("\t\t\t\tAddress :      '");
                    for(i = 0x00; i < 6; i++)
                    {
                        printf("%02X ", HandoverSelectContent.bluetooth.address[i]);
                    }
                    printf("'\n\t\t\t\tDevice Name :  '");
                    for(i = 0x00; i < HandoverSelectContent.bluetooth.device_name_length; i++)    
                    {
                        printf("%c ", HandoverSelectContent.bluetooth.device_name[i]);
                    }
                    printf("'\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverSelectContent.bluetooth.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.bluetooth.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n\t\tWIFI : \n\t\t\t\tPower state : ");
                    switch(HandoverSelectContent.wifi.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    
                    printf("\t\t\t\tSSID :         '");
                    for(i = 0x01; i < HandoverSelectContent.wifi.ssid_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.ssid[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }
                    printf("'\n\t\t\t\tKey :          '");
                    for(i = 0x01; i < HandoverSelectContent.wifi.key_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.key[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }                
                    printf("'\n\t\t\t\tNDEF Record : \n");
                    for(i = 0x01; i < HandoverSelectContent.wifi.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.ndef[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }
                    printf("\n");
                }
                else
                {
                    printf("\n\t\tRead NDEF Handover Select Failed\n");
                }
                
            } break;
            case NDEF_FRIENDLY_TYPE_HR:
            {
                res = ndef_readHandoverRequestInfo(NDEFContent, res, &HandoverRequestContent);
                if(0x00 <= res)
                {
                    printf("\n\t\tHandover Request : \n");
                    printf("\t\tBluetooth : \n\t\t\t\tPower state : ");
                    switch(HandoverRequestContent.bluetooth.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    if(HANDOVER_TYPE_BT == HandoverRequestContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BT'\n");
                    }
                    else if(HANDOVER_TYPE_BLE == HandoverRequestContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BLE'\n");
                    }
                    else
                    {
                        printf("\t\t\t\tType :            'Unknown'\n");
                    }
                    printf("\t\t\t\tAddress :      '");
                    for(i = 0x00; i < 6; i++)
                    {
                        printf("%02X ", HandoverRequestContent.bluetooth.address[i]);
                    }
                    printf("'\n\t\t\t\tDevice Name :  '");
                    for(i = 0x00; i < HandoverRequestContent.bluetooth.device_name_length; i++)    
                    {
                        printf("%c ", HandoverRequestContent.bluetooth.device_name[i]);
                    }
                    printf("'\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverRequestContent.bluetooth.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverRequestContent.bluetooth.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n\t\t\t\tWIFI :         'Has WIFI Request : %X '", HandoverRequestContent.wifi.has_wifi);
                    printf("\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverRequestContent.wifi.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverRequestContent.wifi.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n");
                }
                else
                {
                    printf("\n\t\tRead NDEF Handover Request Failed\n");
                }
            } break;
            case NDEF_FRIENDLY_TYPE_OTHER:
            {
                switch(NDEFContent[0] & 0x7)
                {
                    case NDEF_TNF_EMPTY:
                    {
                        printf("\n\t\tTNF Empty\n");
                    } break;
                    case NDEF_TNF_WELLKNOWN:
                    {
                        printf("\n\t\tTNF Well Known\n");
                    } break;
                    case NDEF_TNF_MEDIA:
                    {
                        printf("\n\t\tTNF Media\n\n");
                        printf("\t\t\tType : ");
                        for(i = 0x00; i < NDEFContent[1]; i++)
                        {
                            printf("%c", NDEFContent[3 + i]);
                        }
                        printf("\n\t\t\tData : ");
                        for(i = 0x00; i < NDEFContent[2]; i++)
                         {
                            printf("%c", NDEFContent[3 + NDEFContent[1] + i]);
                            if('\n' == NDEFContent[3 + NDEFContent[1] + i])
                            {
                                printf("\t\t\t");
                            }
                        }
                        printf("\n");
                        
                    } break;
                    case NDEF_TNF_URI:
                    {
                        printf("\n\t\tTNF URI\n");
                    } break;
                    case NDEF_TNF_EXT:
                    {
                        printf("\n\t\tTNF External\n\n");
                        printf("\t\t\tType : ");
                        for(i = 0x00; i < NDEFContent[1]; i++)
                        {
                            printf("%c", NDEFContent[3 + i]);
                        }
                        printf("\n\t\t\tData : ");
                        for(i = 0x00; i < NDEFContent[2]; i++)
                         {
                            printf("%c", NDEFContent[3 + NDEFContent[1] + i]);
                            if('\n' == NDEFContent[3 + NDEFContent[1] + i])
                            {
                                printf("\t\t\t");
                            }
                        }
                        printf("\n");
                    } break;
                    case NDEF_TNF_UNKNOWN:
                    {
                        printf("\n\t\tTNF Unknown\n");
                    } break;
                    case NDEF_TNF_UNCHANGED:
                    {
                        printf("\n\t\tTNF Unchanged\n");
                    } break;
                    default:
                    {
                        printf("\n\t\tTNF Other\n");
                    } break;
                }
            } break;
            default:
            {
            } break;
        }
        printf("\n\t\t%d bytes of NDEF data received :\n\t\t", ndefRawLen);
        for(i = 0x00; i < ndefRawLen; i++)
        {
            printf("%02X ", NDEFContent[i]);
            if(i%30 == 0 && 0x00 != i)
            {
                printf("\n\t\t");
            }
        }
        printf("\n\n");
    }
    
    if(NULL != NDEFContent)
    {
        free(NDEFContent);
        NDEFContent = NULL;
    }
}


void onMessageReceived(unsigned char *message, unsigned int length)
{
    unsigned int i = 0x00;
    // printf("\n\t\tNDEF Message Received : \n"); //TODO fixme
    PrintNDEFContent(NULL, NULL, message, length);
}
 
int InitMode(int tag, int p2p, int hce)
{
    int res = 0x00;
    
    State::g_TagCB.onTagArrival = onTagArrival;
    State::g_TagCB.onTagDeparture = onTagDeparture;
        
    State::g_SnepServerCB.onDeviceArrival = onDeviceArrival;
    State::g_SnepServerCB.onDeviceDeparture = onDeviceDeparture;
    State::g_SnepServerCB.onMessageReceived = onMessageReceived;
    
    State::g_SnepClientCB.onDeviceArrival = onSnepClientReady;
    State::g_SnepClientCB.onDeviceDeparture = onSnepClientClosed;
        
    State::g_HceCB.onDataReceived = onDataReceived;
    State::g_HceCB.onHostCardEmulationActivated = onHostCardEmulationActivated;
    State::g_HceCB.onHostCardEmulationDeactivated = onHostCardEmulationDeactivated;
    
    if(0x00 == res)
    {
        res = nfcManager_doInitialize();
        if(0x00 != res)
        {
            printf("NfcService Init Failed\n");
        }
    }
    
    if(0x00 == res)
    {
        if(0x01 == tag)
        {
            nfcManager_registerTagCallback(&State::g_TagCB);
        }
        
        if(0x01 == p2p)
        {
            res = nfcSnep_registerClientCallback(&State::g_SnepClientCB);
            if(0x00 != res)
            {
                printf("SNEP Client Register Callback Failed\n");
            }
        }
    }
    
    if(0x00 == res && 0x01 == hce)
    {
        nfcHce_registerHceCallback(&State::g_HceCB);
    }
    if(0x00 == res)
    {
        nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 0x00, hce, 0);
        if(0x01 == p2p)
        {
            res = nfcSnep_startServer(&State::g_SnepServerCB);
            if(0x00 != res)
            {
                printf("Start SNEP Server Failed\n");
            }
        }
    }
    
    return res;
}

int DeinitPollMode()
{
    int res = 0x00;
    
    nfcSnep_stopServer();
    
    nfcManager_disableDiscovery();
    
    nfcSnep_deregisterClientCallback();
    
    nfcManager_deregisterTagCallback();
    
    nfcHce_deregisterHceCallback();
    
    res = nfcManager_doDeinitialize();
    
    if(0x00 != res)
    {
        printf("NFC Service Deinit Failed\n");
    }
    return res;
}

int SnepPush(unsigned char* msgToPush, unsigned int len)
{
    int res = 0x00;
    
    framework_LockMutex(State::g_devLock);
    framework_LockMutex(State::g_SnepClientLock);
    
    if(State::eSnepClientState_READY != State::g_SnepClientState && State::eSnepClientState_EXIT!= State::g_SnepClientState && State::eDevState_PRESENT == State::g_DevState)
    {
        framework_UnlockMutex(State::g_devLock);
        State::g_SnepClientState = State::eSnepClientState_WAIT_READY;
        framework_WaitMutex(State::g_SnepClientLock, 0);
    }
    else
    {
        framework_UnlockMutex(State::g_devLock);
    }
    
    if(State::eSnepClientState_READY == State::g_SnepClientState)
    {
        framework_UnlockMutex(State::g_SnepClientLock);
        res = nfcSnep_putMessage(msgToPush, len);
        
        if(0x00 != res)
        {
            printf("\t\tPush Failed\n");
        }
        else
        {
            printf("\t\tPush successful\n");
        }
    }
    else
    {
        framework_UnlockMutex(State::g_SnepClientLock);
    }
    
    return res;
}

int WriteTag(nfc_tag_info_t TagInfo, unsigned char* msgToPush, unsigned int len)
{
    int res = 0x00;
    
    res = nfcTag_writeNdef(TagInfo.handle, msgToPush, len);
    if(0x00 != res)
    {
        res = 0xFF;
    }
    else
    {
        res = 0x00;
    }
    return res;
}

Napi::Object createNDEFObj(Napi::Env env, ndef_info_t pNDEFinfo) {
  Napi::Object info = Napi::Object::New(env);
  
  if(pNDEFinfo.is_ndef) {
    info.Set("size", pNDEFinfo.max_ndef_length);
    info.Set("length", pNDEFinfo.current_ndef_length);
    info.Set("writable", (bool)pNDEFinfo.is_writable);
  } else {
    info.Set("size", 0);
    info.Set("length", 0);
    info.Set("writable", false);
  }
  
  return info;
}

void addNDEFContent(nfc_tag_info_t* TagInfo, ndef_info_t* NDEFinfo, unsigned char* ndefRaw, unsigned int ndefRawLen, Napi::Object info) {
  unsigned char* NDEFContent = NULL;
    nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;
    unsigned int res = 0x00;
    unsigned int i = 0x00;
    char* TextContent = NULL;
    char* URLContent = NULL;
    nfc_handover_select_t HandoverSelectContent;
    nfc_handover_request_t HandoverRequestContent;
    if(NULL != NDEFinfo)
    {
        ndefRawLen = NDEFinfo->current_ndef_length;
        NDEFContent = (unsigned char*)malloc(ndefRawLen * sizeof(unsigned char));
        res = nfcTag_readNdef(TagInfo->handle, NDEFContent, ndefRawLen, &lNDEFType);
    }
    else if (NULL != ndefRaw && 0x00 != ndefRawLen)
    {
        NDEFContent = (unsigned char*)malloc(ndefRawLen * sizeof(unsigned char));
        memcpy(NDEFContent, ndefRaw, ndefRawLen);
        res = ndefRawLen;
        if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x55 == NDEFContent[3])
        {
            lNDEFType = NDEF_FRIENDLY_TYPE_URL;
        }
        if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x54 == NDEFContent[3])
        {
            lNDEFType = NDEF_FRIENDLY_TYPE_TEXT;
        }
    }
    else
    {
        printf("\t\t\t\tError : Invalid Parameters\n");
    }
    
    if(res != ndefRawLen)
    {
        printf("\t\t\t\tRead NDEF Content Failed\n");
    }
    else
    {
        switch(lNDEFType)
        {
            case NDEF_FRIENDLY_TYPE_TEXT:
            {
                TextContent = (char*)malloc(res * sizeof(char));
                res = ndef_readText(NDEFContent, res, TextContent, res);
                if(0x00 <= res)
                {
                    printf("\t\t\t\tType :                 'Text'\n");
                    printf("\t\t\t\tText :                 '%s'\n\n", TextContent);
                    
                    info.Set("type", "Text");
                    info.Set("content", TextContent);
                }
                else
                {
                    printf("\t\t\t\tRead NDEF Text Error\n");
                }
                if(NULL != TextContent)
                {
                    free(TextContent);
                    TextContent = NULL;
                }
            } break;
            case NDEF_FRIENDLY_TYPE_URL:
            {
                /*NOTE : + 27 = Max prefix lenght*/
                URLContent = (char*)malloc(res * sizeof(unsigned char) + 27 );
                memset(URLContent, 0x00, res * sizeof(unsigned char) + 27);
                res = ndef_readUrl(NDEFContent, res, URLContent, res + 27);
                if(0x00 <= res)
                {
                    printf("                Type :                 'URI'\n");
                    printf("                URI :                 '%s'\n\n", URLContent);
                    /*NOTE: open url in browser*/
                    /*open_uri(URLContent);*/
                    
                    info.Set("type", "URI");
                    info.Set("content", URLContent);
                }
                else
                {
                    printf("                Read NDEF URL Error\n");
                }
                if(NULL != URLContent)
                {
                    free(URLContent);
                    URLContent = NULL;
                }
            } break;
            case NDEF_FRIENDLY_TYPE_HS:
            {
                res = ndef_readHandoverSelectInfo(NDEFContent, res, &HandoverSelectContent);
                if(0x00 <= res)
                {
                    printf("\n\t\tHandover Select : \n");
                    
                    printf("\t\tBluetooth : \n\t\t\t\tPower state : ");
                    switch(HandoverSelectContent.bluetooth.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    if(HANDOVER_TYPE_BT == HandoverSelectContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BT'\n");
                    }
                    else if(HANDOVER_TYPE_BLE == HandoverSelectContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BLE'\n");
                    }
                    else
                    {
                        printf("\t\t\t\tType :            'Unknown'\n");
                    }
                    printf("\t\t\t\tAddress :      '");
                    for(i = 0x00; i < 6; i++)
                    {
                        printf("%02X ", HandoverSelectContent.bluetooth.address[i]);
                    }
                    printf("'\n\t\t\t\tDevice Name :  '");
                    for(i = 0x00; i < HandoverSelectContent.bluetooth.device_name_length; i++)    
                    {
                        printf("%c ", HandoverSelectContent.bluetooth.device_name[i]);
                    }
                    printf("'\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverSelectContent.bluetooth.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.bluetooth.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n\t\tWIFI : \n\t\t\t\tPower state : ");
                    switch(HandoverSelectContent.wifi.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    
                    printf("\t\t\t\tSSID :         '");
                    for(i = 0x01; i < HandoverSelectContent.wifi.ssid_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.ssid[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }
                    printf("'\n\t\t\t\tKey :          '");
                    for(i = 0x01; i < HandoverSelectContent.wifi.key_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.key[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }                
                    printf("'\n\t\t\t\tNDEF Record : \n");
                    for(i = 0x01; i < HandoverSelectContent.wifi.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverSelectContent.wifi.ndef[i]);
                        if(i%30 == 0)
                        {
                            printf("\n");
                        }
                    }
                    printf("\n");
                }
                else
                {
                    printf("\n\t\tRead NDEF Handover Select Failed\n");
                }
                
            } break;
            case NDEF_FRIENDLY_TYPE_HR:
            {
                res = ndef_readHandoverRequestInfo(NDEFContent, res, &HandoverRequestContent);
                if(0x00 <= res)
                {
                    printf("\n\t\tHandover Request : \n");
                    printf("\t\tBluetooth : \n\t\t\t\tPower state : ");
                    switch(HandoverRequestContent.bluetooth.power_state)
                    {
                        case HANDOVER_CPS_INACTIVE:
                        {
                            printf(" 'Inactive'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVE:
                        {
                            printf(" 'Active'\n");
                        } break;
                        case HANDOVER_CPS_ACTIVATING:
                        {
                            printf(" 'Activating'\n");
                        } break;
                        case HANDOVER_CPS_UNKNOWN:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                        default:
                        {
                            printf(" 'Unknown'\n");
                        } break;
                    }
                    if(HANDOVER_TYPE_BT == HandoverRequestContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BT'\n");
                    }
                    else if(HANDOVER_TYPE_BLE == HandoverRequestContent.bluetooth.type)
                    {
                        printf("\t\t\t\tType :         'BLE'\n");
                    }
                    else
                    {
                        printf("\t\t\t\tType :            'Unknown'\n");
                    }
                    printf("\t\t\t\tAddress :      '");
                    for(i = 0x00; i < 6; i++)
                    {
                        printf("%02X ", HandoverRequestContent.bluetooth.address[i]);
                    }
                    printf("'\n\t\t\t\tDevice Name :  '");
                    for(i = 0x00; i < HandoverRequestContent.bluetooth.device_name_length; i++)    
                    {
                        printf("%c ", HandoverRequestContent.bluetooth.device_name[i]);
                    }
                    printf("'\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverRequestContent.bluetooth.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverRequestContent.bluetooth.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n\t\t\t\tWIFI :         'Has WIFI Request : %X '", HandoverRequestContent.wifi.has_wifi);
                    printf("\n\t\t\t\tNDEF Record :     \n\t\t\t\t");
                    for(i = 0x01; i < HandoverRequestContent.wifi.ndef_length+1; i++)
                    {
                        printf("%02X ", HandoverRequestContent.wifi.ndef[i]);
                        if(i%8 == 0)
                        {
                            printf("\n\t\t\t\t");
                        }
                    }
                    printf("\n");
                }
                else
                {
                    printf("\n\t\tRead NDEF Handover Request Failed\n");
                }
            } break;
            case NDEF_FRIENDLY_TYPE_OTHER:
            {
                switch(NDEFContent[0] & 0x7)
                {
                    case NDEF_TNF_EMPTY:
                    {
                        printf("\n\t\tTNF Empty\n");
                    } break;
                    case NDEF_TNF_WELLKNOWN:
                    {
                        printf("\n\t\tTNF Well Known\n");
                    } break;
                    case NDEF_TNF_MEDIA:
                    {
                        printf("\n\t\tTNF Media\n\n");
                        printf("\t\t\tType : ");
                        for(i = 0x00; i < NDEFContent[1]; i++)
                        {
                            printf("%c", NDEFContent[3 + i]);
                        }
                        printf("\n\t\t\tData : ");
                        for(i = 0x00; i < NDEFContent[2]; i++)
                         {
                            printf("%c", NDEFContent[3 + NDEFContent[1] + i]);
                            if('\n' == NDEFContent[3 + NDEFContent[1] + i])
                            {
                                printf("\t\t\t");
                            }
                        }
                        printf("\n");
                        
                    } break;
                    case NDEF_TNF_URI:
                    {
                        printf("\n\t\tTNF URI\n");
                    } break;
                    case NDEF_TNF_EXT:
                    {
                        printf("\n\t\tTNF External\n\n");
                        printf("\t\t\tType : ");
                        for(i = 0x00; i < NDEFContent[1]; i++)
                        {
                            printf("%c", NDEFContent[3 + i]);
                        }
                        printf("\n\t\t\tData : ");
                        for(i = 0x00; i < NDEFContent[2]; i++)
                         {
                            printf("%c", NDEFContent[3 + NDEFContent[1] + i]);
                            if('\n' == NDEFContent[3 + NDEFContent[1] + i])
                            {
                                printf("\t\t\t");
                            }
                        }
                        printf("\n");
                    } break;
                    case NDEF_TNF_UNKNOWN:
                    {
                        printf("\n\t\tTNF Unknown\n");
                    } break;
                    case NDEF_TNF_UNCHANGED:
                    {
                        printf("\n\t\tTNF Unchanged\n");
                    } break;
                    default:
                    {
                        printf("\n\t\tTNF Other\n");
                    } break;
                }
            } break;
            default:
            {
            } break;
        }
        printf("\n\t\t%d bytes of NDEF data received :\n\t\t", ndefRawLen);
        for(i = 0x00; i < ndefRawLen; i++)
        {
            printf("%02X ", NDEFContent[i]);
            if(i%30 == 0 && 0x00 != i)
            {
                printf("\n\t\t");
            }
        }
        printf("\n\n");
    }
  
  if(NULL != NDEFContent)
    {
        free(NDEFContent);
        NDEFContent = NULL;
    }
}

void PrintfNDEFInfo(ndef_info_t pNDEFinfo)
{
    if(0x01 == pNDEFinfo.is_ndef)
    {
        printf("\t\tRecord Found :\n");
        printf("\t\t\t\tNDEF Content Max size :     '%d bytes'\n", pNDEFinfo.max_ndef_length);
        printf("\t\t\t\tNDEF Actual Content size :     '%d bytes'\n", pNDEFinfo.current_ndef_length);
        if(0x01 == pNDEFinfo.is_writable)
        {
            printf("\t\t\t\tReadOnly :                      'FALSE'\n");
        }
        else
        {
            printf("\t\t\t\tReadOnly :                         'TRUE'\n");
        }
    }
    else    
    {
        printf("\t\tNo Record found\n");
    }
}



/* mode=1 => poll, mode=2 => push, mode=3 => write, mode=4 => HCE */
int WaitDeviceArrival(int mode, unsigned char* msgToSend, unsigned int len)
{
    int res = 0x00;
    unsigned int i = 0x00;
    int block = 0x01;
    unsigned char key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ndef_info_t NDEFinfo;
    State::eDevType DevTypeBck = State::eDevType_NONE;
    unsigned char MifareAuthCmd[] = {0x60U, 0x00 /*block*/, 0x02, 0x02, 0x02, 0x02, 0x00 /*key*/, 0x00 /*key*/, 0x00 /*key*/, 0x00 /*key*/ , 0x00 /*key*/, 0x00 /*key*/};
    unsigned char MifareAuthResp[255];
    unsigned char MifareReadCmd[] = {0x30U,  /*block*/ 0x00};
    unsigned char MifareWriteCmd[] = {0xA2U,  /*block*/ 0x04, 0xFF, 0xFF, 0xFF, 0xFF};
    unsigned char MifareResp[255];
    
    unsigned char HCEReponse[255];
    short unsigned int HCEResponseLen = 0x00;
    int tag_count=0;
    int num_tags = 0;
    
    nfc_tag_info_t TagInfo;
    Napi::Object tagInfo;
    Napi::Object uid;
    Napi::Object technology;
    
    MifareAuthCmd[1] = block;
    memcpy(&MifareAuthCmd[6], key, 6);
    MifareReadCmd[1] = block;
    
    do
    {
        framework_LockMutex(State::g_devLock);
        if(State::eDevState_EXIT == State::g_DevState)
        {
            framework_UnlockMutex(State::g_devLock);
            break;
        }
        
        else if(State::eDevState_PRESENT != State::g_DevState)
        {
               if(tag_count == 0) printf("Waiting for a Tag/Device...\n\n");
            State::g_DevState = State::eDevState_WAIT_ARRIVAL;
            framework_WaitMutex(State::g_devLock, 0);
        }
        
        if(State::eDevState_EXIT == State::g_DevState)
        {
            framework_UnlockMutex(State::g_devLock);
            break;
        }
        
        if(State::eDevState_PRESENT == State::g_DevState)
        {
            DevTypeBck = State::g_Dev_Type;
            if(State::eDevType_TAG == State::g_Dev_Type)
            {
                tagInfo = Napi::Object::New(pollEnv);
                uid = Napi::Object::New(pollEnv);
                technology = Napi::Object::New(pollEnv);
                
                memcpy(&TagInfo, &State::g_TagInfo, sizeof(nfc_tag_info_t));
                framework_UnlockMutex(State::g_devLock);
                printf("        Type : ");
                
                technology.Set("code", TagInfo.technology);
                
                switch (TagInfo.technology)
                {
                    case TARGET_TYPE_ISO14443_3A:
                    {
                      technology.Set("name", "Type A");
                      technology.Set("type", "ISO14443_3A");
                        printf("        'Type A'\n");
                    } break;
                    case TARGET_TYPE_ISO14443_3B:
                    {
                      technology.Set("name", "Type 4B");
                      technology.Set("type", "ISO14443_3B");
                        printf("        'Type 4B'\n");
                    } break;
                    case TARGET_TYPE_ISO14443_4:
                    {
                      technology.Set("name", "Type 4A");
                      technology.Set("type", "ISO14443_4");
                        printf("        'Type 4A'\n");
                    } break;
                    case TARGET_TYPE_FELICA:
                    {
                      technology.Set("name", "Type F");
                      technology.Set("type", "FELICA");
                        printf("        'Type F'\n");
                    } break;
                    case TARGET_TYPE_ISO15693:
                    {
                      technology.Set("name", "Type V");
                      technology.Set("type", "ISO15693");
                        printf("        'Type V'\n");
                    } break;
                    case TARGET_TYPE_NDEF:
                    {
                      technology.Set("name", "NDEF");
                      technology.Set("type", "NDEF");
                        printf("        'Type NDEF'\n");
                    } break;
                    case TARGET_TYPE_NDEF_FORMATABLE:
                    {
                      technology.Set("name", "Formatable");
                      technology.Set("type", "NDEF_FORMATABLE");
                        printf("        'Type Formatable'\n");
                    } break;
                    case TARGET_TYPE_MIFARE_CLASSIC:
                    {
                      technology.Set("name", "Type A - Mifare Classic");
                      technology.Set("type", "MIFARE_CLASSIC");
                        printf("        'Type A - Mifare Classic'\n");
                    } break;
                    case TARGET_TYPE_MIFARE_UL:
                    {
                      technology.Set("name", "Type A - Mifare Ul");
                      technology.Set("type", "MIFARE_UL");
                        printf("        'Type A - Mifare Ul'\n");
                    } break;
                    case TARGET_TYPE_KOVIO_BARCODE:
                    {
                      technology.Set("name", "Type A - Kovio Barcode");
                      technology.Set("type", "KOVIO_BARCODE");
                        printf("        'Type A - Kovio Barcode'\n");
                    } break;
                    case TARGET_TYPE_ISO14443_3A_3B:
                    {
                      technology.Set("name", "Type A/B");
                      technology.Set("type", "ISO14443_3A_3B");
                        printf("        'Type A/B'\n");
                    } break;
                    default:
                    {
                      technology.Set("name", "Unknown or not supported");
                      technology.Set("type", "UNKNOWN");
                        printf("        'Type %d (Unknown or not supported)'\n", TagInfo.technology);
                    } break;
                }
                
                tagInfo.Set("technology", technology);
                
                /*32 is max UID len (Kovio tags)*/
                if((0x00 != TagInfo.uid_length) && (32 >= TagInfo.uid_length))
                {
                    uid.Set("length", TagInfo.uid_length);
                    if(4 == TagInfo.uid_length || 7 == TagInfo.uid_length || 10 == TagInfo.uid_length)
                    {
                        printf("        NFCID1 :    \t'");
                        uid.Set("type", "NFCID1");
                    }
                    else if(8 == TagInfo.uid_length)
                    {
                        printf("        NFCID2 :    \t'");
                        uid.Set("type", "NFCID2");
                    }
                    else
                    {
                        printf("        UID :       \t'");
                        uid.Set("type", "UID");
                    }
                    
                    std::string s = "";
                    std::ostringstream oss;
                    oss << std::setfill('0');
                    
                    for(i = 0x00; i < TagInfo.uid_length; i++)
                    {
                        printf("%02X ", (unsigned char) TagInfo.uid[i]);
                        oss << std::setw(2) << std::hex << static_cast< int >( TagInfo.uid[i] );
                        
                        if (i < TagInfo.uid_length - 1) {
                            oss << ":";
                        }
                    }
                    printf("'\n");
                    s.assign( oss.str() );
                    
                    uid.Set("id", s);
                    
                    tagInfo.Set("uid", uid);
                }
                res = nfcTag_isNdef(TagInfo.handle, &NDEFinfo);
                if(0x01 == res)
                {
                    PrintfNDEFInfo(NDEFinfo);
                    Napi::Object info = createNDEFObj(pollEnv, NDEFinfo);
                    
                    
                    addNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00, info);
                    
                    tagInfo.Set("ndef", info);
                    
                    PrintNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00);
                }
                else
                {
                    printf("\t\tNDEF Content : NO, mode=%d, tech=%d\n", mode, TagInfo.technology);
                    
                    if(0x03 == mode)
                    {
                         printf("\n\tFormating tag to NDEF prior to write ...\n");
                        if(nfcTag_isFormatable(TagInfo.handle))
                        {
                            if(nfcTag_formatTag(TagInfo.handle) == 0x00)
                            {
                                  printf("\tTag formating succeed\n");
                            }
                            else
                            {
                                  printf("\tTag formating failed\n");
                            }
                        }
                        else
                        {
                              printf("\tTag is not formatable\n");
                        }
                    }
                    else if(TARGET_TYPE_MIFARE_CLASSIC == TagInfo.technology)
                    {
                        memset(MifareAuthResp, 0x00, 255);
                        memset(MifareResp, 0x00, 255);
                        res = nfcTag_transceive(TagInfo.handle, MifareAuthCmd, 12, MifareAuthResp, 255, 500);
                        if(0x00 == res)
                        {
                            printf("\n\t\tRAW Tag transceive failed\n");
                        }
                        else
                        {
                            printf("\n\t\tMifare Authenticate command sent\n\t\tResponse : \n\t\t");
                            for(i = 0x00; i < (unsigned int) res; i++)
                            {
                                printf("%02X ", MifareAuthResp[i]);
                            }
                            printf("\n");
                            
                            res = nfcTag_transceive(TagInfo.handle, MifareReadCmd, 2, MifareResp, 255, 500);
                            if(0x00 == res)
                            {
                                printf("\n\t\tRAW Tag transceive failed\n");
                            }
                            else
                            {
                                printf("\n\t\tMifare Read command sent\n\t\tResponse : \n\t\t");
                                for(i = 0x00; i < (unsigned int)res; i++)
                                {
                                    printf("%02X ", MifareResp[i]);
                                }
                                printf("\n\n");
                            }
                        }
                    }
                    else if(TARGET_TYPE_MIFARE_UL == TagInfo.technology)
                    {
                        printf("\n\tMIFARE UL card\n");
                        printf("\t\tMifare Read command: ");
                        for(i = 0x00; i < (unsigned int) sizeof(MifareReadCmd) ; i++)
                        {
                            printf("%02X ", MifareReadCmd[i]);
                        }
                        printf("\n");
                        res = nfcTag_transceive(TagInfo.handle, MifareReadCmd, sizeof(MifareReadCmd), MifareResp, 16, 500);
                        if(0x00 == res)
                        {
                            printf("\n\t\tRAW Tag transceive failed\n");
                        }
                        else
                        {
                            printf("\n\t\tMifare Read command sent\n\t\tResponse : \n\t\t");
                            for(i = 0x00; i < (unsigned int)res; i++)
                            {
                                printf("%02X ", MifareResp[i]);
                            }
                            printf("\n\n");
                        }
                    }
                    else
                    {
                        printf("\n\tNot a MIFARE card\n");
                    }
                }
                if(0x03 == mode)
                {
                    res = WriteTag(TagInfo, msgToSend, len);
                    if(0x00 == res)
                    {
                        printf("\tWrite Tag OK\n\tRead back data\n");
                        res = nfcTag_isNdef(TagInfo.handle, &NDEFinfo);
                        if(0x01 == res)
                        {
                            PrintfNDEFInfo(NDEFinfo);
                            PrintNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00);
                        }
                    }
                    else
                    {
                        printf("\tWrite Tag Failed\n");
                    }
                }
                num_tags = nfcManager_getNumTags();
                if(num_tags > 1)
                {
                    tag_count++;
                    if (tag_count < num_tags)
                    {
                        printf("\tMultiple tags found, selecting next tag...\n");
                        nfcManager_selectNextTag();
                    }
                    else
                    {
                        tag_count = 0;
                    }
                }
                 framework_LockMutex(State::g_devLock);
                 
                 pollCB.Call(pollEnv.Global(), { tagInfo });
            }
            else if(State::eDevType_P2P == State::g_Dev_Type)/*P2P Detected*/
            {
                framework_UnlockMutex(State::g_devLock);
                 printf("\tDevice Found\n");
                
                if(2 == mode)
                {
                    SnepPush(msgToSend, len);
                }
                
                framework_LockMutex(State::g_SnepClientLock);
    
                if(State::eSnepClientState_READY == State::g_SnepClientState)
                {
                    State::g_SnepClientState = State::eSnepClientState_WAIT_OFF;
                    framework_WaitMutex(State::g_SnepClientLock, 0);
                }
                
                framework_UnlockMutex(State::g_SnepClientLock);
                framework_LockMutex(State::g_devLock);
        
            }
            else if(State::eDevType_READER == State::g_Dev_Type)
            {                
                framework_LockMutex(State::g_HCELock);
                do
                {
                    framework_UnlockMutex(State::g_devLock);
                
                    if(State::eHCEState_NONE == State::g_HCEState)
                    {
                        State::g_HCEState = State::eHCEState_WAIT_DATA;
                        framework_WaitMutex(State::g_HCELock, 0x00);
                    }
                    
                    if(State::eHCEState_DATA_RECEIVED == State::g_HCEState)
                    {
                        State::g_HCEState = State::eHCEState_NONE;
                        
                        unsigned char* HCE_data = getHCEdata();
                        int HCE_dataLenght = getHCEdatalength();
                        
                        if(HCE_data != NULL)
                        {
                            printf("\t\tReceived data from remote device : \n\t\t");
                            
                            for(i = 0x00; i < HCE_dataLenght; i++)
                            {
                                printf("%02X ", HCE_data[i]);
                            }
                            
                            /*Call HCE response builder*/
                            T4T_NDEF_EMU_Next(HCE_data, HCE_dataLenght, HCEReponse, &HCEResponseLen);
                            free(HCE_data);
                            HCE_dataLenght = 0x00;
                            HCE_data = NULL;
                        }
                        framework_UnlockMutex(State::g_HCELock);
                        res = nfcHce_sendCommand(HCEReponse, HCEResponseLen);
                        framework_LockMutex(State::g_HCELock);
                        if(0x00 == res)
                        {
                            printf("\n\n\t\tResponse sent : \n\t\t");
                            for(i = 0x00; i < HCEResponseLen; i++)
                            {
                                printf("%02X ", HCEReponse[i]);
                            }
                            printf("\n\n");
                        }
                        else
                        {
                            printf("\n\n\t\tFailed to send response\n\n");
                        }
                    }
                    framework_LockMutex(State::g_devLock);
                }while(State::eDevState_PRESENT == State::g_DevState);
                framework_UnlockMutex(State::g_HCELock);
            }
            else
            {
                framework_UnlockMutex(State::g_devLock);
                break;
            }
            
            if(State::eDevState_PRESENT == State::g_DevState)
            {
                State::g_DevState = State::eDevState_WAIT_DEPARTURE;
                framework_WaitMutex(State::g_devLock, 0);
                if(State::eDevType_P2P == DevTypeBck)
                {
                    printf("\tDevice Lost\n\n");
                }
                DevTypeBck = State::eDevType_NONE;
            }
            else if(State::eDevType_P2P == DevTypeBck)
            {
                printf("\tDevice Lost\n\n");
            }
        }
        
        framework_UnlockMutex(State::g_devLock);
    }while(0x01);
    
    return res;
}

void strtolower(char * string) 
{
    unsigned int i = 0x00;
    
    for(i = 0; i < strlen(string); i++) 
    {
        string[i] = tolower(string[i]);
    }
}

char* strRemovceChar(const char* str, char car)
{
    unsigned int i = 0x00;
    unsigned int index = 0x00;
    char * dest = (char*)malloc((strlen(str) + 1) * sizeof(char));
    
    for(i = 0x00; i < strlen(str); i++)
    {
        if(str[i] != car)
        {
            dest[index++] = str[i];
        }
    }
    dest[index] = '\0';
    return dest;
}

int convertParamtoBuffer(char* param, unsigned char** outBuffer, unsigned int* outBufferLen)
{
    int res = 0x00;
    unsigned int i = 0x00;
    int index = 0x00;
    char atoiBuf[3];
    atoiBuf[2] = '\0';
    
    if(NULL == param || NULL == outBuffer || NULL == outBufferLen)    
    {
        printf("Parameter Error\n");
        res = 0xFF;
    }
    
    if(0x00 == res)
    {
        param = strRemovceChar(param, ' ');
    }
    
    if(0x00 == res)
    {
        if(0x00 == strlen(param) % 2)
        {
            *outBufferLen = strlen(param) / 2;
            
            *outBuffer = (unsigned char*) malloc((*outBufferLen) * sizeof(unsigned char));
            if(NULL != *outBuffer)
            {
                for(i = 0x00; i < ((*outBufferLen) * 2); i = i + 2)
                {
                    atoiBuf[0] = param[i];
                    atoiBuf[1] = param[i + 1];
                    (*outBuffer)[index++] = strtol(atoiBuf, NULL, 16);
                }
            }
            else
            {
                printf("Memory Allocation Failed\n");
                res = 0xFF;
            }
        }
        else
        {
            printf("Invalid NDEF Message Param\n");
        }
        free(param);
    }
        
    return res;
}

int BuildNDEFMessage(int arg_len, char** arg, unsigned char** outNDEFBuffer, unsigned int* outNDEFBufferLen)
{
    int res = 0x00;
    nfc_handover_cps_t cps = HANDOVER_CPS_UNKNOWN;
    unsigned char* ndef_msg = NULL;
    unsigned int ndef_msg_len = 0x00;
    char *type = NULL;
    char *uri = NULL;
    char *text = NULL;
    char* lang = NULL;
    char* mime_type = NULL;
    char* mime_data = NULL;
    char* carrier_state = NULL;
    char* carrier_name = NULL;
    char* carrier_data = NULL;
    
    if(0xFF == LookForTag(arg, arg_len, "-t", &type, 0x00) && 0xFF == LookForTag(arg, arg_len, "--type", &type, 0x01))
    {
        res = 0xFF;
        printf("Record type missing (-t)\n");
        help(0x02);
    }
    if(0x00 == res)
    {
        strtolower(type);
        if(0x00 == strcmp(type, "uri"))
        {
            if(0x00 == LookForTag(arg, arg_len, "-u", &uri, 0x00) || 0x00 == LookForTag(arg, arg_len, "--uri", &uri, 0x01))
            {
                *outNDEFBufferLen = strlen(uri) + 30; /*TODO : replace 30 by URI NDEF message header*/
                *outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
                res = ndef_createUri(uri, *outNDEFBuffer, *outNDEFBufferLen);
                if(0x00 >= res)
                {
                    printf("Failed to build URI NDEF Message\n");
                }
                else
                {
                    *outNDEFBufferLen = res;
                    res = 0x00;
                }
            }
            else
            {
                printf("URI Missing (-u)\n");
                help(0x02);
                res = 0xFF;
            }
        }
        else if(0x00 == strcmp(type, "text"))
        {
            if(0xFF == LookForTag(arg, arg_len, "-r", &text, 0x00) && 0xFF == LookForTag(arg, arg_len, "--rep", &text, 0x01))
            {
                printf("Representation missing (-r)\n");
                res = 0xFF;
            }
            
            if(0xFF == LookForTag(arg, arg_len, "-l", &lang, 0x00) && 0xFF == LookForTag(arg, arg_len, "--lang", &lang, 0x01))
            {
                printf("Language missing (-l)\n");
                res = 0xFF;
            }
            if(0x00 == res)
            {
                *outNDEFBufferLen = strlen(text) + strlen(lang) + 30; /*TODO : replace 30 by TEXT NDEF message header*/
                *outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
                res = ndef_createText(lang, text, *outNDEFBuffer, *outNDEFBufferLen);
                if(0x00 >= res)
                {
                    printf("Failed to build TEXT NDEF Message\n");
                }
                else
                {
                    *outNDEFBufferLen = res;
                    res = 0x00;
                }    
            }
            else
            {
                help(0x02);
            }
        }
        else if(0x00 == strcmp(type, "mime"))
        {
            if(0xFF == LookForTag(arg, arg_len, "-m", &mime_type, 0x00) && 0xFF == LookForTag(arg, arg_len, "--mime", &mime_type, 0x01))
            {
                printf("Mime-type missing (-m)\n");
                res = 0xFF;
            }
            if(0xFF == LookForTag(arg, arg_len, "-d", &mime_data, 0x00) && 0xFF == LookForTag(arg, arg_len, "--data", &mime_data, 0x01))
            {
                printf("NDEF Data missing (-d)\n");
                res = 0xFF;
            }
            if(0x00 == res)
            {
                *outNDEFBufferLen = strlen(mime_data) +  strlen(mime_type) + 30; /*TODO : replace 30 by MIME NDEF message header*/
                *outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
                
                res = convertParamtoBuffer(mime_data, &ndef_msg, &ndef_msg_len);
                
                if(0x00 == res)
                {
                    res = ndef_createMime(mime_type, ndef_msg, ndef_msg_len, *outNDEFBuffer, *outNDEFBufferLen);
                    if(0x00 >= res)
                    {
                        printf("Failed to build MIME NDEF Message\n");
                    }
                    else
                    {
                        *outNDEFBufferLen = res;
                        res = 0x00;
                    }
                }
            }
            else
            {
                help(0x02);
            }
        }
        else if(0x00 == strcmp(type, "hs"))
        {
            
            if(0xFF == LookForTag(arg, arg_len, "-cs", &carrier_state, 0x00) && 0xFF == LookForTag(arg, arg_len, "--carrierState", &carrier_state, 0x01))
            {
                printf("Carrier Power State missing (-cs)\n");
                res = 0xFF;
            }
            if(0xFF == LookForTag(arg, arg_len, "-cn", &carrier_name, 0x00) && 0xFF == LookForTag(arg, arg_len, "--carrierName", &carrier_name, 0x01))
            {
                printf("Carrier Reference Name missing (-cn)\n");
                res = 0xFF;
            }
            
            if(0xFF == LookForTag(arg, arg_len, "-d", &carrier_data, 0x00) && 0xFF == LookForTag(arg, arg_len, "--data", &carrier_data, 0x01))
            {
                printf("NDEF Data missing (-d)\n");
                res = 0xFF;
            }
            
            if(0x00 == res)
            {
                *outNDEFBufferLen = strlen(carrier_name) + strlen(carrier_data) + 30;  /*TODO : replace 30 by HS NDEF message header*/
                *outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
                
                strtolower(carrier_state);
                
                if(0x00 == strcmp(carrier_state, "inactive"))
                {
                    cps = HANDOVER_CPS_INACTIVE;
                }
                else if(0x00 == strcmp(carrier_state, "active"))
                {
                    cps = HANDOVER_CPS_ACTIVE;
                }
                else if(0x00 == strcmp(carrier_state, "activating"))
                {
                    cps = HANDOVER_CPS_ACTIVATING;
                }
                else
                {
                    printf("Error : unknown carrier power state %s\n", carrier_state);
                    res = 0xFF;
                }
                
                if(0x00 == res)
                {
                    res = convertParamtoBuffer(carrier_data, &ndef_msg, &ndef_msg_len);
                }
                
                if(0x00 == res)
                {
                    res = ndef_createHandoverSelect(cps, carrier_name, ndef_msg, ndef_msg_len, *outNDEFBuffer, *outNDEFBufferLen);
                    if(0x00 >= res)
                    {
                        printf("Failed to build handover select message\n");
                    }
                    else
                    {
                        *outNDEFBufferLen = res;
                        res = 0x00;
                    }
                }
            }
            else
            {
                help(0x02);
            }
        }
        else
        {
            printf("NDEF Type %s not supported\n", type);
            res = 0xFF;
        }
    }
    
    if(NULL != ndef_msg)
    {
        free(ndef_msg);
        ndef_msg = NULL;
        ndef_msg_len = 0x00;
    }
    
    return res;
}

/* if data = NULL this tag is not followed by dataStr : for example -h --help
if format = 0 tag format -t "text" if format=1 tag format : --type=text */
int LookForTag(char** args, int args_len, char* tag, char** data, int format)
{
    int res = 0xFF;
    int i = 0x00;
    int found = 0xFF;
    
    for(i = 0x00; i < args_len; i++)
    {
        found = 0xFF;
        strtolower(args[i]);
        if(0x00 == format)
        {
            found = strcmp(args[i], tag);
        }
        else
        {
            found = strncmp(args[i], tag, strlen(tag));
        }
        
        if(0x00 == found)
        {
            if(NULL != data)
            {
                if(0x00 == format)
                {
                    if(i < (args_len - 1))
                    {
                        *data = args[i + 1];
                        res = 0x00;
                        break;
                    }
                    else
                    {
                        printf("Argument missing after %s\n", tag);
                    }
                }
                else
                {
                    *data = &args[i][strlen(tag) + 1]; /* +1 to remove '='*/
                    res = 0x00;
                    break;
                }
            }
            else
            {
                res = 0x00;
                break;
            }
        }
    }
    
    return res;
}
 
void cmd_poll(int arg_len, char** arg)
{
    int res = 0x00;
    
    printf("#########################################################################################\n");
    printf("##                                       NFC demo                                      ##\n");
    printf("#########################################################################################\n");
    printf("##                                 Poll mode activated                                 ##\n");
    printf("#########################################################################################\n");
    
    InitEnv();
    if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
    {
        help(0x01);
    }
    else
    {
        res = InitMode(0x01, 0x01, 0x00);
        
        if(0x00 == res)
        {
            WaitDeviceArrival(0x01, NULL , 0x00);
        }
    
        res = DeinitPollMode();
    }
    
    printf("Leaving ...\n");
}
 
void cmd_push(int arg_len, char** arg)
{
    int res = 0x00;
    unsigned char * NDEFMsg = NULL;
    unsigned int NDEFMsgLen = 0x00;
    
    printf("#########################################################################################\n");
    printf("##                                       NFC demo                                      ##\n");
    printf("#########################################################################################\n");
    printf("##                                 Push mode activated                                 ##\n");
    printf("#########################################################################################\n");
    
    InitEnv();
    
    if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
    {
        help(0x02);
    }
    else
    {
        res = InitMode(0x01, 0x01, 0x00);
        
        if(0x00 == res)
        {
            res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
        }
        
        if(0x00 == res)
        {
            WaitDeviceArrival(0x02, NDEFMsg, NDEFMsgLen);
        }
        
        if(NULL != NDEFMsg)
        {
            free(NDEFMsg);
            NDEFMsg = NULL;
            NDEFMsgLen = 0x00;
        }
        
        res = DeinitPollMode();
    }
    
    printf("Leaving ...\n");
}

void cmd_share(int arg_len, char** arg)
{
    int res = 0x00;
    unsigned char * NDEFMsg = NULL;
    unsigned int NDEFMsgLen = 0x00;
    
    printf("#########################################################################################\n");
    printf("##                                       NFC demo                                      ##\n");
    printf("#########################################################################################\n");
    printf("##                                 Share mode activated                                ##\n");
    printf("#########################################################################################\n");
    
    InitEnv();
    
    if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
    {
        help(0x02);
    }
    else
    {
        res = InitMode(0x00, 0x00, 0x01);
        
        if(0x00 == res)
        {
            res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
        }
        
        if(0x00 == res)
        {
            T4T_NDEF_EMU_SetRecord(NDEFMsg, NDEFMsgLen, NULL);
        }
        
        if(0x00 == res)
        {
            WaitDeviceArrival(0x04, NDEFMsg, NDEFMsgLen);
        }
        
        if(NULL != NDEFMsg)
        {
            free(NDEFMsg);
            NDEFMsg = NULL;
            NDEFMsgLen = 0x00;
        }
        
        res = DeinitPollMode();
    }
    
    printf("Leaving ...\n");
}
void cmd_write(int arg_len, char** arg)
{
    int res = 0x00;
    unsigned char * NDEFMsg = NULL;
    unsigned int NDEFMsgLen = 0x00;
    
    printf("#########################################################################################\n");
    printf("##                                       NFC demo                                      ##\n");
    printf("#########################################################################################\n");
    printf("##                                 Write mode activated                                ##\n");
    printf("#########################################################################################\n");
    
    InitEnv();
    
    if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
    {
        help(0x02);
    }
    else
    {
        res = InitMode(0x01, 0x01, 0x00);
        
        if(0x00 == res)
        {
            res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
        }
        
        if(0x00 == res)
        {
            WaitDeviceArrival(0x03, NDEFMsg, NDEFMsgLen);
        }
        
        if(NULL != NDEFMsg)
        {
            free(NDEFMsg);
            NDEFMsg = NULL;
            NDEFMsgLen = 0x00;
        }
        
        res = DeinitPollMode();
    }
    
    printf("Leaving ...\n");
}

void* ExitThread(void* pContext)
{
    printf("                              ... press enter to quit ...\n\n");
    
    getchar();
    
    framework_LockMutex(State::g_SnepClientLock);
    
    if(State::eSnepClientState_WAIT_OFF == State::g_SnepClientState || State::eSnepClientState_WAIT_READY == State::g_SnepClientState)
    {
        State::g_SnepClientState = State::eSnepClientState_EXIT;
        framework_NotifyMutex(State::g_SnepClientLock, 0);
    }
    else
    {
        State::g_SnepClientState = State::eSnepClientState_EXIT;
    }
    framework_UnlockMutex(State::g_SnepClientLock);
    
    framework_LockMutex(State::g_devLock);
    
    if(State::eDevState_WAIT_ARRIVAL == State::g_DevState || State::eDevState_WAIT_DEPARTURE == State::g_DevState)
    {
        State::g_DevState = State::eDevState_EXIT;
        framework_NotifyMutex(State::g_devLock, 0);
    }
    else
    {
        State::g_DevState = State::eDevState_EXIT;
    }
    
    framework_UnlockMutex(State::g_devLock);
    return NULL;
}

int InitEnv()
{
    eResult tool_res = FRAMEWORK_SUCCESS;
    int res = 0x00;
    
    tool_res = framework_CreateMutex(&State::g_devLock);
    if(FRAMEWORK_SUCCESS != tool_res)
    {
        res = 0xFF;
    }
    
    if(0x00 == res)
    {
        tool_res = framework_CreateMutex(&State::g_SnepClientLock);
        if(FRAMEWORK_SUCCESS != tool_res)
        {
            res = 0xFF;
        }
     }
     
     if(0x00 == res)
    {
        tool_res = framework_CreateMutex(&State::g_HCELock);
        if(FRAMEWORK_SUCCESS != tool_res)
        {
            res = 0xFF;
        }
     }
     if(0x00 == res)
    {
        tool_res = framework_CreateThread(&State::g_ThreadHandle, ExitThread, NULL);
        if(FRAMEWORK_SUCCESS != tool_res)
        {
            res = 0xFF;
        }
    }
    return res;
}

int CleanEnv()
{
    if(NULL != State::g_ThreadHandle)
    {
        framework_JoinThread(State::g_ThreadHandle);
        framework_DeleteThread(State::g_ThreadHandle);
        State::g_ThreadHandle = NULL;
    }
        
    if(NULL != State::g_devLock)
    {
        framework_DeleteMutex(State::g_devLock);
        State::g_devLock = NULL;
    }
    
    if(NULL != State::g_SnepClientLock)
    {
        framework_DeleteMutex(State::g_SnepClientLock);
        State::g_SnepClientLock = NULL;
    }
    if(NULL != State::g_HCELock)
    {
        framework_DeleteMutex(State::g_HCELock);
        State::g_HCELock = NULL;
    }
    return 0x00;
}
 
int main(int argc, char ** argv)
{
    if (argc<2)
    {
        printf("Missing argument\n");
        help(0x00);
    }
    else if (strcmp(argv[1],"poll") == 0)
    {
        cmd_poll(argc - 2, &argv[2]);
    }
    else if(strcmp(argv[1],"write") == 0)
    {
        cmd_write(argc - 2, &argv[2]);
    }
    else if(strcmp(argv[1],"push") == 0)
    {
        cmd_push(argc - 2, &argv[2]);
    }
    else if(strcmp(argv[1],"share") == 0)
    {
        cmd_share(argc - 2, &argv[2]);
    }
    else
    {
        help(0x00);
    }
    printf("\n");
    
    CleanEnv();
    
    return 0;
}
 
void help(int mode)
{
    printf("\nCOMMAND: \n");
    printf("\tpoll\tPolling mode \t e.g. <nfcDemoApp poll >\n");
    printf("\twrite\tWrite tag \t e.g. <nfcDemoApp write --type=Text -l en -r \"Test\">\n");
    printf("\tshare\tTag Emulation Mode \t e.g. <nfcDemoApp share -t URI -u http://www.nxp.com>\n");
    printf("\tpush\tPush to device \t e.g. <nfcDemoApp push -t URI -u http://www.nxp.com>\n");
    printf("\t    \t               \t e.g. <nfcDemoApp push --type=mime -m \"application/vnd.bluetooth.ep.oob\" -d \"2200AC597405AF1C0E0947616C617879204E6F74652033040D0C024005031E110B11\">\n");
    printf("\n");
    printf("Help Options:\n");
    printf("-h, --help                       Show help options\n");
    printf("\n");
    
    if(0x01 == mode)
    {
        printf("No Application Options for poll mode\n");
    }
    if(0x02 == mode)
    {
        printf("Supported NDEF Types : \n");
        printf("\t URI\n");
        printf("\t TEXT\n");
        printf("\t MIME\n");
        printf("\t HS (handover select)\n");
        printf("\n");
        
        printf("Application Options : \n");
        printf("\t-l,  --lang=en                   Language\n");
        printf("\t-m,  --mime=audio/mp3            Mime-type\n");
        printf("\t-r,  --rep=sample text           Representation\n");
        printf("\t-t,  --type=Text                 Record type (Text, URI...\n");
        printf("\t-u,  --uri=http://www.nxp.com    URI\n");
        printf("\t-d,  --data=00223344565667788    NDEF Data (for type MIME & Handover select)\n");
        printf("\t-cs, --carrierState=active       Carrier State (for type Handover select)\n");
        printf("\t-cn, --carrierName=ble           Carrier Reference Name (for type Handover select)\n");
    }
    printf("\n");
}

Napi::String Method(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    return Napi::String::New(env, "world");
}



void OnPoll(const Napi::CallbackInfo& info) {
  pollEnv = info.Env();
  pollCB = info[0].As<Napi::Function>();
  // pollCB.Call(pollEnv.Global(), { Napi::String::New(pollEnv, "hello world") });
  
  
  int res = 0x00;
    
    InitEnv();
   
    res = InitMode(0x01, 0x01, 0x00);
    
    if(0x00 == res)
    {
        WaitDeviceArrival(0x01, NULL , 0x00);
    }

    res = DeinitPollMode();
    

}

void OnError(const Napi::CallbackInfo& info) {
  errorEnv = info.Env();
  errorCB = info[0].As<Napi::Function>();
}
/*
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(
        Napi::String::New(env, "hello"),
        Napi::Function::New(env, Method)
    );
    
    exports.Set(
        Napi::String::New(env, "poll"),
        Napi::Function::New(env, OnPoll)
    );
    
    exports.Set(
      Napi::String::New(env, "error"),
      Napi::Function::New(env, OnError)
    );

    return exports;
}
*/
// NODE_API_MODULE(hello, Init)
