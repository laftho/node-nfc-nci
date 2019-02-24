#ifndef STATE_H
#define STATE_H

#include <stddef.h>
#include "deviceinterface.h"
#include <linux_nfc_api.h>

namespace State {
  typedef enum eDevState
  {
      eDevState_NONE,
      eDevState_WAIT_ARRIVAL,
      eDevState_PRESENT,
      eDevState_WAIT_DEPARTURE,
      eDevState_DEPARTED,
      eDevState_EXIT
  }eDevState;
  typedef enum eSnepClientState
  {
      eSnepClientState_WAIT_OFF,
      eSnepClientState_OFF,
      eSnepClientState_WAIT_READY,
      eSnepClientState_READY,
      eSnepClientState_EXIT
  }eSnepClientState;
  typedef enum eHCEState
  {
      eHCEState_NONE,
      eHCEState_WAIT_DATA,
      eHCEState_DATA_RECEIVED,
      eHCEState_EXIT
  }eHCEState;
  typedef enum eDevType
  {
      eDevType_NONE,
      eDevType_TAG,
      eDevType_P2P,
      eDevType_READER
  }eDevType;
  typedef enum T4T_NDEF_EMU_state_t
  {
      Ready,
      NDEF_Application_Selected,
      CC_Selected,
      NDEF_Selected
  } T4T_NDEF_EMU_state_t;

  extern void* g_ThreadHandle;
  extern void* g_devLock;
  extern void* g_SnepClientLock;
  extern void* g_HCELock;
  extern eDevState g_DevState;
  extern eDevType g_Dev_Type;
  extern eSnepClientState g_SnepClientState;
  extern eHCEState g_HCEState;
  extern nfc_tag_info_t g_TagInfo;
  extern nfcTagCallback_t g_TagCB;
  extern nfcHostCardEmulationCallback_t g_HceCB;
  extern nfcSnepServerCallback_t g_SnepServerCB;
  extern nfcSnepClientCallback_t g_SnepClientCB;
}

#endif
