////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Georgi Angelov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#include "hal_spi.h"
#include "hal_pctl.h"
#include "hal_gpio.h"

#define SPI_MAX_PACKET_LENGTH (2048)                   // !!! firmware defined
static uint8_t *spi_tx_buffer = (uint8_t *)0x00009024; // !!! firmware address EXTSRAM
static uint8_t *spi_rx_buffer = (uint8_t *)0x00009824; // !!! firmware address EXTSRAM

static int spi_current_owner = 0;
static int spi_in_use(spi_context *ctx)
{
    if (NULL == ctx)
        return -1;
    if (ctx->id == spi_current_owner)
        return 0;
    return -1;
}

spi_context *SPI_Create(void)
{
    spi_context *ctx = (spi_context *)calloc(sizeof(spi_context), 1);
    static int owner = 0;
    // DEFAULT SETTINGS
    ctx->id = owner++;
    ctx->attr.setup_time = 5;
    ctx->attr.hold_time = 5;
    ctx->attr.clk_low = (1000000 / 1000) >> 5;  // 1 MHz
    ctx->attr.clk_high = (1000000 / 1000) >> 5; // 1 MHz
    ctx->attr.idle_time = 5;
    ctx->attr.enable_pause_int = 0;
    ctx->attr.enable_finish_int = 0;
    ctx->attr.enable_pause_mode = 0;
    ctx->attr.enable_deassert_mode = 0;
    ctx->attr.tx_mlsb = SPI_LSB;
    ctx->attr.rx_mlsb = SPI_LSB;
    ctx->attr.tx_mode = SPI_MODE_FIFO;
    ctx->attr.rx_mode = SPI_MODE_FIFO;
    ctx->attr.clk_polarity = SPI_CPOL_0;
    ctx->attr.clk_format = SPI_CPHA_0;
    return ctx;
}

int SPI_Close(spi_context *ctx)
{
    if (spi_in_use(ctx))
        return -1;
    //protect
    spi_current_owner = 0;
    //end
    PCTL_PowerDown(PD_SPI);
    return 0;
}

int SPI_Open(spi_context *ctx)
{
    if (spi_in_use(ctx))
        return -1;
    spi_current_owner = ctx->id;

    PCTL_PowerUp(PD_SPI);
    PCTL_PowerUp(PD_DMA);
    SPI_COMM.RESET = 1;

    GPIO_Setup(GPIO26, GPMODE(GPIO26_MODE_SPICS));   //
    GPIO_Setup(GPIO27, GPMODE(GPIO27_MODE_SPISCK));  //
    GPIO_Setup(GPIO28, GPMODE(GPIO28_MODE_SPIMOSI)); //
    GPIO_Setup(GPIO29, GPMODE(GPIO29_MODE_SPIMISO)); //

    return 0;
}

int SPI_Config(spi_context *ctx)
{
    if (spi_in_use(ctx))
        return -1;
    SPI_CONF0_REG = 0;
    SPI_CONF1_REG = 0;
    SPI_TX_ADDR_REG = 0;
    SPI_RX_ADDR_REG = 0;
    SPI_COMM_REG = 0;
    SPI_CONF0.TIME_HIGH = ctx->attr.clk_high; //
    SPI_CONF0.TIME_LOW = ctx->attr.clk_low;   //
    SPI_CONF0.TIME_HOLD = 5;                  // ctx->attr.hold_time;
    SPI_CONF0.TIME_SETUP = 5;                 // ctx->attr.setup_time;
    SPI_CONF1.CS_IDLE_COUNT = 0;              // ctx->attr.idle_time;
    SPI_COMM.RX_MSBF = ctx->attr.rx_mlsb;     //
    SPI_COMM.TX_MSBF = ctx->attr.tx_mlsb;     //
    SPI_COMM.CPOL = ctx->attr.clk_polarity;   //
    SPI_COMM.CPHA = ctx->attr.clk_format;     //
    SPI_COMM.PAUSE_EN = 0;                    // ctx->attr.enable_pause_mode;
    SPI_COMM.CS_DEASSERT_EN = 0;              // ctx->attr.enable_deassert_mode;
    SPI_COMM.DMA_EN_RX = 0;                   // ctx->attr.rx_mode;
    SPI_COMM.DMA_EN_TX = 0;                   // ctx->attr.tx_mode;
    SPI_COMM.PAUSE_IE = 0;                    // ctx->attr.enable_pause_int;
    SPI_COMM.FINISH_IE = 0;                   // ctx->attr.enable_finish_int;
    return 0;
}

int SPI_SetSpeed(spi_context *ctx, uint16_t kHz, bool update)
{
    if (spi_in_use(ctx))
        return -1;
    uint8_t t = 1000000 / kHz >> 5;
    ctx->attr.clk_low = ctx->attr.clk_high = t;
    if (update)
        SPI_CONF0.TIME_LOW = SPI_CONF0.TIME_HIGH = t;
    return 0;
}

static uint32_t spi_transfer(spi_context *ctx, uint8_t *wr_buf, uint8_t *rd_buf, unsigned int size)
{
    if (spi_in_use(ctx))
        return 0;
    if (size > SPI_MAX_PACKET_LENGTH)
        size = SPI_MAX_PACKET_LENGTH; // limit to 1024

#if 0 // clear fifo, not need   
    int volatile dumy;
    for (int i = 0; i < SPI_FIFO_SIZE / 4; i++)
    {
        dumy = SPI_RX_FIFO_REG;
        SPI_TX_FIFO_REG = 0;
    }
#endif

    memcpy(spi_tx_buffer, wr_buf, size);
    SPI_TX_ADDR_REG = (uint32_t)spi_tx_buffer;
    SPI_RX_ADDR_REG = (uint32_t)spi_rx_buffer;
    SPI_COMM.DMA_EN_TX = SPI_MODE_DMA;  // SPI_MODE_FIFO
    SPI_COMM.DMA_EN_RX = SPI_MODE_DMA;  // SPI_MODE_FIFO
    SPI_CONF1.PACKET_LENGTH = size - 1; // max 2048
    SPI_CONF1.PACKET_LOOP_CNT = 0;
    SPI_COMM.ACT = 1;

    while (SPI_STATUS1_REG & 1) // wait finish
        ;
    SPI_STATUS1_REG = 0; // clear finish

    memcpy(rd_buf, spi_rx_buffer, size); // return rx
    return size;
}

int SPI_Transfer(spi_context *ctx, uint8_t *wr_buf, uint8_t *rd_buf, uint16_t size)
{
    if (0 == size || NULL == wr_buf || NULL == rd_buf)
        return 0;
    int res, cnt = 0;
    while (size)
    {
        if (0 == (res = spi_transfer(ctx, wr_buf, rd_buf, size)))
            break;
        size -= res;
        cnt += res;
        wr_buf += res;
        rd_buf += res;
    }
    return cnt;
}

void spi_dump_reg(void)
{
    printf("  SPI_CONF0_REG                 = %08X \n", (int)SPI_CONF0_REG);
    printf("  SPI_CONF1_REG                 = %08X \n", (int)SPI_CONF1_REG);
    printf("  SPI_TX_ADDR_REG               = %08X \n", (int)SPI_TX_ADDR_REG);
    printf("  SPI_RX_ADDR_REG               = %08X \n", (int)SPI_RX_ADDR_REG);
    printf("  SPI_TX_FIFO_REG               = %08X \n", (int)SPI_TX_FIFO_REG);
    printf("  SPI_RX_FIFO_REG               = %08X \n", (int)SPI_RX_FIFO_REG);
    printf("  SPI_COMM_REG                  = %08X \n", (int)SPI_COMM_REG);
    printf("  SPI_STATUS1_REG               = %08X \n", (int)SPI_STATUS1_REG);
    printf("  SPI_STATUS2_REG               = %08X \n", (int)SPI_STATUS2_REG);
    printf("  SPI_GMC_SLOW_DOWN_REG         = %08X \n", (int)SPI_GMC_SLOW_DOWN_REG);
    printf("  SPI_ULTRA_HIGH_PRIORITY_REG   = %08X \n", (int)SPI_ULTRA_HIGH_PRIORITY_REG);
    printf("  SPI_PAD_MACRO_SELECT_REG      = %08X \n", (int)SPI_PAD_MACRO_SELECT_REG);
}

/* 
QUECTEL
  SPI_CONF0_REG                 = 05051F1F 
  SPI_CONF1_REG                 = 00010005 
  SPI_TX_ADDR_REG               = 00009026 
  SPI_RX_ADDR_REG               = 00009826 
  SPI_TX_FIFO_REG               = 00000000 
  SPI_RX_FIFO_REG               = 00000000 
  SPI_COMM_REG                  = 00003D00 
  SPI_STATUS1_REG               = 00000000 
  SPI_STATUS2_REG               = 00000001 
  SPI_GMC_SLOW_DOWN_REG         = 00000000 
  SPI_ULTRA_HIGH_PRIORITY_REG   = 00000000 
  SPI_PAD_MACRO_SELECT_REG      = 00000000 

THIS
  SPI_CONF0_REG                 = 05051F1F 
  SPI_CONF1_REG                 = 000F0000 
  SPI_TX_ADDR_REG               = 00009036 ??
  SPI_RX_ADDR_REG               = 00009836 ??
  SPI_TX_FIFO_REG               = 00000000 
  SPI_RX_FIFO_REG               = 00000000 
  SPI_COMM_REG                  = 00003D00 
  SPI_STATUS1_REG               = 00000001 
  SPI_STATUS2_REG               = 00000001 
  SPI_GMC_SLOW_DOWN_REG         = 00000000 
  SPI_ULTRA_HIGH_PRIORITY_REG   = 00000000 
  SPI_PAD_MACRO_SELECT_REG      = 00000000 

*/