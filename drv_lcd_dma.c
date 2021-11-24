#include <Arduino.h>
#include <drv_lcd.h>

#define H16(v) (((v)&0xFF00) >> 8)
#define L16(v) ((v)&0xFF)

TSCREEN LCDScreen;

pLAYER LCDIF_LAYER[LCDIF_NUMLAYERS] = {
    (volatile pLAYER) & LCDIF_LAYER0BASE,
    (volatile pLAYER) & LCDIF_LAYER1BASE,
    (volatile pLAYER) & LCDIF_LAYER2BASE,
    (volatile pLAYER) & LCDIF_LAYER3BASE,
};

const uint8_t CFormatToBPP[] = {1, 2, 0, 3, 4, 4, 4};

static inline bool LCDIF_isRunning(void)
{
    return (LCDIF_START & LCDIF_RUN) != 0;
}

static inline bool LCDIF_isBusy(void)
{
    uint16_t reg = LCDIF_INTSTA; // auto clear
    if (reg & LCDIF_CPL)         // frame transfer is completed
    {
        LCDIF_START = 0;
        return false;
    }
    return reg;
}

void LCDIF_Run(bool wait)
{
    LCDIF_START = 0;
    LCDIF_START = LCDIF_RUN;
    while (wait && LCDIF_isBusy())
        ;
}

void LCDIF_SetOutputWindow(pRECT rect)
{
    if (LCDIF_isRunning())
        return;
#define DATA_LEN 11
    uint32_t data[DATA_LEN];
    uint32_t *p = data;
    if (rect)
    {
        *p++ = LCDIF_COMM(CASET) | LCDIF_CMD;
        *p++ = LCDIF_COMM(H16(rect->l)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(L16(rect->l)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(H16(rect->r)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(L16(rect->r)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(RASET) | LCDIF_CMD;
        *p++ = LCDIF_COMM(H16(rect->t)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(L16(rect->t)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(H16(rect->b)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(L16(rect->b)) | LCDIF_DATA;
        *p++ = LCDIF_COMM(RAMWR) | LCDIF_CMD;
        LCDIF_WROIOFS = LCDIF_WROIOFX(rect->l) | LCDIF_WROIOFY(rect->t);
        LCDIF_WROISIZE = LCDIF_WROICOL(rect->r - rect->l + 1) | LCDIF_WROIROW(rect->b - rect->t + 1);
        p = data;
        for (int i = 0; i < DATA_LEN; i++)
            LCDIF_COMD(i) = *p++;
        LCDIF_WROICON &= ~LCDIF_COMMAND_MASK;
        LCDIF_WROICON |= LCDIF_COMMAND(DATA_LEN - 1) | LCDIF_ENC; // enable command transfer + data
    }
}

bool LCDIF_EnableLayer(unsigned char Layer, bool Enabled, bool Update)
{
    TRECT layerRect;

    if ((Layer >= LCDIF_NUMLAYERS) || !LCDScreen.VLayer[Layer].Initialized)
        return false;

    if (LCDScreen.VLayer[Layer].Enabled != Enabled)
    {
        if (Enabled)
            LCDIF_WROICON |= LCDScreen.VLayer[Layer].LayerEnMask;
        else
            LCDIF_WROICON &= ~LCDScreen.VLayer[Layer].LayerEnMask;

        LCDScreen.VLayer[Layer].Enabled = Enabled;

        if (Update)
        {
            //layerRect = LCDScreen.VLayer[Layer].LayerRgn;
            //layerRect.l += LCDScreen.VLayer[Layer].LayerOffset.x - LCDScreen.ScreenOffset.x;
            //layerRect.r += LCDScreen.VLayer[Layer].LayerOffset.x - LCDScreen.ScreenOffset.x;
            //layerRect.t += LCDScreen.VLayer[Layer].LayerOffset.y - LCDScreen.ScreenOffset.y;
            //layerRect.b += LCDScreen.VLayer[Layer].LayerOffset.y - LCDScreen.ScreenOffset.y;
            //if (GDI_ANDRectangles(&LayerRect, &LCDScreen.ScreenRgn)) LCDIF_UpdateRectangle(LayerRect);

            LCDIF_Run(true);
        }
    }
    return Enabled;
}

bool LCDIF_SetupLayer(unsigned char Layer, uint32_t X, uint32_t Y, uint32_t W, uint32_t H, TCFORMAT CFormat)
{
    if (Layer >= LCDIF_NUMLAYERS)
        return false;

    LCDScreen.VLayer[Layer].Enabled = false;
    LCDScreen.VLayer[Layer].Initialized = false;
    LCDIF_WROICON &= ~LCDScreen.VLayer[Layer].LayerEnMask;

    if (LCDScreen.VLayer[Layer].FrameBuffer)
    {
        free(LCDScreen.VLayer[Layer].FrameBuffer); // delete frame buffer
        LCDScreen.VLayer[Layer].FrameBuffer = NULL;
    }

    if (W && H && (CFormat < CF_NUM))
    {
        uint32_t n;

        LCDScreen.VLayer[Layer].LayerRgn = Rect(0, 0, W - 1, H - 1);
        LCDScreen.VLayer[Layer].LayerOffset = Point(X, Y);
        LCDScreen.VLayer[Layer].ColorFormat = CFormat;
        LCDScreen.VLayer[Layer].BPP = CFormatToBPP[CFormat];

        n = W * H * LCDScreen.VLayer[Layer].BPP;
        if (n)
        {
            LCDScreen.VLayer[Layer].FrameBuffer = malloc(n); // create new frame buffer     0x9024
            if (LCDScreen.VLayer[Layer].FrameBuffer)
            {
                LCDIF_LAYER[Layer]->LCDIF_LWINCON = LCDIF_LCF_RGB565 | LCDIF_LROTATE(LCDIF_LR_NO) | LCDIF_LCF(CFormat);
                LCDIF_LAYER[Layer]->LCDIF_LWINADD = (uint32_t)LCDScreen.VLayer[Layer].FrameBuffer; // window display start address register
                LCDIF_LAYER[Layer]->LCDIF_LWINOFFS = LCDIF_LWINOF_X(X) | LCDIF_LWINOF_Y(Y);        // window display offset register
                LCDIF_LAYER[Layer]->LCDIF_LWINSIZE = LCDIF_LCOLS(W) | LCDIF_LROWS(H);              // window size
                LCDIF_LAYER[Layer]->LCDIF_LWINSCRL = LCDIF_LSCCOL(0) | LCDIF_LSCROW(0);            // scroll start offset
                LCDIF_LAYER[Layer]->LCDIF_LWINMOFS = LCDIF_LMOFCOL(0) | LCDIF_LMOFROW(0);          // memory offset
                LCDIF_LAYER[Layer]->LCDIF_LWINPITCH = LCDScreen.VLayer[Layer].BPP * W;             // memory pitch

                LCDScreen.VLayer[Layer].Initialized = true;
            }
        }
    }
    return LCDScreen.VLayer[Layer].Initialized;
}

void LCDIF_WROI_INIT(void)
{
    memset(&LCDScreen, 0, sizeof(TSCREEN));
    LCDScreen.VLayer[0].LayerEnMask = LCDIF_L0EN;
    LCDScreen.VLayer[1].LayerEnMask = LCDIF_L1EN;
    LCDScreen.VLayer[2].LayerEnMask = LCDIF_L2EN;
    LCDScreen.VLayer[3].LayerEnMask = LCDIF_L3EN;

    LCDIF_WROICON = (LCDIF_F_RGB | LCDIF_F_RGB565 | LCDIF_F_PADDLSB | LCDIF_F_ITF_8B);
    LCDIF_WROICADD = LCDIF_CSIF0;
    LCDIF_WROIDADD = LCDIF_DSIF0;
    LCDIF_WROIOFS = LCDIF_WROIOFX(0) | LCDIF_WROIOFY(0);
    LCDIF_WROISIZE = LCDIF_WROICOL(LCD_X_RESOLUTION) | LCDIF_WROIROW(LCD_Y_RESOLUTION);
    LCDIF_WROI_BGCLR = 0;
    LCDIF_Run(true);
}

void LCD_draw_imageRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *data)
{
    if (LCDIF_SetupLayer(0, x1, y1, x2 - x1 + 1, y2 - y1 + 1, CF_8IDX))
    {
        uint16_t *dst = (uint16_t *)LCDScreen.VLayer[0].FrameBuffer;
        int size = (x2 - x1 + 1) * (y2 - y1 + 1);
        while (size--)
            *dst++ == *data++;
        LCDIF_EnableLayer(0, true, true);
    }
}

void LCD_draw_image(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *data)
{
    LCDIF_WROI_BGCLR = rand(); // only for test 
    LCD_draw_imageRect(x, y, x + w - 1, y + h - 1, data);
}