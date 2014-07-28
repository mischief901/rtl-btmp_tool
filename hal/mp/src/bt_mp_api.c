#define LOG_TAG "btif_mp_api"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <termios.h>
#include <time.h>
#include <utils/Log.h>

#include "bluetoothmp.h"
#include "bt_mp_transport.h"
#include "bt_mp_api.h"
#include "bt_mp_base.h"
#include "bt_mp_build.h"
#include "bt_user_func.h"
#include "bt_mp_device_general.h"
#include "bt_mp_device_base.h"
#include "bt_mp_device_skip.h"
#include "bt_mp_device_rtl8723a.h"
#include "bt_mp_module_base.h"
#include "foundation.h"

#include "btif_api.h"

#include "gki.h"
#include "btu.h"

#include "hcidefs.h"
#include "hcimsgs.h"
#include "btif_common.h"


#define DEFAULT_HIT_ADDRESS 0x0000009e8b33

#define DEFAULT_CH_NUM                      10
#define DEFAULT_PKT_TYPE                    BT_PKT_3DH5
#define DEFAULT_PAYLOAD_TYPE                BT_PAYLOAD_TYPE_PRBS9
#define DEFAULT_PKT_COUNT                   0
#define DEFAULT_TX_GAIN_VALUE               0xA9
#define DEFAULT_WHITE_COEFF_VALUE           0

#define DEFAULT_TX_GAIN_INDEX               0xFF
#define DEFAULT_TEST_MODE                   BT_PSEUDO_MODE
#define DEFAULT_TX_DAC                      0x13
#define DEFAULT_PKTHEADER                   0x1234
#define DEFAULT_HOPPING_CH_NUM              0
#define DEFAULT_MULTI_RX_ENABLE             0

#define BT_PARAM_IDX0    0   //mPGRawData
#define BT_PARAM_IDX1    1   //mChannelNumber
#define BT_PARAM_IDX2    2   //mPacketType
#define BT_PARAM_IDX3    3   //mPayloadType
#define BT_PARAM_IDX4    4   //mTxPacketCount
#define BT_PARAM_IDX5    5   //mTxGainValue
#define BT_PARAM_IDX6    6   //mWhiteningCoeffValue
#define BT_PARAM_IDX7    7   //mTxGainIndex
#define BT_PARAM_IDX8    8   //mTxDAC
#define BT_PARAM_IDX9    9   //mPacketHeader
#define BT_PARAM_IDX10   10  //mHoppingFixChannel
#define BT_PARAM_IDX11   11  //mHitTarget
#define BT_PARAM_IDX12   12  //TXGainTable
#define BT_PARAM_IDX13   13  //TXDACTable
#define BT_PARAM_IDX14   14  //Xtal
#define BT_PARAM_IDX_NUM 15

typedef struct _EVENT_STRING {
    char EventData[3];//,XX
} EVENT_STRING;

static void bt_index2param(BT_MODULE *pBtModule, int index, int64_t value)
{
    switch (index) {
    case BT_PARAM_IDX0:
        pBtModule->pBtParam->mPGRawData[0] = (uint8_t)value;
        break;
    case BT_PARAM_IDX1:
        pBtModule->pBtParam->mChannelNumber = (uint8_t)value;
        break;
    case BT_PARAM_IDX2:
        pBtModule->pBtParam->mPacketType = (BT_PKT_TYPE)value;
        break;
    case BT_PARAM_IDX3:
        pBtModule->pBtParam->mPayloadType = (BT_PAYLOAD_TYPE)value;
        break;
    case BT_PARAM_IDX4:
        pBtModule->pBtParam->mTxPacketCount = (uint16_t)value;
        break;
    case BT_PARAM_IDX5:
        pBtModule->pBtParam->mTxGainValue = (uint8_t)value;
        break;
    case BT_PARAM_IDX6:
        pBtModule->pBtParam->mWhiteningCoeffValue = (uint8_t)value;
        break;
    case BT_PARAM_IDX7:
        pBtModule->pBtParam->mTxGainIndex = (uint8_t)value;
        break;
    case BT_PARAM_IDX8:
        pBtModule->pBtParam->mTxDAC = (uint8_t)value;
        break;
    case BT_PARAM_IDX9:
        pBtModule->pBtParam->mPacketHeader = (uint16_t)value;
        break;
    case BT_PARAM_IDX10:
        pBtModule->pBtParam->mHoppingFixChannel = (uint8_t)value;
        break;
    case BT_PARAM_IDX11:
        pBtModule->pBtParam->mHitTarget = (uint64_t)value;
        break;
    case BT_PARAM_IDX12:
        pBtModule->pBtParam->TXGainTable[0] = (uint8_t)value;
        break;
    case BT_PARAM_IDX13:
        pBtModule->pBtParam->TXDACTable[0] = (uint8_t)value;
        break;
    case BT_PARAM_IDX14:
        pBtModule->pBtParam->Rtl8761Xtal = (uint32_t)value;
        break;
    default:
        break;
    }
}

static void bt_index2print(BT_MODULE *pBtModule, int index, char *buf_cb)
{
    char pair_str[6];
    uint8_t i, len;

    switch (index) {
    case BT_PARAM_IDX0:
        len = pBtModule->pBtParam->mPGRawData[1];

        sprintf(buf_cb, "%s%s%d%s0x%02x%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPGRawData[0],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPGRawData[1]);

        for (i = 0; i < len; i++) {
            sprintf(pair_str, "%s0x%02x", STR_BT_MP_RESULT_DELIM, pBtModule->pBtParam->mPGRawData[2+i]);
            strcat(buf_cb, pair_str);
        }
        break;
    case BT_PARAM_IDX1:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mChannelNumber);
        break;
    case BT_PARAM_IDX2:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketType);
        break;
    case BT_PARAM_IDX3:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPayloadType);
        break;
    case BT_PARAM_IDX4:
        sprintf(buf_cb, "%s%s%d%s0x%04x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxPacketCount);
        break;
    case BT_PARAM_IDX5:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainValue);
        break;
    case BT_PARAM_IDX6:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mWhiteningCoeffValue);
        break;
    case BT_PARAM_IDX7:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainIndex);
        break;
    case BT_PARAM_IDX8:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxDAC);
        break;
    case BT_PARAM_IDX9:
        sprintf(buf_cb, "%s%s%d%s0x%04x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketHeader);
        break;
    case BT_PARAM_IDX10:
        sprintf(buf_cb, "%s%s%d%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHoppingFixChannel);
        break;
    case BT_PARAM_IDX11:
        sprintf(buf_cb, "%s%s%d%s0x%012llx",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHitTarget);
        break;
    case BT_PARAM_IDX12:
        sprintf(buf_cb, "%s%s%d%s0x%02x%s0x%02x%s0x%02x%s0x%02x%s0x%02x%s0x%02x%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[0],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[1],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[2],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[3],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[4],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[5],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXGainTable[6]);
        break;
    case BT_PARAM_IDX13:
        sprintf(buf_cb, "%s%s%d%s0x%02x%s0x%02x%s0x%02x%s0x%02x%s0x%02x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXDACTable[0],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXDACTable[1],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXDACTable[2],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXDACTable[3],
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->TXDACTable[4]);
        break;
    case BT_PARAM_IDX14:
        sprintf(buf_cb, "%s%s%d%s0x%08x",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                index,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->Rtl8761Xtal);
        break;
    default:
        break;
    }
}

int BT_SendHciCmd(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    int rtn = BT_FUNCTION_SUCCESS;
    char *token = NULL;
    uint16_t OpCode = 0;
    uint8_t ParamLen = 0;
    uint8_t ParamLen_1 = 0;
    uint8_t ParamArray[255];

    uint8_t params_count = 0;

    uint8_t pEvent[255] = {0};
    EVENT_STRING pEventString[255];
    uint32_t EventLen = 0;
    uint8_t i = 0;

    ALOGI("++%s: %s", STR_BT_MP_HCI_CMD, p);

    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        OpCode = strtol(token, NULL, 0);
        params_count++;
    } else {
        rtn = FUNCTION_PARAMETER_ERROR;
        goto EXIT;
    }

    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ParamLen = strtol(token, NULL, 0);
        params_count++;
    } else {
        rtn = FUNCTION_PARAMETER_ERROR;
        goto EXIT;
    }

    ParamLen_1 = ParamLen;
    while (ParamLen_1--) {
        token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
        if (token != NULL) {
            ParamArray[i++] = strtol(token, NULL, 0);
            params_count++;
        } else {
            rtn = FUNCTION_PARAMETER_ERROR;
            goto EXIT;
        }
    }

    if (params_count != ParamLen + 2) {
        rtn = FUNCTION_PARAMETER_ERROR;
        goto EXIT;
    }

    rtn = pBtModule->SendHciCommandWithEvent(pBtModule, OpCode, ParamLen, ParamArray, 0x0E, pEvent, &EventLen);

    ALOGI("%s%s%x", STR_BT_MP_HCI_CMD, STR_BT_MP_RESULT_DELIM, rtn);

    if (rtn == BT_FUNCTION_SUCCESS) {
        sprintf(pNotifyBuffer, "%s", STR_BT_MP_HCI_CMD);
        for (i = 0; i < EventLen; i++) {
            sprintf(pEventString[i].EventData, "%s%x", STR_BT_MP_RESULT_DELIM, pEvent[i]);
            strcat(pNotifyBuffer, pEventString[i].EventData);
        }
    } else {
        goto EXIT;
    }

    ALOGI("--%s", STR_BT_MP_HCI_CMD);
    return rtn;

EXIT:
    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_HCI_CMD, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
    return rtn;
}

int BT_GetParam(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    char *token = NULL;
    char *endptr = NULL;
    int index = -1;
    int rtn = BT_FUNCTION_SUCCESS;


    ALOGI("++%s: index %s", STR_BT_MP_GET_PARAM, p);

    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        index = strtol(token, &endptr, 0);
        if (*endptr || index < 0 || index >= BT_PARAM_IDX_NUM) {
            ALOGI("%s%s%d%s0x%02x",
                    STR_BT_MP_GET_PARAM,
                    STR_BT_MP_RESULT_DELIM,
                    BT_PARAM_IDX_NUM,
                    STR_BT_MP_RESULT_DELIM,
                    FUNCTION_PARAMETER_ERROR);

            sprintf(pNotifyBuffer, "%s%s%d%s0x%02x",
                    STR_BT_MP_GET_PARAM,
                    STR_BT_MP_RESULT_DELIM,
                    BT_PARAM_IDX_NUM,
                    STR_BT_MP_RESULT_DELIM,
                    FUNCTION_PARAMETER_ERROR);

            return FUNCTION_PARAMETER_ERROR;
        }

        bt_index2print(pBtModule, index, pNotifyBuffer);
    } else {
        /* print all exposed params if not specified */
        ALOGI("%s%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%012llx",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mChannelNumber,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketType,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPayloadType,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxPacketCount,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainValue,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mWhiteningCoeffValue,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainIndex,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxDAC,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketHeader,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHoppingFixChannel,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHitTarget);

        sprintf(pNotifyBuffer, "%s%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%x%s%012llx",
                STR_BT_MP_GET_PARAM,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mChannelNumber,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketType,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPayloadType,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxPacketCount,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainValue,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mWhiteningCoeffValue,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxGainIndex,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mTxDAC,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mPacketHeader,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHoppingFixChannel,
                STR_BT_MP_RESULT_DELIM,
                pBtModule->pBtParam->mHitTarget);

    }

    ALOGI("--%s", STR_BT_MP_GET_PARAM);
    return rtn;
}

int BT_SetParam(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    uint16_t pairs_count, params_count;
    char *pair_token, *param_token;
    char *pairs_buf, *params_buf;
    char *save_pairs, *save_params;
    int index = -1;
    int var_pair;
    int64_t value = 0;

    ALOGI("++%s: %s", STR_BT_MP_SET_PARAM, p);

    for (pairs_count = 0, pairs_buf = p; ; pairs_count++, pairs_buf = NULL) {
        pair_token = strtok_r(pairs_buf, STR_BT_MP_PAIR_DELIM, &save_pairs);
        if (pair_token == NULL)
            break;

        var_pair = 0; // set 1 if variable pair

        for (params_count = 0, params_buf = pair_token; ; params_count++, params_buf = NULL) {
            param_token = strtok_r(params_buf, STR_BT_MP_PARAM_DELIM, &save_params);
            if (param_token && params_count == 0) {
                index = strtol(param_token, NULL, 0);
                if (index < 0 || index >= BT_PARAM_IDX_NUM) {
                    ALOGI("Invalid BT param index %d", index);
                    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
                    return FUNCTION_PARAMETER_ERROR;
                } else if (index == 0 || index == 12 || index == 13) {
                    var_pair = 1;
                }
            } else if (param_token && params_count == 1) {
                value = strtoll(param_token, NULL, 0);
            } else if (param_token && params_count > 1 && var_pair == 1) {
                if (index == 0)
                    pBtModule->pBtParam->mPGRawData[params_count - 1] = (uint8_t)strtoll(param_token, NULL, 0);
                else if (index == 12 && params_count <= MAX_TXGAIN_TABLE_SIZE)
                    pBtModule->pBtParam->TXGainTable[params_count - 1] = (uint8_t)strtoll(param_token, NULL, 0);
                else if (index == 13 && params_count <= MAX_TXDAC_TABLE_SIZE)
                    pBtModule->pBtParam->TXDACTable[params_count - 1] = (uint8_t)strtoll(param_token, NULL, 0);
            } else if (param_token == NULL) // null token OR token parsing completed
                break;
        }

        if (params_count > 2 && var_pair == 1) {
            bt_index2param(pBtModule, index, value);
            uint16_t i = 0;
            for (i = 0; i < params_count - 1; i++) {
                if (index == 0) // variable pair format<index, cmd, len, data...>
                    ALOGI("PG raw data[%d]: 0x%02x", i, pBtModule->pBtParam->mPGRawData[i]);
                else if (index == 12)
                    ALOGI("TX gain table[%d]: 0x%02x", i, pBtModule->pBtParam->TXGainTable[i]);
                else if (index == 13)
                    ALOGI("TX dac table[%d]: 0x%02x", i, pBtModule->pBtParam->TXDACTable[i]);
            }
        } else if (params_count == 2 && var_pair == 0) { // 2-param pair format<index, value>
            bt_index2param(pBtModule, index, value);
            ALOGI("Pair index %d, pair value 0x%llx", index, value);
        } else if (params_count == 0) { // null pair
            continue;
        } else { // wrong pair format
            ALOGI("Invalid BT pair format, params count %d", params_count);
            sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
            return FUNCTION_PARAMETER_ERROR;
        }
    }

    ALOGI("--%s: pairs count %d", STR_BT_MP_SET_PARAM, pairs_count);

    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM, STR_BT_MP_RESULT_DELIM, BT_FUNCTION_SUCCESS);

    return BT_FUNCTION_SUCCESS;
}

int BT_SetParam1(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    char *token = NULL;
    const uint8_t BT_PARAM1_COUNT = 6;
    uint8_t params_count = 0;
    int ret = BT_FUNCTION_SUCCESS;

    ALOGI("++%s: %s", STR_BT_MP_SET_PARAM1, p);

    // uint8_t mChannelNumber;
    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mChannelNumber = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // BT_PKT_TYPE mPacketType;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mPacketType = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // BT_PAYLOAD_TYPE mPayloadType;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mPayloadType = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint16_t mTxPacketCount;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mTxPacketCount = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t mTxGainValue;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mTxGainValue = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t mWhiteningCoeffValue;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mWhiteningCoeffValue = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    //end of parameter
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ALOGI("BT_SetParam1: redundant token[%s]", token);
        params_count++;
    }

EXIT:
    ALOGI("%s: params_count = %d", STR_BT_MP_SET_PARAM1, params_count);

    if (params_count != BT_PARAM1_COUNT) {
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM1, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
        ret = FUNCTION_PARAMETER_ERROR;
    } else {
        ALOGI("mChannelNumber:0x%02x, mPacketType:0x%02x, mPayloadType:0x%02x, "
              "mTxPacketCount:0x%04x, mTxGainValue:0x%x, mWhiteningCoeffValue:0x%02x",
              pBtModule->pBtParam->mChannelNumber,
              pBtModule->pBtParam->mPacketType,
              pBtModule->pBtParam->mPayloadType,
              pBtModule->pBtParam->mTxPacketCount,
              pBtModule->pBtParam->mTxGainValue,
              pBtModule->pBtParam->mWhiteningCoeffValue
             );

        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM1, STR_BT_MP_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }

    ALOGI("--%s", STR_BT_MP_SET_PARAM1);
    return ret;
}

int BT_SetParam2(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    char *token = NULL;
    const uint8_t BT_PARAM2_COUNT = 5;
    uint8_t params_count = 0;
    int ret = BT_FUNCTION_SUCCESS;

    ALOGI("++%s: %s", STR_BT_MP_SET_PARAM2, p);

    // uint8_t mTxGainIndex;
    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mTxGainIndex = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t mTxDAC;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mTxDAC = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint16_t mPacketHeader;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mPacketHeader = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t mHoppingFixChannel
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mHoppingFixChannel = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint64_t(6 Bytes) mHitTarget
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        pBtModule->pBtParam->mHitTarget = strtoull(token, NULL, 16);
        params_count++;
    } else {
        goto EXIT;
    }

    // end of parameter
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ALOGI("BT_SetParam2: redundant token[%s]", token);
        params_count++;
    }

EXIT:
    ALOGI("%s: params_count = %d", STR_BT_MP_SET_PARAM2, params_count);

    if (params_count != BT_PARAM2_COUNT) {
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM2, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
        ret = FUNCTION_PARAMETER_ERROR;
    } else {
        ALOGI("mTxGainIndex:0x%02x, mTxDAC:0x%02x, mPacketHeader:0x%04x, "
              "mHoppingFixChannel:0x%02x, mHitTarget 0x%012llx",
                pBtModule->pBtParam->mTxGainIndex,
                pBtModule->pBtParam->mTxDAC,
                pBtModule->pBtParam->mPacketHeader,
                pBtModule->pBtParam->mHoppingFixChannel,
                pBtModule->pBtParam->mHitTarget
             );

        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_PARAM2, STR_BT_MP_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }

    ALOGI("--%s", STR_BT_MP_SET_PARAM2);

    return ret;
}

int BT_SetConfig(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    uint16_t pairs_count, params_count;
    char *pair_token, *param_token;
    char *pairs_buf, *params_buf;
    char *save_pairs, *save_params;
    int8_t mode = 0;
    char config_path[128];
    char buffer[128];
    int fd = -1;

    ALOGI("++%s: %s", STR_BT_MP_SET_CONFIG, p);

    for (pairs_count = 0, pairs_buf = p; ; pairs_count++, pairs_buf = NULL) {
        pair_token = strtok_r(pairs_buf, STR_BT_MP_PAIR_DELIM, &save_pairs);
        if (pair_token == NULL)
            break;

        // First pair must be <config_path, mode>
        if (pairs_count == 0) {
            for (params_count = 0, params_buf = pair_token; ; params_count++, params_buf = NULL) {
                param_token = strtok_r(params_buf, STR_BT_MP_PARAM_DELIM, &save_params);
                if (param_token && params_count == 0) {
                    strcpy(config_path, param_token);
                } else if (param_token && params_count == 1) {
                    mode = (int8_t)strtol(param_token, NULL, 0);
                    if (mode < 0 || mode > 3) {
                        ALOGI("Invalid file mode %d", mode);
                        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
                        return FUNCTION_PARAMETER_ERROR;
                    }
                } else if (param_token && params_count > 1) {
                    ALOGI("Invalid config pair format<%s>", pair_token);
                    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
                    return FUNCTION_PARAMETER_ERROR;
                } else if (param_token == NULL) // null token OR token parsing completed
                    break;
            }

            if (params_count == 2) {
                fd = open(config_path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                if (fd < 0) {
                    ALOGI("Failed to open config file: %s", strerror(errno));
                    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, FUNCTION_ERROR);
                    return FUNCTION_ERROR;
                }
            } else {
                ALOGI("Invalid config pair format<%s>", pair_token);
                sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
                return FUNCTION_PARAMETER_ERROR;
            }
        } else {
            for (params_count = 0, params_buf = pair_token; ; params_count++, params_buf = NULL) {
                param_token = strtok_r(params_buf, STR_BT_MP_PARAM_DELIM, &save_params);
                if (param_token) {
                    if (mode == 0) {
                        strcpy(buffer, param_token);
                        ALOGI("Write BT MAC address %s", buffer);
                    } else {
                        buffer[params_count] = strtol(param_token, NULL, 0);
                    }
                } else if (param_token == NULL) // null token OR token parsing completed
                    break;
            }

            ssize_t count;
            if (mode == 0)
                count = write(fd, buffer, 17);
            else
                count = write(fd, buffer, params_count);
            if (count < 0) {
                ALOGI("Failed to write config file<%s>", strerror(errno));
                sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, FUNCTION_ERROR);
                close(fd);
                return FUNCTION_ERROR;
            }
        }
    }

    close(fd);

    ALOGI("--%s: pairs count %d", STR_BT_MP_SET_CONFIG, pairs_count);

    sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_SET_CONFIG, STR_BT_MP_RESULT_DELIM, BT_FUNCTION_SUCCESS);

    return BT_FUNCTION_SUCCESS;
}

int BT_Exec(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    char *token = NULL;
    int ParameterIndex = 0;
    BT_PARAMETER *pBtParam = NULL;
    int rtn = BT_FUNCTION_SUCCESS;

    // index
    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ParameterIndex = strtol(token, NULL, 0);
    } else {
        rtn = FUNCTION_PARAMETER_ERROR;
        goto EXIT;
    }

    // end of parameter
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ALOGI("BT_Exec: redundant token[%s]", token);
        rtn = FUNCTION_PARAMETER_ERROR;
        goto EXIT;
    }

    ALOGI("BT_Exec: param index[%d]", ParameterIndex);

    if (ParameterIndex > NOTTHING &&
        ParameterIndex < NUMBEROFBT_ACTIONCONTROL_TAG) {

        pBtParam = pBtModule->pBtParam;
        pBtParam->ParameterIndex = ParameterIndex;
        rtn = pBtModule->ActionControlExcute(pBtModule);

        sprintf(pNotifyBuffer, "%s%s%x%s%x",
                STR_BT_MP_EXEC,
                STR_BT_MP_RESULT_DELIM,
                ParameterIndex,
                STR_BT_MP_RESULT_DELIM,
                rtn
                );
         ALOGI("%s%s%x%s%x",
                STR_BT_MP_EXEC,
                STR_BT_MP_RESULT_DELIM,
                ParameterIndex,
                STR_BT_MP_RESULT_DELIM,
                rtn
                );
    } else {
        sprintf(pNotifyBuffer, "%s%s%s%s%x",
                STR_BT_MP_EXEC,
                STR_BT_MP_RESULT_DELIM,
                p,
                STR_BT_MP_RESULT_DELIM,
                FUNCTION_PARAMETER_ERROR
                );
        ALOGI("%s%s%s%s%x",
                STR_BT_MP_EXEC,
                STR_BT_MP_RESULT_DELIM,
                p,
                STR_BT_MP_RESULT_DELIM,
                FUNCTION_PARAMETER_ERROR
                );
    }

EXIT:
    return rtn;
}

int BT_ReportTx(BT_MODULE *pBtModule, char *pNotifyBuffer)
{
    BT_DEVICE_REPORT BtDeviceReport;
    int rtn = BT_FUNCTION_SUCCESS;

    ALOGI("++%s", STR_BT_MP_REPORT_TX);

    rtn = pBtModule->ActionReport(pBtModule, REPORT_PKT_TX, &BtDeviceReport);

    if (rtn != BT_FUNCTION_SUCCESS) {
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_REPORT_TX, STR_BT_MP_RESULT_DELIM, rtn);
        ALOGI("%s%s%x", STR_BT_MP_REPORT_TX, STR_BT_MP_RESULT_DELIM, rtn);
        goto exit;
    } else {
        ALOGI("%s%s%x%s%x",
                STR_BT_MP_REPORT_TX,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalTXBits,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalTxCounts);

        sprintf(pNotifyBuffer, "%s%s%x%s%x",
                                STR_BT_MP_REPORT_TX,
                                STR_BT_MP_RESULT_DELIM,
                                BtDeviceReport.TotalTXBits,
                                STR_BT_MP_RESULT_DELIM,
                                BtDeviceReport.TotalTxCounts);
    }

exit:
    ALOGI("--%s", STR_BT_MP_REPORT_TX);

    return rtn;
}

int BT_ReportContTx(BT_MODULE *pBtModule, char *pNotifyBuffer)
{
    BT_DEVICE_REPORT BtDeviceReport;
    int rtn = BT_FUNCTION_SUCCESS;

    ALOGI("++%s", STR_BT_MP_REPORT_CONT_TX);

    rtn=pBtModule->ActionReport(pBtModule, REPORT_CONT_TX, &BtDeviceReport);

    if (rtn != BT_FUNCTION_SUCCESS) {
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_REPORT_CONT_TX, STR_BT_MP_RESULT_DELIM, rtn);
        ALOGI("%s%s%x", STR_BT_MP_REPORT_CONT_TX, STR_BT_MP_RESULT_DELIM, rtn);
        goto exit;
    } else {
        ALOGI("%s%s%x%s%x",
                STR_BT_MP_REPORT_CONT_TX,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalTXBits,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalTxCounts);

        sprintf(pNotifyBuffer, "%s%s%x%s%x",
                                STR_BT_MP_REPORT_CONT_TX,
                                STR_BT_MP_RESULT_DELIM,
                                BtDeviceReport.TotalTXBits,
                                STR_BT_MP_RESULT_DELIM,
                                BtDeviceReport.TotalTxCounts);
    }

exit:
    ALOGI("--%s", STR_BT_MP_REPORT_CONT_TX);

    return rtn;
}

int BT_ReportRx(BT_MODULE  *pBtModule, char* pNotifyBuffer)
{
    BT_DEVICE_REPORT BtDeviceReport;
    int rtn = BT_FUNCTION_SUCCESS;

    ALOGI("++%s", STR_BT_MP_REPORT_RX);

    rtn = pBtModule->ActionReport(pBtModule, REPORT_RX, &BtDeviceReport);
    if (rtn != BT_FUNCTION_SUCCESS) {
        ALOGI("%s%s%x", STR_BT_MP_PKTRXSTART, STR_BT_MP_RESULT_DELIM, rtn);
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_PKTRXSTART, STR_BT_MP_RESULT_DELIM, rtn);
        goto exit;
    } else {
        ALOGI("%s%s%x%s%x%s%x%s%x",
                STR_BT_MP_REPORT_RX,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.RxRssi,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRXBits,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRxCounts,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRxErrorBits
             );

        sprintf(pNotifyBuffer, "%s%s%x%s%x%s%x%s%x",
                STR_BT_MP_REPORT_RX,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.RxRssi,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRXBits,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRxCounts,
                STR_BT_MP_RESULT_DELIM,
                BtDeviceReport.TotalRxErrorBits
               );
    }

exit:
    ALOGI("--%s", STR_BT_MP_REPORT_RX);

    return rtn;
}

int BT_RegRW(BT_MODULE *pBtModule, char *p, char *pNotifyBuffer)
{
    char *token = NULL;
    uint8_t REGRW_PARAM_COUNT = 5;// 5:read, 6:write; +1 if BB reg
    uint8_t params_count = 0;
    uint8_t type = 0;
    uint8_t rw = 0;
    uint8_t page = 0;
    uint8_t address = 0;
    uint8_t msb = 0;
    uint8_t lsb = 0;
    uint32_t data = 0;
    int rtn = BT_FUNCTION_SUCCESS;

    ALOGI("++%s: %s", STR_BT_MP_REG_RW, p);

    // uint8_t type;
    token = strtok(p, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        type = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t rw;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        rw = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // BB Reg has an extra field<PAGE>
    if (type == BB_REG) {
        // uint8_t page;
        token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
        if (token != NULL) {
            page = strtol(token, NULL, 0);
            params_count++;
        } else {
            goto EXIT;
        }
    }

    // uint16_t address
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        address = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t msb;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        msb = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    // uint8_t lsb;
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        lsb = strtol(token, NULL, 0);
        params_count++;
    } else {
        goto EXIT;
    }

    if (rw == 1) { //write
        // uint32_t dataToWrite;
        token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
        if (token != NULL) {
            data = strtol(token, NULL, 0);
            params_count++;
        } else {
            goto EXIT;
        }

    }

    // end of parameters
    token = strtok(NULL, STR_BT_MP_PARAM_DELIM);
    if (token != NULL) {
        ALOGI("BT_RegRW: redundant token[%s]", token);
        params_count++;
    } else {
        goto EXIT;
    }

EXIT:
    ALOGI("%s: params_count = %d", STR_BT_MP_REG_RW, params_count);

    if (rw == 1) REGRW_PARAM_COUNT++;
    if (type == BB_REG) REGRW_PARAM_COUNT++;

    if (params_count != REGRW_PARAM_COUNT) {
        sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_REG_RW, STR_BT_MP_RESULT_DELIM, FUNCTION_PARAMETER_ERROR);
    } else {
        ALOGI("BT_RegRW: type 0x%x, rw 0x%x, page 0x%x, address 0x%04x, msb 0x%x, lsb 0x%x, data 0x%08x",
                type, rw, page, address, msb, lsb, data);

        if (rw == 0) {
            rtn = pBtModule->GetRegMaskBits(pBtModule, type, page, address, msb, lsb, &data);
            sprintf(pNotifyBuffer, "%s%s%x%s%08x", STR_BT_MP_REG_RW, STR_BT_MP_RESULT_DELIM, rtn,
                    STR_BT_MP_RESULT_DELIM, data);
        } else {
            rtn = pBtModule->SetRegMaskBits(pBtModule, type, page, address, msb, lsb, data);
            sprintf(pNotifyBuffer, "%s%s%x", STR_BT_MP_REG_RW, STR_BT_MP_RESULT_DELIM, rtn);
        }

    }

    ALOGI("--%s", STR_BT_MP_REG_RW);
    return rtn;
}

void bt_mp_module_init(BASE_INTERFACE_MODULE *pBaseInterfaceModule, BT_MODULE *pBtModule)
{
    uint8_t pTxGainTable[7] = {0x49,0x4d,0x69,0x89,0x8d,0xa9,0xa9}; // RTL8761 Table
    uint8_t pTxDACTable[5] = {0x10,0x11,0x12,0x13,0x14};

    ALOGI("bt_mp_module_init, pBaseInterfaceModule %p, pBtModule %p", pBaseInterfaceModule, pBtModule);

    BuildTransportInterface(
            pBaseInterfaceModule,
            1,
            115200,
            NULL,//open
            bt_transport_SendHciCmd,
            bt_transport_RecvHciEvt,
            NULL,//close
            UserDefinedWaitMs
            );

    BuildBluetoothModule(
            pBaseInterfaceModule,
            pBtModule,
            NULL,
            pTxGainTable,
            pTxDACTable
            );

    pBtModule->pBtParam->mPGRawData[0] = 0;
    pBtModule->pBtParam->mChannelNumber = DEFAULT_CH_NUM;
    pBtModule->pBtParam->mPacketType = DEFAULT_PKT_TYPE;
    pBtModule->pBtParam->mPayloadType = DEFAULT_PAYLOAD_TYPE;
    pBtModule->pBtParam->mTxPacketCount = DEFAULT_PKT_COUNT;
    pBtModule->pBtParam->mTxGainValue = DEFAULT_TX_GAIN_VALUE;
    pBtModule->pBtParam->mWhiteningCoeffValue = DEFAULT_WHITE_COEFF_VALUE;
    pBtModule->pBtParam->mTxGainIndex = DEFAULT_TX_GAIN_INDEX;
    pBtModule->pBtParam->mTxDAC = DEFAULT_TX_DAC;
    pBtModule->pBtParam->mPacketHeader = DEFAULT_PKTHEADER;
    pBtModule->pBtParam->mHoppingFixChannel = DEFAULT_HOPPING_CH_NUM;
    pBtModule->pBtParam->mHitTarget = 0x0000009e8b33;
}
