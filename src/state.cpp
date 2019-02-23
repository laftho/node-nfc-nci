#include "state.h"
#include <stddef.h>
#include "deviceinterface.h"
#include <linux_nfc_api.h>

namespace State {
  void* g_ThreadHandle = NULL;
  void* g_devLock = NULL;
  void* g_SnepClientLock = NULL;
  void* g_HCELock = NULL;
  eDevState g_DevState = eDevState_NONE;
  eDevType g_Dev_Type = eDevType_NONE;
  eSnepClientState g_SnepClientState = eSnepClientState_OFF;
  eHCEState g_HCEState = eHCEState_NONE;
  nfc_tag_info_t g_TagInfo;
  nfcTagCallback_t g_TagCB;
  nfcHostCardEmulationCallback_t g_HceCB;
  nfcSnepServerCallback_t g_SnepServerCB;
  nfcSnepClientCallback_t g_SnepClientCB;
}
