#include "osal.h"
#include "zstackconfig.h"
#include "nvocmp.h"
#include "nwk.h"
#include "zd_app.h"
#include "zd_nwk_mgr.h"
#include "bdb.h"
#include "aps_frag.h"
#include "zstacktask.h"
#include "zstackapi.h"
#include "zcl.h"

TRNG_Handle TRNG_handle;
AESCCM_Handle ZAESCCM_handle;
AESECB_Handle ZAESECB_handle;

static SemaphoreP_Handle semHandle;
static SemaphoreP_Params semParam = {0}; // SemaphoreP_Mode_COUNTING
static void *taskHandle;
static uint8_t stackServiceId;
uint8_t getStackServiceId(void) { return stackServiceId; }

uint8_t aExtendedAddress[8] = {7, 6, 5, 4, 3, 2, 1, 0}; // init just for test

static zstack_Config_t zstack_userCfg = {
    {0, 0, 0, 0, 0, 0, 0, 0}, // Extended Address [8]
    {0, 0, 0, 0, 0, 0, 0, 0}, // NV function pointers ( is not [8] )
    0,                        // Application thread ID
    0,                        // stack image init fail flag
    MAC_USER_CFG,             //
};
zstack_Config_t *pZStackCfg = &zstack_userCfg;

typedef uint32_t (*pZTaskEventHandlerFn)(uint8_t task_id, uint32_t event);
const pZTaskEventHandlerFn zstackTasksArr[] = {
    ZMacEventLoop,          // [0]
    nwk_event_loop,         // [1]
    APS_event_loop,         // [2]
    APSF_ProcessEvent,      // [3]
    ZDApp_event_loop,       // [4]
    bdb_event_loop,         // [5]
    ZStackTaskProcessEvent, // [6]
};
const uint8_t zstackTasksCnt = sizeof(zstackTasksArr) / sizeof(zstackTasksArr[0]);
static uint32_t macServiceEvents = 0;    // [0]
static uint32_t nwkServiceEvents = 0;    // [1]
static uint32_t apsServiceEvents = 0;    // [2]
static uint32_t apsfServiceEvents = 0;   // [3]
static uint32_t zdAppServiceEvents = 0;  // [4]
static uint32_t bdbServiceEvents = 0;    // [5]
static uint32_t zstackServiceEvents = 0; // [6]
uint32_t **pTasksEvents;

static void stackServiceFxnsInit(void)
{
    uint8_t tmpServiceId, idx = 0;
    pTasksEvents = OsalPort_malloc(sizeof(uint32_t) * zstackTasksCnt);
    if (pTasksEvents != NULL)
    {
        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &macServiceEvents);
        MAP_macMainSetTaskId(tmpServiceId); // 0
        pTasksEvents[idx++] = &macServiceEvents;

        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &nwkServiceEvents);
        nwk_init(tmpServiceId); // 1
        pTasksEvents[idx++] = &nwkServiceEvents;

        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &apsServiceEvents);
        APS_Init(tmpServiceId); // 2
        pTasksEvents[idx++] = &apsServiceEvents;

        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &apsfServiceEvents);
        APSF_Init(tmpServiceId); // 3
        pTasksEvents[idx++] = &apsfServiceEvents;

        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &zdAppServiceEvents);
        ZDApp_Init(tmpServiceId); // 4
        pTasksEvents[idx++] = &zdAppServiceEvents;

        tmpServiceId = OsalPort_registerTask(taskHandle, semHandle, &bdbServiceEvents);
        bdb_Init(tmpServiceId); // 5
        pTasksEvents[idx++] = &bdbServiceEvents;

        stackServiceId = OsalPort_registerTask(taskHandle, semHandle, &zstackServiceEvents);
        ZStackTaskInit(stackServiceId); // 6
        Zstackapi_init(stackServiceId);
        pTasksEvents[idx++] = &zstackServiceEvents;
    }
    else
    {
        // handle malloc error
    }
}

void z_init(void)
{
    taskHandle = Task_self();
    semHandle = SemaphoreP_create(0, &semParam);

    extern void TIMAC_ROM_Init();
    TIMAC_ROM_Init();

    { // THE DRIVERS
        if (Random_seedAutomatic() != Random_STATUS_SUCCESS)
            System_abort("Random_seedAutomatic() failed\n");

        TRNG_Params TRNGParams;
        TRNG_init();
        TRNG_Params_init(&TRNGParams);
        TRNGParams.returnBehavior = TRNG_RETURN_BEHAVIOR_POLLING;
        TRNG_handle = TRNG_open(0, &TRNGParams);
        //printf(" * TRNG_init( %p )\n", TRNG_handle);

        AESCCM_Params AESCCMParams;
        AESCCM_init(); // Initialize AESCCM Driver
        AESCCM_Params_init(&AESCCMParams);
        AESCCMParams.returnBehavior = AESCCM_RETURN_BEHAVIOR_POLLING;
        ZAESCCM_handle = AESCCM_open(0, &AESCCMParams);
        //printf(" * AESCCM_init( %p )\n", ZAESCCM_handle);

        AESECB_Params AESECBParams;
        AESECB_init(); // Initialize AESECB Driver
        AESECB_Params_init(&AESECBParams);
        AESECBParams.returnBehavior = AESECB_RETURN_BEHAVIOR_POLLING;
        ZAESECB_handle = AESECB_open(0, &AESECBParams);
        //printf(" * AESECB_init( %p )\n", ZAESECB_handle);
    }

    { // CONFIG
        extern void assertHandler(void);
        pZStackCfg->macConfig.pAssertFP = assertHandler;
        pZStackCfg->initFailStatus = ZSTACKCONFIG_INIT_SUCCESS;
        OsalPort_memcpy(aExtendedAddress, (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0), (APIMAC_SADDR_EXT_LEN));
        OsalPort_memcpy(pZStackCfg->extendedAddress, aExtendedAddress, (APIMAC_SADDR_EXT_LEN));
#if 0
        NVOCMP_loadApiPtrs(&pZStackCfg->nvFps); // load MV API
        if (pZStackCfg->nvFps.initNV)
            pZStackCfg->nvFps.initNV(NULL);
#endif
    }

    { // INIT

        uintptr_t key = HwiP_disable();           /* Disable interrupts */
        macSetUserConfig(&pZStackCfg->macConfig); //
        ZMacInit();                               // MAC_Init() MAC_InitDevice()
        zgInit();                                 // Initialize basic NV items
        afInit();                                 //
        stackServiceFxnsInit();                   //
        nwk_InitializeDefaultPollRates();         // Initialize default poll rates
        macLowLevelBufferInit();                  // Initialize MAC buffer
        HwiP_restore(key);                        /* Enable interrupts */
    }

    macLowLevelInit();    // Must be done last
    ZMacSrcMatchEnable(); // same as rfSetConfigIeee()

    //printf("\n[+] ZMacReset\n");
    ZMacReset(TRUE); // reset PIB
    //printf("[-] ZMacReset\n");

    ZMacSetZigbeeMACParams(); // power

    //printf("\n[+] nwk_SetCurrentPollRateType\n");
    if (ZG_DEVICE_ENDDEVICE_TYPE && zgRxAlwaysOn == TRUE)
        nwk_SetCurrentPollRateType(POLL_RATE_RX_ON_TRUE, TRUE);
    //printf("[-] nwk_SetCurrentPollRateType\n");

    OsalPortTimers_registerCleanupEvent(stackServiceId, OSALPORT_CLEAN_UP_TIMERS_EVT);

    printf("\n[ZSTACK] INIT DONE\n\n");
}

void *z_ZdoJoinCnfCB(void *pStr)
{
    printf("[TEST] _ZdoJoinCnfCB\n\n");
    /* Join Complete. De-register the callback with ZDO */
    ZDO_DeregisterForZdoCB(ZDO_JOIN_CNF_CBID);
    return NULL;
}

void z_test_join(void)
{
    // TEST THE RADIO //
    printf("[TEST] z_test_join\n\n");
    ZStatus_t s = ZDApp_JoinReq(12, 0x1A62, NULL, 0, 0, 1);
    ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, &z_ZdoJoinCnfCB);
}

void z_test_beacon(void)
{
    // TEST THE RADIO //
#if 1
    ZStatus_t s = ZDApp_NetworkDiscoveryReq(0x00001000, 1); // 0x07FFF800
#else
    zstack_devNwkDiscReq_t r;
    r.scanChannels = 0x00001000;
    r.scanDuration = 1;
    Zstackapi_DevNwkDiscReq(0, &r);
#endif
}

void z_process(bool blocked)
{
    if (blocked)
    {
        SemaphoreP_pend(semHandle, BIOS_WAIT_FOREVER);
    }
    else
    {
        if (0 != SemaphoreP_pend(semHandle, 1))
            return;
    }

    uint8_t i = 0;
    do // copy the priority-based round-robin scheduler from OSAL
    {
        if (*(pTasksEvents[i])) // Task is highest priority that is ready.
            break;
    } while (++i < zstackTasksCnt);

    if (i < zstackTasksCnt)
    {
        uint32_t events, key = OsalPort_enterCS();
        events = *(pTasksEvents[i]);
        *(pTasksEvents[i]) = 0; // Clear the Events for this task.
        OsalPort_leaveCS(key);

        //printf("[Z] ID:%d EV:%04X\n", (int)i, (int)events);
        events = (zstackTasksArr[i])(0U, events);

        key = OsalPort_enterCS();
        *(pTasksEvents[i]) |= events; // Add back unprocessed events to the current task.
        OsalPort_leaveCS(key);
    }
}
