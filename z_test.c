#include <stdio.h>
#include "osal.h"

#include "zstack.h"
#include "zstackapi.h"
#include "zcl.h"
#include "zcl_port.h"
#include "zcl_ha.h"
#include "zcl_ms.h"
#include "zcl_general.h"
#include "util_timer.h"

static SemaphoreP_Handle appSemHandle = NULL;
static SemaphoreP_Params appSemParams = {0}; // SemaphoreP_Mode_COUNTING
static uint32_t appTaskEvents = 0;
static void *appTaskHandle;
static uint8_t appTaskId;

static ClockP_Handle rejoinClkHandle = NULL;
static ClockP_Struct rejoinClkStruct = {0};
#define SAMPLEAPP_END_DEVICE_REJOIN_EVT 0x0002
#define SAMPLEAPP_END_DEVICE_REJOIN_DELAY 100

#define SENSOR_ENDPOINT 1
#define SENSOR_DEVICE_VERSION 0
#define SENSOR_FLAGS 0
static endPointDesc_t sensor_EpDesc = {0};
static zclGeneral_AppCallbacks_t sensor_CmdCallbacks = {0};

#define ZCLSENSOR_MAX_INCLUSTERS 3
const cId_t sensor_InClusterList[ZCLSENSOR_MAX_INCLUSTERS] = {
    ZCL_CLUSTER_ID_GENERAL_BASIC,
    ZCL_CLUSTER_ID_GENERAL_IDENTIFY,
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
};

#define ZCLSENSOR_MAX_OUTCLUSTERS 1
const cId_t sensor_OutClusterList[ZCLSENSOR_MAX_OUTCLUSTERS] = {
    ZCL_CLUSTER_ID_GENERAL_IDENTIFY,
};

SimpleDescriptionFormat_t sensor_SimpleDesc = {
    SENSOR_ENDPOINT,                 //  int Endpoint;
    ZCL_HA_PROFILE_ID,               //  uint16_t AppProfId[2];
    ZCL_DEVICEID_TEMPERATURE_SENSOR, //  uint16_t AppDeviceId[2];
    SENSOR_DEVICE_VERSION,           //  int   AppDevVer:4;
    SENSOR_FLAGS,                    //  int   AppFlags:4;
    ZCLSENSOR_MAX_INCLUSTERS,        //  byte  AppNumInClusters;
    (cId_t *)sensor_InClusterList,   //  byte *pAppInClusterList;
    ZCLSENSOR_MAX_OUTCLUSTERS,       //  byte  AppNumInClusters;
    (cId_t *)sensor_OutClusterList   //  byte *pAppInClusterList;
};
// Global attributes
const uint16_t sensor_basic_clusterRevision = 0x0002;
const uint16_t sensor_identify_clusterRevision = 0x0001;
const uint16_t sensor_temperaturems_clusterRevision = 0x0001;
// Basic Cluster
const uint8_t sensor_HWRevision = 3;
const uint8_t sensor_ZCLVersion = 3;
const uint8_t sensor_ManufacturerName[] = {5, 'W', 'i', 'z', 'I', 'O'};
const uint8_t sensor_PowerSource = POWER_SOURCE_BATTERY;
uint8_t sensor_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
// Identify Cluster
uint16_t sensor_IdentifyTime;
// Temperature Sensor Cluster
int16_t sensor_MeasuredValue = 2700;           // 27.00C;
const int16_t sensor_MinMeasuredValue = 1000;  // 10.00C;
const uint16_t sensor_MaxMeasuredValue = 4000; // 40.00C;
// Attributes
CONST zclAttrRec_t sensor_Attrs[] = {
    // *** General Basic Cluster Attributes ***
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {// Attribute record
         ATTRID_BASIC_ZCL_VERSION,
         ZCL_DATATYPE_UINT8,
         ACCESS_CONTROL_READ,
         (void *)&sensor_ZCLVersion}},
    {ZCL_CLUSTER_ID_GENERAL_BASIC, // Cluster IDs - defined in the foundation (ie. zcl.h)
     {
         // Attribute record
         ATTRID_BASIC_HW_VERSION,   // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
         ZCL_DATATYPE_UINT8,        // Data Type - found in zcl.h
         ACCESS_CONTROL_READ,       // Variable access control - found in zcl.h
         (void *)&sensor_HWRevision // Pointer to attribute variable
     }},
    {ZCL_CLUSTER_ID_GENERAL_BASIC,
     {// Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)sensor_ManufacturerName}},
    {ZCL_CLUSTER_ID_GENERAL_BASIC,
     {// Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&sensor_PowerSource}},
    {ZCL_CLUSTER_ID_GENERAL_BASIC,
     {// Attribute record
      ATTRID_BASIC_PHYSICAL_ENVIRONMENT,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&sensor_PhysicalEnvironment}},
    {ZCL_CLUSTER_ID_GENERAL_BASIC,
     {// Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&sensor_basic_clusterRevision}},
    // *** Identify Cluster Attribute ***
    {
        ZCL_CLUSTER_ID_GENERAL_IDENTIFY,
        {// Attribute record
         ATTRID_IDENTIFY_IDENTIFY_TIME,
         ZCL_DATATYPE_UINT16,
         (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
         (void *)&sensor_IdentifyTime}},
    {ZCL_CLUSTER_ID_GENERAL_IDENTIFY,
     {// Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_GLOBAL,
      (void *)&sensor_identify_clusterRevision}},

    // *** Temperature Measurement Attriubtes ***
    {
        ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
        {// Attribute record
         ATTRID_TEMPERATURE_MEASUREMENT_MEASURED_VALUE,
         ZCL_DATATYPE_INT16,
         ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
         (void *)&sensor_MeasuredValue}},
    {ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
     {// Attribute record
      ATTRID_TEMPERATURE_MEASUREMENT_MIN_MEASURED_VALUE,
      ZCL_DATATYPE_INT16,
      ACCESS_CONTROL_READ,
      (void *)&sensor_MinMeasuredValue}},
    {ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
     {// Attribute record
      ATTRID_TEMPERATURE_MEASUREMENT_MAX_MEASURED_VALUE,
      ZCL_DATATYPE_INT16,
      ACCESS_CONTROL_READ,
      (void *)&sensor_MaxMeasuredValue}},

    {ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
     {// Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&sensor_temperaturems_clusterRevision}},
};
uint8_t CONST sensor_NumAttributes = (sizeof(sensor_Attrs) / sizeof(sensor_Attrs[0]));

void sensor_ResetAttributesToDefaultValues(void)
{
    sensor_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
#ifdef ZCL_IDENTIFY
    sensor_IdentifyTime = 0;
#endif
}

static void sensor_SetupZStackCallbacks(void)
{
    zstack_devZDOCBReq_t zdoCBReq = {0};
    zdoCBReq.has_devStateChange = true;
    zdoCBReq.devStateChange = true;
    (void)Zstackapi_DevZDOCBReq(appTaskId, &zdoCBReq);
}

#ifdef ZCL_READ
static uint8_t sensor_ProcessInReadRspCmd(zclIncoming_t *pInMsg)
{
    zclReadRspCmd_t *readRspCmd;
    uint8_t i;
    readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
    for (i = 0; i < readRspCmd->numAttr; i++)
    {
        // Notify the originator of the results of the original read attributes attempt and, for each successfull request, the value of the requested attribute
    }
    return (TRUE);
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
static uint8_t sensor_ProcessInWriteRspCmd(zclIncoming_t *pInMsg)
{
    zclWriteRspCmd_t *writeRspCmd;
    uint8_t i;
    writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
    for (i = 0; i < writeRspCmd->numAttr; i++)
    {
        // Notify the device of the results of the its original write attributes command.
    }
    return (TRUE);
}
#endif // ZCL_WRITE

#ifdef ZCL_DISCOVER
static uint8_t sensor_ProcessInDiscCmdsRspCmd(zclIncoming_t *pInMsg)
{
    zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
    uint8_t i;
    discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
    for (i = 0; i < discoverRspCmd->numCmd; i++)
    {
        // Device is notified of the result of its attribute discovery command.
    }
    return (TRUE);
}

static uint8_t sensor_ProcessInDiscAttrsRspCmd(zclIncoming_t *pInMsg)
{
    zclDiscoverAttrsRspCmd_t *discoverRspCmd;
    uint8_t i;
    discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
    for (i = 0; i < discoverRspCmd->numAttr; i++)
    {
        // Device is notified of the result of its attribute discovery command.
    }
    return (TRUE);
}

static uint8_t sensor_ProcessInDiscAttrsExtRspCmd(zclIncoming_t *pInMsg)
{
    zclDiscoverAttrsExtRsp_t *discoverRspCmd;
    uint8_t i;
    discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
    for (i = 0; i < discoverRspCmd->numAttr; i++)
    {
        // Device is notified of the result of its attribute discovery command.
    }
    return (TRUE);
}
#endif // ZCL_DISCOVER

static uint8_t sensor_ProcessInDefaultRspCmd(zclIncoming_t *pInMsg)
{
    // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
    // Device is notified of the Default Response command.
    (void)pInMsg;
    return (TRUE);
}

static uint8_t sensor_ProcessIncomingMsg(zclIncoming_t *pInMsg)
{
    uint8_t handled = FALSE;
    switch (pInMsg->hdr.commandID)
    {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
        sensor_ProcessInReadRspCmd(pInMsg);
        handled = TRUE;
        break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
        sensor_ProcessInWriteRspCmd(pInMsg);
        handled = TRUE;
        break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
        //sensor_ProcessInConfigReportCmd( pInMsg );
        break;
    case ZCL_CMD_READ_REPORT_CFG:
        //sensor_ProcessInReadReportCfgCmd( pInMsg );
        break;
    case ZCL_CMD_CONFIG_REPORT_RSP:
        //sensor_ProcessInConfigReportRspCmd( pInMsg );
        break;
    case ZCL_CMD_READ_REPORT_CFG_RSP:
        //sensor_ProcessInReadReportCfgRspCmd( pInMsg );
        break;

    case ZCL_CMD_REPORT:
        //sensor_ProcessInReportCmd( pInMsg );
        break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
        sensor_ProcessInDefaultRspCmd(pInMsg);
        handled = TRUE;
        break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
        sensor_ProcessInDiscCmdsRspCmd(pInMsg);
        handled = TRUE;
        break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
        sensor_ProcessInDiscCmdsRspCmd(pInMsg);
        handled = TRUE;
        break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
        sensor_ProcessInDiscAttrsRspCmd(pInMsg);
        handled = TRUE;
        break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
        sensor_ProcessInDiscAttrsExtRspCmd(pInMsg);
        handled = TRUE;
        break;
#endif
    default:
        break;
    }

    return handled;
}

static void sensor_RejoinTimeoutCallback(unsigned int arg)
{
    printf("[APP] %s()\n", __func__);
    (void)arg; // Parameter is not used
    appTaskEvents |= SAMPLEAPP_END_DEVICE_REJOIN_EVT;
    SemaphoreP_post(appSemHandle); // Wake up the application thread when it waits for clock event
}

static void sensor_processCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
    switch (bdbCommissioningModeMsg->bdbCommissioningMode)
    {
    case BDB_COMMISSIONING_FORMATION:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
            //YOUR JOB:
            printf("[APP] BDB_COMMISSIONING_FORMATION\n");
        }
        else
        {
            //Want to try other channels?
            //try with bdb_setChannelAttribute
            printf("[ERROR] BDB_COMMISSIONING_FORMATION\n");
        }
        break;
    case BDB_COMMISSIONING_NWK_STEERING:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
            //YOUR JOB:
            //We are on the nwk, what now?
            printf("[APP] BDB_COMMISSIONING_NWK_STEERING\n");
        }
        else
        {
            //See the possible errors for nwk steering procedure
            //No suitable networks found
            //Want to try other channels?
            //try with bdb_setChannelAttribute
            printf("[ERROR] BDB_COMMISSIONING_NWK_STEERING\n");
        }
        break;
    case BDB_COMMISSIONING_FINDING_BINDING:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
            //YOUR JOB:
            printf("[APP] BDB_COMMISSIONING_FINDING_BINDING\n");
        }
        else
        {
            //YOUR JOB:
            //retry?, wait for user interaction?
            printf("[ERROR] BDB_COMMISSIONING_FINDING_BINDING\n");
        }
        break;
    case BDB_COMMISSIONING_INITIALIZATION:
        //Initialization notification can only be successful. Failure on initialization
        //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification
        //YOUR JOB:
        //We are on a network, what now?
        printf("[APP] BDB_COMMISSIONING_INITIALIZATION\n");
        //bdb_StartCommissioning(DEFAULT_COMISSIONING_MODE);
        break;
#if ZG_BUILD_ENDDEVICE_TYPE
    case BDB_COMMISSIONING_PARENT_LOST:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
        {
            //We did recover from losing parent
            printf("[APP] BDB_COMMISSIONING_PARENT_LOST\n");
        }
        else
        {
            //Parent not found, attempt to rejoin again after a fixed delay
            UtilTimer_setTimeout(rejoinClkHandle, SAMPLEAPP_END_DEVICE_REJOIN_DELAY);
            UtilTimer_start(&rejoinClkStruct);
            printf("[ERROR] BDB_COMMISSIONING_PARENT_LOST\n");
        }
        break;
#endif
    }
}

static void sensor_processAfIncomingMsgInd(zstack_afIncomingMsgInd_t *pInMsg)
{
    afIncomingMSGPacket_t afMsg;
    /*
     * All incoming messages are passed to the ZCL message processor,
     * first convert to a structure that ZCL can process.
     */
    afMsg.groupId = pInMsg->groupID;
    afMsg.clusterId = pInMsg->clusterId;
    afMsg.srcAddr.endPoint = pInMsg->srcAddr.endpoint;
    afMsg.srcAddr.panId = pInMsg->srcAddr.panID;
    afMsg.srcAddr.addrMode = (afAddrMode_t)pInMsg->srcAddr.addrMode;
    if ((afMsg.srcAddr.addrMode == afAddr16Bit) || (afMsg.srcAddr.addrMode == afAddrGroup) || (afMsg.srcAddr.addrMode == afAddrBroadcast))
    {
        afMsg.srcAddr.addr.shortAddr = pInMsg->srcAddr.addr.shortAddr;
    }
    else if (afMsg.srcAddr.addrMode == afAddr64Bit)
    {
        OsalPort_memcpy(afMsg.srcAddr.addr.extAddr, &(pInMsg->srcAddr.addr.extAddr), 8);
    }
    afMsg.macDestAddr = pInMsg->macDestAddr;
    afMsg.endPoint = pInMsg->endpoint;
    afMsg.wasBroadcast = pInMsg->wasBroadcast;
    afMsg.LinkQuality = pInMsg->linkQuality;
    afMsg.correlation = pInMsg->correlation;
    afMsg.rssi = pInMsg->rssi;
    afMsg.SecurityUse = pInMsg->securityUse;
    afMsg.timestamp = pInMsg->timestamp;
    afMsg.nwkSeqNum = pInMsg->nwkSeqNum;
    afMsg.macSrcAddr = pInMsg->macSrcAddr;
    afMsg.radius = pInMsg->radius;
    afMsg.cmd.DataLength = pInMsg->n_payload;
    afMsg.cmd.Data = pInMsg->pPayload;
    zcl_ProcessMessageMSG(&afMsg);
}

static void sensor_processZStackMsgs(zstackmsg_genericReq_t *pMsg)
{
    printf("[A] MSG:%X\n", (int)pMsg->hdr.event);
    switch (pMsg->hdr.event)
    {
    case zstackmsg_CmdIDs_BDB_NOTIFICATION: // C5
    {
        zstackmsg_bdbNotificationInd_t *pInd;
        pInd = (zstackmsg_bdbNotificationInd_t *)pMsg;
        sensor_processCommissioningStatus(&(pInd->Req));
    }
    break;

    case zstackmsg_CmdIDs_BDB_IDENTIFY_TIME_CB:
    {
        //CUI_DISABLE
    }
    break;

    case zstackmsg_CmdIDs_BDB_BIND_NOTIFICATION_CB:
    {
        //CUI_DISABLE
    }
    break;

    case zstackmsg_CmdIDs_AF_INCOMING_MSG_IND: // 92
    {
        // Process incoming data messages
        zstackmsg_afIncomingMsgInd_t *pInd;
        pInd = (zstackmsg_afIncomingMsgInd_t *)pMsg;
        sensor_processAfIncomingMsgInd(&(pInd->req));
    }
    break;

#if (ZG_BUILD_JOINING_TYPE)
    case zstackmsg_CmdIDs_BDB_CBKE_TC_LINK_KEY_EXCHANGE_IND: // CA ?
    {
        zstack_bdbCBKETCLinkKeyExchangeAttemptReq_t zstack_bdbCBKETCLinkKeyExchangeAttemptReq;
        /* Z3.0 has not defined CBKE yet, so lets attempt default TC Link Key exchange procedure by reporting CBKE failure. */
        zstack_bdbCBKETCLinkKeyExchangeAttemptReq.didSuccess = FALSE;
        Zstackapi_bdbCBKETCLinkKeyExchangeAttemptReq(appTaskId, &zstack_bdbCBKETCLinkKeyExchangeAttemptReq);
    }
    break;

    case zstackmsg_CmdIDs_BDB_FILTER_NWK_DESCRIPTOR_IND: // CB
    {
        /*User logic to remove networks that do not want to join Networks to be removed can be released with Zstackapi_bdbNwkDescFreeReq */
        Zstackapi_bdbFilterNwkDescComplete(appTaskId);
    }
    break;
#endif

    case zstackmsg_CmdIDs_DEV_STATE_CHANGE_IND: // 94
    {
        //CUI_DISABLE
    }
    break;

#ifdef BDB_TL_TARGET
    case zstackmsg_CmdIDs_BDB_TOUCHLINK_TARGET_ENABLE_IND:
    {
        ////zstackmsg_bdbTouchLinkTargetEnableInd_t *pInd = (zstackmsg_bdbTouchLinkTargetEnableInd_t *)pMsg;
        ////uiProcessTouchlinkTargetEnable(pInd->Enable);
    }
    break;
#endif

    case zstackmsg_CmdIDs_DEV_PERMIT_JOIN_IND:
    case zstackmsg_CmdIDs_BDB_TC_LINK_KEY_EXCHANGE_NOTIFICATION_IND:
    case zstackmsg_CmdIDs_AF_DATA_CONFIRM_IND:
    case zstackmsg_CmdIDs_ZDO_DEVICE_ANNOUNCE:
    case zstackmsg_CmdIDs_ZDO_NWK_ADDR_RSP:
    case zstackmsg_CmdIDs_ZDO_IEEE_ADDR_RSP:
    case zstackmsg_CmdIDs_ZDO_NODE_DESC_RSP:
    case zstackmsg_CmdIDs_ZDO_POWER_DESC_RSP:
    case zstackmsg_CmdIDs_ZDO_SIMPLE_DESC_RSP:
    case zstackmsg_CmdIDs_ZDO_ACTIVE_EP_RSP:
    case zstackmsg_CmdIDs_ZDO_COMPLEX_DESC_RSP:
    case zstackmsg_CmdIDs_ZDO_USER_DESC_RSP:
    case zstackmsg_CmdIDs_ZDO_USER_DESC_SET_RSP:
    case zstackmsg_CmdIDs_ZDO_SERVER_DISC_RSP:
    case zstackmsg_CmdIDs_ZDO_END_DEVICE_BIND_RSP:
    case zstackmsg_CmdIDs_ZDO_BIND_RSP:
    case zstackmsg_CmdIDs_ZDO_UNBIND_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_NWK_DISC_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_LQI_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_RTG_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_BIND_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_LEAVE_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_DIRECT_JOIN_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_PERMIT_JOIN_RSP:
    case zstackmsg_CmdIDs_ZDO_MGMT_NWK_UPDATE_NOTIFY:
    case zstackmsg_CmdIDs_ZDO_SRC_RTG_IND:
    case zstackmsg_CmdIDs_ZDO_CONCENTRATOR_IND:
    case zstackmsg_CmdIDs_ZDO_LEAVE_CNF:
    case zstackmsg_CmdIDs_ZDO_LEAVE_IND:
    case zstackmsg_CmdIDs_SYS_RESET_IND:
    case zstackmsg_CmdIDs_AF_REFLECT_ERROR_IND:
    case zstackmsg_CmdIDs_ZDO_TC_DEVICE_IND:
        break;

    default:
        break;
    }
}

static void sensor_initParameters(void)
{
    zstack_bdbSetAttributesReq_t zstack_bdbSetAttrReq;
    zstack_bdbSetAttrReq.bdbCommissioningGroupID = BDB_DEFAULT_COMMISSIONING_GROUP_ID;
    zstack_bdbSetAttrReq.bdbPrimaryChannelSet = BDB_DEFAULT_PRIMARY_CHANNEL_SET;
    zstack_bdbSetAttrReq.bdbScanDuration = BDB_DEFAULT_SCAN_DURATION;
    zstack_bdbSetAttrReq.bdbSecondaryChannelSet = BDB_DEFAULT_SECONDARY_CHANNEL_SET;
    zstack_bdbSetAttrReq.has_bdbCommissioningGroupID = TRUE;
    zstack_bdbSetAttrReq.has_bdbPrimaryChannelSet = TRUE;
    zstack_bdbSetAttrReq.has_bdbScanDuration = TRUE;
    zstack_bdbSetAttrReq.has_bdbSecondaryChannelSet = TRUE;
#if (ZG_BUILD_JOINING_TYPE)
    zstack_bdbSetAttrReq.has_bdbTCLinkKeyExchangeAttemptsMax = TRUE;
    zstack_bdbSetAttrReq.has_bdbTCLinkKeyExchangeMethod = TRUE;
    zstack_bdbSetAttrReq.bdbTCLinkKeyExchangeAttemptsMax = BDB_DEFAULT_TC_LINK_KEY_EXCHANGE_ATTEMPS_MAX;
    zstack_bdbSetAttrReq.bdbTCLinkKeyExchangeMethod = BDB_DEFAULT_TC_LINK_KEY_EXCHANGE_METHOD;
#endif
    Zstackapi_bdbSetAttributesReq(appTaskId, &zstack_bdbSetAttrReq);
}

static void sensor_init(void)
{
    //Register Endpoint
    sensor_EpDesc.endPoint = SENSOR_ENDPOINT;
    sensor_EpDesc.simpleDesc = &sensor_SimpleDesc;
    zclport_registerEndpoint(appTaskId, &sensor_EpDesc); // BLOCKED

    // Register the ZCL General Cluster Library callback functions
    zclGeneral_RegisterCmdCallbacks(SENSOR_ENDPOINT, &sensor_CmdCallbacks);

    // Register the application's attribute list and reset to default values
    sensor_ResetAttributesToDefaultValues();
    zcl_registerAttrList(SENSOR_ENDPOINT, sensor_NumAttributes, sensor_Attrs);

    // Register the Application to receive the unprocessed Foundation command/response messages
    zclport_registerZclHandleExternal(SENSOR_ENDPOINT, sensor_ProcessIncomingMsg);

    //Write the bdb initialization parameters
    sensor_initParameters();

    //Setup ZDO callbacks
    sensor_SetupZStackCallbacks();

#ifdef BDB_REPORTING
    //Adds the default configuration values for the temperature attribute of the ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT cluster, for endpoint SENSOR_ENDPOINT
    zstack_bdbRepAddAttrCfgRecordDefaultToListReq_t Req = {0};
    Req.attrID = ATTRID_TEMPERATURE_MEASUREMENT_MEASURED_VALUE;
    Req.cluster = ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT;
    Req.endpoint = SENSOR_ENDPOINT;
    Req.maxReportInt = 10;
    Req.minReportInt = 3;
    uint8_t reportableChange[] = {0x2C, 0x01, 0x00, 0x00}; // ?
    OsalPort_memcpy(Req.reportableChange, reportableChange, BDBREPORTING_MAX_ANALOG_ATTR_SIZE);
    Zstackapi_bdbRepAddAttrCfgRecordDefaultToListReq(appTaskId, &Req);
#endif

    // Call BDB initialization. Should be called once from application at startup to restore  previous network configuration, if applicable.
    zstack_bdbStartCommissioningReq_t zstack_bdbStartCommissioningReq;
    zstack_bdbStartCommissioningReq.commissioning_mode = 0;
    Zstackapi_bdbStartCommissioningReq(appTaskId, &zstack_bdbStartCommissioningReq);

    //printf("\n[APP] %s( DONE )\n\n", __func__);
}

void zclAppProcess(bool blocked)
{
    if (blocked)
    {
        SemaphoreP_pend(appSemHandle, BIOS_WAIT_FOREVER);
    }
    else
    {
        if (0 != SemaphoreP_pend(appSemHandle, 1))
            return;
    }

    //printf("[A] ID:7 EV:%04X\n", (int)appTaskEvents);

    zstackmsg_genericReq_t *pMsg = NULL;
    bool msgProcessed = FALSE;

    /* Retrieve the response message */
    if ((pMsg = (zstackmsg_genericReq_t *)OsalPort_msgReceive(appTaskId)) != NULL)
    {
        /* Process the message from the stack */
        sensor_processZStackMsgs(pMsg);
        // Free any separately allocated memory
        msgProcessed = Zstackapi_freeIndMsg(pMsg);
    }

    if ((msgProcessed == FALSE) && (pMsg != NULL))
    {
        OsalPort_msgDeallocate((uint8_t *)pMsg);
    }

#if ZG_BUILD_ENDDEVICE_TYPE
    if (appTaskEvents & SAMPLEAPP_END_DEVICE_REJOIN_EVT)
    {
        zstack_bdbRecoverNwkRsp_t zstack_bdbRecoverNwkRsp;
        Zstackapi_bdbRecoverNwkReq(appTaskId, &zstack_bdbRecoverNwkRsp);
        appTaskEvents &= ~SAMPLEAPP_END_DEVICE_REJOIN_EVT;
    }
#endif
}

void zclAppInit(void)
{
    appTaskHandle = Task_self();

    rejoinClkHandle = UtilTimer_construct(&rejoinClkStruct, sensor_RejoinTimeoutCallback, SAMPLEAPP_END_DEVICE_REJOIN_DELAY, 0, false, 0);

    appSemHandle = SemaphoreP_create(0, &appSemParams);

    appTaskId = OsalPort_registerTask(appTaskHandle, appSemHandle, &appTaskEvents);
    printf("[APP] %s( ID: %d )\n", __func__, (int)appTaskId);

    sensor_init();
    printf("[APP] %s( DONE )\n", __func__);

    //bdb_StartCommissioning(BDB_COMMISSIONING_REJOIN_EXISTING_NETWORK_ON_STARTUP);
    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    //bdb_StartCommissioning(DEFAULT_COMISSIONING_MODE);
}
