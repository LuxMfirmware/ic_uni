/**
 ******************************************************************************
 * File Name          : LCDConf.c
 * Date               : 10.3.2018
 * Description        : TFT LCD Display driver for STemWin graphics
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32746g_sdram.h"
#include "GUI_Private.h"
#include "GUIDRV_Lin.h"
#include "LCDConf.h"
#include "GUI.h"
/*********************************************************************
*
*       Supported orientation modes (not to be changed)
*/
#define ROTATION_0   0
#define ROTATION_CW  1
#define ROTATION_180 2
#define ROTATION_CCW 3


/*********************************************************************
*
*       Supported color modes (not to be changed)
*/
//      Color mode               Conversion     Driver (default orientation)
//      ---------------------------------------------
#define COLOR_MODE_ARGB8888     0 // GUICC_M8888I - GUIDRV_LIN_32
#define COLOR_MODE_RGB888       1 // GUICC_M888   - GUIDRV_LIN_24
#define COLOR_MODE_RGB565       2 // GUICC_M565   - GUIDRV_LIN_16
#define COLOR_MODE_ARGB1555     3 // GUICC_M1555I - GUIDRV_LIN_16
#define COLOR_MODE_ARGB4444     4 // GUICC_M4444I - GUIDRV_LIN_16
#define COLOR_MODE_L8           5 // GUICC_8666   - GUIDRV_LIN_8
#define COLOR_MODE_AL44         6 // GUICC_1616I  - GUIDRV_LIN_8
#define COLOR_MODE_AL88         7 // GUICC_88666I - GUIDRV_LIN_16


/*********************************************************************
**********************************************************************
*
*       DISPLAY CONFIGURATION START (TO BE MODIFIED)
*
**********************************************************************
**********************************************************************
*/
/*********************************************************************
*
*       Common
*/
//
// Physical display size
//
#define XSIZE_PHYS 480
#define YSIZE_PHYS 272

//
// Buffers / VScreens
//
#define NUM_BUFFERS  3 // Number of multiple buffers to be used (at least 1 buffer)
#define NUM_VSCREENS 1 // Number of virtual  screens to be used (at least 1 screen)

//
// Redefine number of layers for this configuration file. Must be equal or less than in GUIConf.h!
//
#undef  GUI_NUM_LAYERS
#define GUI_NUM_LAYERS 2

//
// Touch screen
//
#ifdef RTE_Graphics_Touchscreen
#define USE_TOUCH   1
#else
#define USE_TOUCH   0
#endif
//
// Touch screen calibration
#define TOUCH_X_MIN 0x0000
#define TOUCH_X_MAX 0x01E0
#define TOUCH_Y_MIN 0x0000
#define TOUCH_Y_MAX 0x0110

//
// Video RAM Address
//
#define VRAM_ADDR 			0xC0000000
#define VRAM_BUFER_SIZE 	0x00200000	// 2MB for one layer frame buffers
//
// DMA2D Buffer Address
//
#define DMA2D_BUFFER_ADDR 	0x20000000

/*********************************************************************
*
*       Layer 0
*/
//
// Color mode layer 0. Should be one of the above defined color modes
//
#define COLOR_MODE_0 COLOR_MODE_RGB565
//#define COLOR_MODE_0 COLOR_MODE_ARGB8888
//
// Size of layer 0.
//
#define XSIZE_0 480
#define YSIZE_0 272

//
// Orientation of layer 0. Should be one of the above defined display orientations.
//
#define ORIENTATION_0 ROTATION_0

/*********************************************************************
*
*       Layer 1
*/
//
// Color mode layer 1. Should be one of the above defined color modes
//
#define COLOR_MODE_1 COLOR_MODE_ARGB8888
//#define COLOR_MODE_1 COLOR_MODE_RGB565
//
// Size of layer 1
//
#define XSIZE_1 480
#define YSIZE_1 272

//
// Orientation of layer 1. Should be one of the above defined display orientations.
//
#define ORIENTATION_1 ROTATION_0

/*********************************************************************
*
*       Background color shown where no layer is active
*/
#define BK_COLOR GUI_DARKBLUE


/*********************************************************************
*
*       Automatic selection of driver and color conversion
*/
#if	(COLOR_MODE_0 == COLOR_MODE_ARGB8888)
#define COLOR_CONVERSION_0 GUICC_M8888I
#elif (COLOR_MODE_0 == COLOR_MODE_RGB888)
#define COLOR_CONVERSION_0 GUICC_M888
#elif (COLOR_MODE_0 == COLOR_MODE_RGB565)
#define COLOR_CONVERSION_0 GUICC_M565
#elif (COLOR_MODE_0 == COLOR_MODE_ARGB1555)
#define COLOR_CONVERSION_0 GUICC_M1555I
#elif (COLOR_MODE_0 == COLOR_MODE_ARGB4444)
#define COLOR_CONVERSION_0 GUICC_M4444I
#elif (COLOR_MODE_0 == COLOR_MODE_L8)
#define COLOR_CONVERSION_0 GUICC_8666
#elif (COLOR_MODE_0 == COLOR_MODE_AL44)
#define COLOR_CONVERSION_0 GUICC_1616I
#elif (COLOR_MODE_0 == COLOR_MODE_AL88)
#define COLOR_CONVERSION_0 GUICC_88666I
#else
#error Illegal color mode 0!
#endif

#if	(COLOR_MODE_0 == COLOR_MODE_ARGB8888)
#define PIXEL_BYTES_0 4
#if   (ORIENTATION_0 == ROTATION_0)
#define DSP_DRIVER_0   GUIDRV_LIN_32
#elif (ORIENTATION_0 == ROTATION_CW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSX_32
#elif (ORIENTATION_0 == ROTATION_180)
#define DSP_DRIVER_0   GUIDRV_LIN_OXY_32
#elif (ORIENTATION_0 == ROTATION_CCW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSY_32
#endif
#elif (COLOR_MODE_0 == COLOR_MODE_RGB888)
#define PIXEL_BYTES_0 3
#if   (ORIENTATION_0 == ROTATION_0)
#define DSP_DRIVER_0   GUIDRV_LIN_24
#elif (ORIENTATION_0 == ROTATION_CW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSX_24
#elif (ORIENTATION_0 == ROTATION_180)
#define DSP_DRIVER_0   GUIDRV_LIN_OXY_24
#elif (ORIENTATION_0 == ROTATION_CCW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSY_24
#endif
#elif (COLOR_MODE_0 == COLOR_MODE_RGB565)   \
   || (COLOR_MODE_0 == COLOR_MODE_ARGB1555) \
   || (COLOR_MODE_0 == COLOR_MODE_ARGB4444) \
   || (COLOR_MODE_0 == COLOR_MODE_AL88)
#define PIXEL_BYTES_0 2
#if   (ORIENTATION_0 == ROTATION_0)
#define DSP_DRIVER_0   GUIDRV_LIN_16
#elif (ORIENTATION_0 == ROTATION_CW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSX_16
#elif (ORIENTATION_0 == ROTATION_180)
#define DSP_DRIVER_0   GUIDRV_LIN_OXY_16
#elif (ORIENTATION_0 == ROTATION_CCW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSY_16
#endif
#elif (COLOR_MODE_0 == COLOR_MODE_L8)   \
   || (COLOR_MODE_0 == COLOR_MODE_AL44)
#define PIXEL_BYTES_0 1
#if   (ORIENTATION_0 == ROTATION_0)
#define DSP_DRIVER_0   GUIDRV_LIN_8
#elif (ORIENTATION_0 == ROTATION_CW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSX_8
#elif (ORIENTATION_0 == ROTATION_180)
#define DSP_DRIVER_0   GUIDRV_LIN_OXY_8
#elif (ORIENTATION_0 == ROTATION_CCW)
#define DSP_DRIVER_0   GUIDRV_LIN_OSY_8
#endif
#endif

#if	(GUI_NUM_LAYERS > 1)
#if   (COLOR_MODE_1 == COLOR_MODE_ARGB8888)
#define COLOR_CONVERSION_1 GUICC_M8888I
#elif (COLOR_MODE_1 == COLOR_MODE_RGB888)
#define COLOR_CONVERSION_1 GUICC_M888
#elif (COLOR_MODE_1 == COLOR_MODE_RGB565)
#define COLOR_CONVERSION_1 GUICC_M565
#elif (COLOR_MODE_1 == COLOR_MODE_ARGB1555)
#define COLOR_CONVERSION_1 GUICC_M1555I
#elif (COLOR_MODE_1 == COLOR_MODE_ARGB4444)
#define COLOR_CONVERSION_1 GUICC_M4444I
#elif (COLOR_MODE_1 == COLOR_MODE_L8)
#define COLOR_CONVERSION_1 GUICC_8666
#elif (COLOR_MODE_1 == COLOR_MODE_AL44)
#define COLOR_CONVERSION_1 GUICC_1616I
#elif (COLOR_MODE_1 == COLOR_MODE_AL88)
#define COLOR_CONVERSION_1 GUICC_88666I
#else
#error Illegal color mode 1!
#endif

#if   (COLOR_MODE_1 == COLOR_MODE_ARGB8888)
#define PIXEL_BYTES_1 4
#if   (ORIENTATION_1 == ROTATION_0)
#define DSP_DRIVER_1   GUIDRV_LIN_32
#elif (ORIENTATION_1 == ROTATION_CW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSX_32
#elif (ORIENTATION_1 == ROTATION_180)
#define DSP_DRIVER_1   GUIDRV_LIN_OXY_32
#elif (ORIENTATION_1 == ROTATION_CCW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSY_32
#endif
#elif (COLOR_MODE_1 == COLOR_MODE_RGB888)
#define PIXEL_BYTES_1 3
#if   (ORIENTATION_1 == ROTATION_0)
#define DSP_DRIVER_1   GUIDRV_LIN_24
#elif (ORIENTATION_1 == ROTATION_CW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSX_24
#elif (ORIENTATION_1 == ROTATION_180)
#define DSP_DRIVER_1   GUIDRV_LIN_OXY_24
#elif (ORIENTATION_1 == ROTATION_CCW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSY_24
#endif
#elif (COLOR_MODE_1 == COLOR_MODE_RGB565)   \
   || (COLOR_MODE_1 == COLOR_MODE_ARGB1555) \
   || (COLOR_MODE_1 == COLOR_MODE_ARGB4444) \
   || (COLOR_MODE_1 == COLOR_MODE_AL88)
#define PIXEL_BYTES_1 2
#if   (ORIENTATION_1 == ROTATION_0)
#define DSP_DRIVER_1   GUIDRV_LIN_16
#elif (ORIENTATION_1 == ROTATION_CW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSX_16
#elif (ORIENTATION_1 == ROTATION_180)
#define DSP_DRIVER_1   GUIDRV_LIN_OXY_16
#elif (ORIENTATION_1 == ROTATION_CCW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSY_16
#endif
#elif (COLOR_MODE_1 == COLOR_MODE_L8)   \
   || (COLOR_MODE_1 == COLOR_MODE_AL44)
#define PIXEL_BYTES_1 1
#if   (ORIENTATION_1 == ROTATION_0)
#define DSP_DRIVER_1   GUIDRV_LIN_8
#elif (ORIENTATION_1 == ROTATION_CW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSX_8
#elif (ORIENTATION_1 == ROTATION_180)
#define DSP_DRIVER_1   GUIDRV_LIN_OXY_8
#elif (ORIENTATION_1 == ROTATION_CCW)
#define DSP_DRIVER_1   GUIDRV_LIN_OSY_8
#endif
#endif

#else

/*********************************************************************
*
*       Use complete display automatically in case of only one layer
*/
#undef XSIZE_0
#undef YSIZE_0
#define XSIZE_0 XSIZE_PHYS
#define YSIZE_0 YSIZE_PHYS

#endif

/*********************************************************************
*
*       Touch screen defintions
*/

#if	((ORIENTATION_0 == ROTATION_CW) || (ORIENTATION_0 == ROTATION_CCW))
#define WIDTH  YSIZE_PHYS  /* Screen Width (in pixels) */
#define HEIGHT XSIZE_PHYS  /* Screen Height (in pixels)*/
#else
#define WIDTH  XSIZE_PHYS  /* Screen Width (in pixels) */
#define HEIGHT YSIZE_PHYS  /* Screen Height (in pixels)*/
#endif

#if	(ORIENTATION_0 == ROTATION_CW)
#define DSP_ORIENTATION (GUI_SWAP_XY | GUI_MIRROR_X)
#define TOUCH_LEFT   TOUCH_Y_MIN
#define TOUCH_RIGHT  TOUCH_Y_MAX
#define TOUCH_TOP    TOUCH_X_MAX
#define TOUCH_BOTTOM TOUCH_X_MIN
#elif (ORIENTATION_0 == ROTATION_CCW)
#define DSP_ORIENTATION (GUI_SWAP_XY | GUI_MIRROR_Y)
#define TOUCH_LEFT   TOUCH_Y_MAX
#define TOUCH_RIGHT  TOUCH_Y_MIN
#define TOUCH_TOP    TOUCH_X_MIN
#define TOUCH_BOTTOM TOUCH_X_MAX
#elif (ORIENTATION_0 == ROTATION_180)
#define DSP_ORIENTATION (GUI_MIRROR_X | GUI_MIRROR_Y)
#define TOUCH_LEFT   TOUCH_X_MAX
#define TOUCH_RIGHT  TOUCH_X_MIN
#define TOUCH_TOP    TOUCH_Y_MAX
#define TOUCH_BOTTOM TOUCH_Y_MIN
#else
#define DSP_ORIENTATION 0
#define TOUCH_LEFT   TOUCH_X_MIN
#define TOUCH_RIGHT  TOUCH_X_MAX
#define TOUCH_TOP    TOUCH_Y_MIN
#define TOUCH_BOTTOM TOUCH_Y_MAX
#endif

/*********************************************************************
*
*       Redirect bulk conversion to DMA2D routines
*/
#define DEFINE_DMA2D_COLORCONVERSION(PFIX, PIXELFORMAT)                                                        \
static void _Color2IndexBulk_##PFIX##_DMA2D(LCD_COLOR * pColor, void * pIndex, U32 NumItems, U8 SizeOfIndex) { \
  _DMA_Color2IndexBulk(pColor, pIndex, NumItems, SizeOfIndex, PIXELFORMAT);                                    \
}                                                                                                              \
static void _Index2ColorBulk_##PFIX##_DMA2D(void * pIndex, LCD_COLOR * pColor, U32 NumItems, U8 SizeOfIndex) { \
  _DMA_Index2ColorBulk(pIndex, pColor, NumItems, SizeOfIndex, PIXELFORMAT);                                    \
}

/*********************************************************************
*
*       H/V front/backporch and synchronization width/height
*/
#define HFP 	8
#define HSW 	1
#define HBP 	43
#define VFP  	4
#define VSW  	9
#define VBP 	21


/*********************************************************************
*
*       Configuration checking
*/
#if NUM_BUFFERS > 3
#error More than 3 buffers make no sense and are not supported in this configuration file!
#endif
#ifndef   XSIZE_PHYS
#error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
#error Physical Y size of display is not defined!
#endif
#ifndef   NUM_BUFFERS
#define NUM_BUFFERS 2
#else
#if (NUM_BUFFERS <= 0)
#error At least one buffer needs to be defined!
#endif
#endif
#ifndef   NUM_VSCREENS
#define NUM_VSCREENS 1
#else
#if (NUM_VSCREENS <= 0)
#error At least one screeen needs to be defined!
#endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
#error Virtual screens together with multiple buffers are not allowed!
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if (GUI_NUM_LAYERS == 2)
#define VRAM_SIZE \
 ((XSIZE_0 * YSIZE_0 * PIXEL_BYTES_0 * NUM_VSCREENS * NUM_BUFFERS) + \
  (XSIZE_1 * YSIZE_1 * PIXEL_BYTES_1 * NUM_VSCREENS * NUM_BUFFERS))
#else
#define VRAM_SIZE \
  (XSIZE_0 * YSIZE_0 * PIXEL_BYTES_0 * NUM_VSCREENS * NUM_BUFFERS)
#endif
//static uint8_t  _VRAM[VRAM_SIZE];

static const U32 _aAddr[GUI_NUM_LAYERS] = {
    VRAM_ADDR,
#if (GUI_NUM_LAYERS > 1)
    VRAM_ADDR + (XSIZE_0 * YSIZE_0 * PIXEL_BYTES_0 * NUM_VSCREENS * NUM_BUFFERS)
#endif
};

static int _aPendingBuffer[2] = { -1, -1};
static int _aBufferIndex[GUI_NUM_LAYERS];
static int _axSize[GUI_NUM_LAYERS];
static int _aySize[GUI_NUM_LAYERS];
static int _aBytesPerPixels[GUI_NUM_LAYERS];

//
// Prototypes of DMA2D color conversion routines
//
static void _DMA_Index2ColorBulk(void * pIndex, LCD_COLOR * pColor, U32 NumItems, U8 SizeOfIndex, U32 PixelFormat);
static void _DMA_Color2IndexBulk(LCD_COLOR * pColor, void * pIndex, U32 NumItems, U8 SizeOfIndex, U32 PixelFormat);

//
// Color conversion routines using DMA2D
//
DEFINE_DMA2D_COLORCONVERSION(M8888I, LTDC_PIXEL_FORMAT_ARGB8888)
DEFINE_DMA2D_COLORCONVERSION(M888,   LTDC_PIXEL_FORMAT_ARGB8888) // Internal pixel format of emWin is 32 bit, because of that ARGB8888
DEFINE_DMA2D_COLORCONVERSION(M565,   LTDC_PIXEL_FORMAT_RGB565)
DEFINE_DMA2D_COLORCONVERSION(M1555I, LTDC_PIXEL_FORMAT_ARGB1555)
DEFINE_DMA2D_COLORCONVERSION(M4444I, LTDC_PIXEL_FORMAT_ARGB4444)
//
// Buffer for DMA2D color conversion, required because hardware does not support overlapping regions
//
static U32	 					_aBuffer[XSIZE_PHYS * sizeof(U32) * 3];
static U32 * _pBuffer_DMA2D =  &_aBuffer[XSIZE_PHYS * sizeof(U32) * 0];
static U32 * _pBuffer_FG    =  &_aBuffer[XSIZE_PHYS * sizeof(U32) * 1];
static U32 * _pBuffer_BG    =  &_aBuffer[XSIZE_PHYS * sizeof(U32) * 2];
static uint32_t	_CLUT[256];
//
// Array of color conversions for each layer
//
static const LCD_API_COLOR_CONV * _apColorConvAPI[] = {
    COLOR_CONVERSION_0,
#if GUI_NUM_LAYERS > 1
    COLOR_CONVERSION_1,
#endif
};

//
// Array of orientations for each layer
//
//static const int _aOrientation[] = {
//  ORIENTATION_0,
//#if GUI_NUM_LAYERS > 1
//  ORIENTATION_1,
//#endif
//};


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetPixelformat
*/
static U32 _GetPixelformat(int LayerIndex)
{
    const LCD_API_COLOR_CONV * pColorConvAPI;

    if (LayerIndex >= GUI_COUNTOF(_apColorConvAPI))
    {
        return 0;
    }

    pColorConvAPI = _apColorConvAPI[LayerIndex];

    if		(pColorConvAPI == GUICC_M8888I) return LTDC_PIXEL_FORMAT_ARGB8888;
    else if (pColorConvAPI == GUICC_M888) 	return LTDC_PIXEL_FORMAT_RGB888;
    else if (pColorConvAPI == GUICC_M565)	return LTDC_PIXEL_FORMAT_RGB565;
    else if (pColorConvAPI == GUICC_M1555I) return LTDC_PIXEL_FORMAT_ARGB1555;
    else if (pColorConvAPI == GUICC_M4444I) return LTDC_PIXEL_FORMAT_ARGB4444;
    else if (pColorConvAPI == GUICC_8666)	return LTDC_PIXEL_FORMAT_L8;
    else if (pColorConvAPI == GUICC_1616I)	return LTDC_PIXEL_FORMAT_AL44;
    else if (pColorConvAPI == GUICC_88666I) return LTDC_PIXEL_FORMAT_AL88;

    while (1); // Error
}

/*********************************************************************
*
*       _DMA_ExecOperation
*/
static void _DMA_ExecOperation(void)
{
    DMA2D->CR |= DMA2D_CR_START;					// Control Register (Start operation)
    while (DMA2D->CR & DMA2D_CR_START) 				// Wait until transfer is done
    {
        __WFI();									// Sleep until next interrupt
    }
}
/*********************************************************************
*
*       _DMA_Copy
*/
static void _DMA_Copy(int LayerIndex, void * pSrc, void * pDst, int xSize, int ySize, int OffLineSrc, int OffLineDst)
{
    U32 PixelFormat;

    PixelFormat = _GetPixelformat(LayerIndex);
    DMA2D->CR      = 0x00000000UL | (1 << 9);		// Control Register (Memory to memory and TCIE)
    DMA2D->FGMAR   = (U32)pSrc;						// Foreground Memory Address Register (Source address)
    DMA2D->OMAR    = (U32)pDst;						// Output Memory Address Register (Destination address)
    DMA2D->FGOR    = OffLineSrc;					// Foreground Offset Register (Source line offset)
    DMA2D->OOR     = OffLineDst;					// Output Offset Register (Destination line offset)
    DMA2D->FGPFCCR = PixelFormat;					// Foreground PFC Control Register (Defines the input pixel format)
    DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;// Number of Line Register (Size configuration of area to be transfered)
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_Fill
*/
static void _DMA_Fill(int LayerIndex, void * pDst, int xSize, int ySize, int OffLine, U32 ColorIndex)
{
    U32 PixelFormat;

    PixelFormat = _GetPixelformat(LayerIndex);
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00030000UL | (1 << 9);         // Control Register (Register to memory and TCIE)
    DMA2D->OCOLR   = ColorIndex;                      // Output Color Register (Color to be used)
    //
    // Set up pointers
    //
    DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
    //
    // Set up offsets
    //
    DMA2D->OOR     = OffLine;                         // Output Offset Register (Destination line offset)
    //
    // Set up pixel format
    //
    DMA2D->OPFCCR  = PixelFormat;                     // Output PFC Control Register (Defines the output pixel format)
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize; // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_AlphaBlendingBulk
*/
static void _DMA_AlphaBlendingBulk(LCD_COLOR * pColorFG, LCD_COLOR * pColorBG, LCD_COLOR * pColorDst, U32 NumItems)
{
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00020000UL | (1 << 9);         // Control Register (Memory to memory with blending of FG and BG and TCIE)
    //
    // Set up pointers
    //
    DMA2D->FGMAR   = (U32)pColorFG;                   // Foreground Memory Address Register
    DMA2D->BGMAR   = (U32)pColorBG;                   // Background Memory Address Register
    DMA2D->OMAR    = (U32)pColorDst;                  // Output Memory Address Register (Destination address)
    //
    // Set up offsets
    //
    DMA2D->FGOR    = 0;                               // Foreground Offset Register
    DMA2D->BGOR    = 0;                               // Background Offset Register
    DMA2D->OOR     = 0;                               // Output Offset Register
    //
    // Set up pixel format
    //
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;      // Foreground PFC Control Register (Defines the FG pixel format)
    DMA2D->BGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;      // Background PFC Control Register (Defines the BG pixel format)
    DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;      // Output     PFC Control Register (Defines the output pixel format)
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(NumItems << 16) | 1;       // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_MixColors
*
* Purpose:
*   Function for mixing up 2 colors with the given intensity.
*   If the background color is completely transparent the
*   foreground color should be used unchanged.
*/
static LCD_COLOR _DMA_MixColors(LCD_COLOR Color, LCD_COLOR BkColor, U8 Intens)
{

    if ((BkColor & 0xFF000000) == 0xFF000000) {
        return Color;
    }
    *_pBuffer_FG = Color   ^ 0xFF000000;
    *_pBuffer_BG = BkColor ^ 0xFF000000;
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00020000UL | (1 << 9);         // Control Register (Memory to memory with blending of FG and BG and TCIE)
    //
    // Set up pointers
    //
    DMA2D->FGMAR   = (U32)_pBuffer_FG;                // Foreground Memory Address Register
    DMA2D->BGMAR   = (U32)_pBuffer_BG;                // Background Memory Address Register
    DMA2D->OMAR    = (U32)_pBuffer_DMA2D;             // Output Memory Address Register (Destination address)
    //
    // Set up pixel format
    //
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                     | (1UL << 16)
                     | ((U32)Intens << 24);
    DMA2D->BGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                     | (0UL << 16)
                     | ((U32)(255 - Intens) << 24);
    DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(1 << 16) | 1;              // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //

    //_DMA_ExecOperation();
    DMA2D->CR     |= DMA2D_CR_START;                  // Control Register (Start operation)
    //
    // Wait until transfer is done
    //
    while (DMA2D->CR & DMA2D_CR_START) {
        __WFI();                                        // Sleep until next interrupt
    }

    return _pBuffer_DMA2D[0] ^ 0xFF000000;
}

/*********************************************************************
*
*       _DMA_MixColorsBulk
*/
static void _DMA_MixColorsBulk(LCD_COLOR * pColorFG, LCD_COLOR * pColorBG, LCD_COLOR * pColorDst, U8 Intens, U32 NumItems)
{
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00020000UL | (1 << 9);         // Control Register (Memory to memory with blending of FG and BG and TCIE)
    //
    // Set up pointers
    //
    DMA2D->FGMAR   = (U32)pColorFG;                   // Foreground Memory Address Register
    DMA2D->BGMAR   = (U32)pColorBG;                   // Background Memory Address Register
    DMA2D->OMAR    = (U32)pColorDst;                  // Output Memory Address Register (Destination address)
    //
    // Set up pixel format
    //
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                     | (1UL << 16)
                     | ((U32)Intens << 24);
    DMA2D->BGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                     | (0UL << 16)
                     | ((U32)(255 - Intens) << 24);
    DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(NumItems << 16) | 1;              // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_ConvertColor
*/
static void _DMA_ConvertColor(void * pSrc, void * pDst,  U32 PixelFormatSrc, U32 PixelFormatDst, U32 NumItems)
{
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00010000UL | (1 << 9);         // Control Register (Memory to memory with pixel format conversion and TCIE)
    //
    // Set up pointers
    //
    DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
    DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
    //
    // Set up offsets
    //
    DMA2D->FGOR    = 0;                               // Foreground Offset Register (Source line offset)
    DMA2D->OOR     = 0;                               // Output Offset Register (Destination line offset)
    //
    // Set up pixel format
    //
    DMA2D->FGPFCCR = PixelFormatSrc;                  // Foreground PFC Control Register (Defines the input pixel format)
    DMA2D->OPFCCR  = PixelFormatDst;                  // Output PFC Control Register (Defines the output pixel format)
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(NumItems << 16) | 1;       // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_DrawBitmapL8
*/
static void _DMA_DrawBitmapL8(void * pSrc, void * pDst,  U32 OffSrc, U32 OffDst, U32 PixelFormatDst, U32 xSize, U32 ySize)
{
    //
    // Set up mode
    //
    DMA2D->CR      = 0x00010000UL | (1 << 9);         // Control Register (Memory to memory with pixel format conversion and TCIE)
    //
    // Set up pointers
    //
    DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
    DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
    //
    // Set up offsets
    //
    DMA2D->FGOR    = OffSrc;                          // Foreground Offset Register (Source line offset)
    DMA2D->OOR     = OffDst;                          // Output Offset Register (Destination line offset)
    //
    // Set up pixel format
    //
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_L8;            // Foreground PFC Control Register (Defines the input pixel format)
    DMA2D->OPFCCR  = PixelFormatDst;                  // Output PFC Control Register (Defines the output pixel format)
    //
    // Set up size
    //
    DMA2D->NLR     = (U32)(xSize << 16) | ySize;      // Number of Line Register (Size configuration of area to be transfered)
    //
    // Execute operation
    //
    _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_LoadLUT
*/
static void _DMA_LoadLUT(LCD_COLOR * pColor, U32 NumItems)
{
    DMA2D->FGCMAR  = (U32)pColor;                     	// Foreground CLUT Memory Address Register
    //
    // Foreground PFC Control Register
    //
    DMA2D->FGPFCCR  = LTDC_PIXEL_FORMAT_RGB888        	// Pixel format
//	DMA2D->FGPFCCR  = LTDC_PIXEL_FORMAT_RGB565
                      | ((NumItems - 1) & 0xFF) << 8;   		// Number of items to load
    DMA2D->FGPFCCR |= (1 << 5);                       	// Start loading
    //
    // Waiting not required here...
    //
}

/*********************************************************************
*
*       _InvertAlpha_SwapRB
*
* Purpose:
*   Color format of DMA2D is different to emWin color format. This routine
*   swaps red and blue and inverts alpha that it is compatible to emWin
*   and vice versa.
*/
static void _InvertAlpha_SwapRB(LCD_COLOR * pColorSrc, LCD_COLOR * pColorDst, U32 NumItems)
{
    U32 Color;

    do {
        Color = *pColorSrc++;
        *pColorDst++ = ((Color & 0x000000FF) << 16)         // Swap red <-> blue
                       |  (Color & 0x0000FF00)                // Green
                       | ((Color & 0x00FF0000) >> 16)         // Swap red <-> blue
                       | ((Color & 0xFF000000) ^ 0xFF000000); // Invert alpha
    } while (--NumItems);
}

/*********************************************************************
*
*       _InvertAlpha
*
* Purpose:
*   Color format of DMA2D is different to emWin color format. This routine
*   inverts alpha that it is compatible to emWin and vice versa.
*   Changes are done in the destination memory location.
*/
static void _InvertAlpha(LCD_COLOR * pColorSrc, LCD_COLOR * pColorDst, U32 NumItems)
{
    U32 Color;

    do {
        Color = *pColorSrc++;
        *pColorDst++ = Color ^ 0xFF000000; // Invert alpha
    } while (--NumItems);
}

/*********************************************************************
*
*       _DMA_AlphaBlending
*/
static void _DMA_AlphaBlending(LCD_COLOR * pColorFG, LCD_COLOR * pColorBG, LCD_COLOR * pColorDst, U32 NumItems)
{
    //
    // Invert alpha values
    //
    _InvertAlpha(pColorFG, _pBuffer_FG, NumItems);
    _InvertAlpha(pColorBG, _pBuffer_BG, NumItems);
    //
    // Use DMA2D for mixing
    //
    _DMA_AlphaBlendingBulk(_pBuffer_FG, _pBuffer_BG, _pBuffer_DMA2D, NumItems);
    //
    // Invert alpha values
    //
    _InvertAlpha(_pBuffer_DMA2D, pColorDst, NumItems);
}

/*********************************************************************
*
*       _DMA_Index2ColorBulk
*
* Purpose:
*   This routine is used by the emWin color conversion routines to use DMA2D for
*   color conversion. It converts the given index values to 32 bit colors.
*   Because emWin uses ABGR internally and 0x00 and 0xFF for opaque and fully
*   transparent the color array needs to be converted after DMA2D has been used.
*/
static void _DMA_Index2ColorBulk(void * pIndex, LCD_COLOR * pColor, U32 NumItems, U8 SizeOfIndex, U32 PixelFormat)
{
    _DMA_ConvertColor(pIndex, _pBuffer_DMA2D, PixelFormat, LTDC_PIXEL_FORMAT_ARGB8888, NumItems);	// Use DMA2D for the conversion
    _InvertAlpha_SwapRB(_pBuffer_DMA2D, pColor, NumItems);											// Convert colors from ARGB to ABGR and invert alpha values
}

/*********************************************************************
*
*       _DMA_Color2IndexBulk
*
* Purpose:
*   This routine is used by the emWin color conversion routines to use DMA2D for
*   color conversion. It converts the given 32 bit color array to index values.
*   Because emWin uses ABGR internally and 0x00 and 0xFF for opaque and fully
*   transparent the given color array needs to be converted before DMA2D can be used.
*/
static void _DMA_Color2IndexBulk(LCD_COLOR * pColor, void * pIndex, U32 NumItems, U8 SizeOfIndex, U32 PixelFormat)
{
    _InvertAlpha_SwapRB(pColor, _pBuffer_DMA2D, NumItems);											// Convert colors from ABGR to ARGB and invert alpha values
    _DMA_ConvertColor(_pBuffer_DMA2D, pIndex, LTDC_PIXEL_FORMAT_ARGB8888, PixelFormat, NumItems);	// Use DMA2D for the conversion
}

/*********************************************************************
*
*       _LCD_MixColorsBulk
*/
static void _LCD_MixColorsBulk(U32 * pFG, U32 * pBG, U32 * pDst, unsigned OffFG, unsigned OffBG, unsigned OffDest, unsigned xSize, unsigned ySize, U8 Intens)
{
    int y;

    GUI_USE_PARA(OffFG);
    GUI_USE_PARA(OffDest);

    for (y = 0; y < ySize; y++)
    {
        _InvertAlpha(pFG, _pBuffer_FG, xSize); // Invert alpha values
        _InvertAlpha(pBG, _pBuffer_BG, xSize);
        _DMA_MixColorsBulk(_pBuffer_FG, _pBuffer_BG, _pBuffer_DMA2D, Intens, xSize);
        _InvertAlpha(_pBuffer_DMA2D, pDst, xSize);
        pFG  += xSize + OffFG;
        pBG  += xSize + OffBG;
        pDst += xSize + OffDest;
    }
}

/*********************************************************************
*
*       _LCD_DisplayOn
*/
static void _LCD_DisplayOn(void)
{
    __HAL_LTDC_ENABLE(&hltdc);	// Display On
//	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_SET);
//	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2,  GPIO_PIN_SET);
}

/*********************************************************************
*
*       _LCD_DisplayOff
*/
static void _LCD_DisplayOff(void)
{
    __HAL_LTDC_DISABLE(&hltdc);	// Display Off
//	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_RESET);
//	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2,  GPIO_PIN_RESET);
}

/*********************************************************************
*
*       _GetBufferSize
*/
static U32 _GetBufferSize(int LayerIndex)
{
    U32 BufferSize;

    BufferSize = _axSize[LayerIndex] * _aySize[LayerIndex] * _aBytesPerPixels[LayerIndex];
    return BufferSize;
}

/*********************************************************************
*
*       _LCD_CopyBuffer
*/
static void _LCD_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst)
{
    U32 BufferSize, AddrSrc, AddrDst;

    BufferSize = _GetBufferSize(LayerIndex);
    AddrSrc    = _aAddr[LayerIndex] + BufferSize * IndexSrc;
    AddrDst    = _aAddr[LayerIndex] + BufferSize * IndexDst;
    _DMA_Copy(LayerIndex, (void *)AddrSrc, (void *)AddrDst, _axSize[LayerIndex], _aySize[LayerIndex], 0, 0);
    _aBufferIndex[LayerIndex] = IndexDst; // After this function has been called all drawing operations are routed to Buffer[IndexDst]!
}

/*********************************************************************
*
*       _LCD_CopyRect
*/
static void _LCD_CopyRect(int LayerIndex, int x0, int y0, int x1, int y1, int xSize, int ySize)
{
    U32 BufferSize, AddrSrc, AddrDst;
    int OffLine;

    BufferSize = _GetBufferSize(LayerIndex);
    AddrSrc = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y0 * _axSize[LayerIndex] + x0) * _aBytesPerPixels[LayerIndex];
    AddrDst = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y1 * _axSize[LayerIndex] + x1) * _aBytesPerPixels[LayerIndex];
    OffLine = _axSize[LayerIndex] - xSize;
    _DMA_Copy(LayerIndex, (void *)AddrSrc, (void *)AddrDst, xSize, ySize, OffLine, OffLine);
}

/*********************************************************************
*
*       _LCD_FillRect
*/
static void _LCD_FillRect(int LayerIndex, int x0, int y0, int x1, int y1, U32 PixelIndex)
{
    U32 BufferSize, AddrDst;
    int xSize, ySize;

    if (GUI_GetDrawMode() == GUI_DM_XOR)
    {
        LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
        LCD_FillRect(x0, y0, x1, y1);
        LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))_LCD_FillRect);
    }
    else
    {
        xSize = x1 - x0 + 1;
        ySize = y1 - y0 + 1;
        BufferSize = _GetBufferSize(LayerIndex);
        AddrDst = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y0 * _axSize[LayerIndex] + x0) * _aBytesPerPixels[LayerIndex];
        _DMA_Fill(LayerIndex, (void *)AddrDst, xSize, ySize, _axSize[LayerIndex] - xSize, PixelIndex);
    }
}

/**
  * @brief  Draw indirect color bitmap
  * @param  pSrc: pointer to the source
  * @param  pDst: pointer to the destination
  * @param  OffSrc: offset source
  * @param  OffDst: offset destination
  * @param  PixelFormatDst: pixel format for destination
  * @param  xSize: X size
  * @param  ySize: Y size
  * @retval None
  */
static void _LCD_DrawBitmap32bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
    U32 BufferSize, AddrDst;
    int OffLineSrc, OffLineDst;

    BufferSize = _GetBufferSize(LayerIndex);
    AddrDst = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y * _axSize[LayerIndex] + x) * _aBytesPerPixels[LayerIndex];
    OffLineSrc = (BytesPerLine / 4) - xSize;
    OffLineDst = _axSize[LayerIndex] - xSize;
    _DMA_Copy(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}
/*********************************************************************
*
*       _LCD_DrawBitmap16bpp
*/
static void _LCD_DrawBitmap16bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine)
{
    U32 BufferSize, AddrDst;
    int OffLineSrc, OffLineDst;

    BufferSize = _GetBufferSize(LayerIndex);
    AddrDst = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y * _axSize[LayerIndex] + x) * _aBytesPerPixels[LayerIndex];
    OffLineSrc = (BytesPerLine / 2) - xSize;
    OffLineDst = _axSize[LayerIndex] - xSize;
    _DMA_Copy(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}
/*********************************************************************
*
*       _LCD_DrawBitmap8bpp
*/
static void _LCD_DrawBitmap8bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
    U32 BufferSize, AddrDst;
    int OffLineSrc, OffLineDst;
    U32 PixelFormat;

    BufferSize = _GetBufferSize(LayerIndex);
    AddrDst = _aAddr[LayerIndex] + BufferSize * _aBufferIndex[LayerIndex] + (y * _axSize[LayerIndex] + x) * _aBytesPerPixels[LayerIndex];
    OffLineSrc = BytesPerLine - xSize;
    OffLineDst = _axSize[LayerIndex] - xSize;
    PixelFormat = _GetPixelformat(LayerIndex);
    _DMA_DrawBitmapL8((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);
}

/*********************************************************************
*
*       _LCD_GetpPalConvTable
*
* Purpose:
*   The emWin function LCD_GetpPalConvTable() normally translates the given colors into
*   index values for the display controller. In case of index based bitmaps without
*   transparent pixels we load the palette only to the DMA2D LUT registers to be
*   translated (converted) during the process of drawing via DMA2D.
*/
static LCD_PIXELINDEX * _LCD_GetpPalConvTable(const LCD_LOGPALETTE GUI_UNI_PTR * pLogPal, const GUI_BITMAP GUI_UNI_PTR * pBitmap, int LayerIndex)
{
    void (* pFunc)(void);
    int DoDefault = 0;
    //
    // Check if we have a non transparent device independent bitmap
    //
    if (pBitmap->BitsPerPixel == 8)
    {
        pFunc = LCD_GetDevFunc(LayerIndex, LCD_DEVFUNC_DRAWBMP_8BPP);

        if (pFunc)
        {
            if (pBitmap->pPal)
            {
                if (pBitmap->pPal->HasTrans) DoDefault = 1;
            }
            else DoDefault = 1;
        }
        else DoDefault = 1;
    }
    else DoDefault = 1;

    if (DoDefault) 											// Default palette management for other cases
    {
        return LCD_GetpPalConvTable(pLogPal);				// Return a pointer to the index values to be used by the controller
    }
    _InvertAlpha_SwapRB((U32 *)pLogPal->pPalEntries,
                        _pBuffer_DMA2D,
                        pLogPal->NumEntries);		// Convert palette colors from ARGB to ABGR
    _DMA_LoadLUT(_pBuffer_DMA2D, pLogPal->NumEntries);		// Load LUT using DMA2D
    return _pBuffer_DMA2D;									// Return something not NULL
}

/*********************************************************************
*
*       _LCD_SetOrg
*/
static void _LCD_SetOrg(int LayerIndex, int xPos, int yPos)
{
    uint32_t Address;

    Address = _aAddr[LayerIndex] + (xPos + yPos * _axSize[LayerIndex]) * _aBytesPerPixels[LayerIndex];
    HAL_LTDC_SetAddress(&hltdc, Address, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetLUTEntry
*/
static void _LCD_SetLUTEntry(int LayerIndex, LCD_COLOR Color, U8 Pos)
{
    _CLUT[Pos] = ((Color & 0xFF0000) >> 16) |
                 (Color & 0x00FF00)        |
                 ((Color & 0x0000FF) << 16);
    HAL_LTDC_ConfigCLUT(&hltdc, _CLUT, 256, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetVis
*/
static void _LCD_SetVis(int LayerIndex, int OnOff)
{
    if (OnOff) 	__HAL_LTDC_LAYER_ENABLE (&hltdc, LayerIndex);
    else 		__HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);

    __HAL_LTDC_RELOAD_CONFIG(&hltdc);
}

/*********************************************************************
*
*       _LCD_InitLayer
*/
static void _LCD_InitLayer(int LayerIndex)
{
    LTDC_LayerCfgTypeDef LayerCfg;

    if (LayerIndex < GUI_NUM_LAYERS)
    {
        LayerCfg.WindowX0    	 = 0;								// Windowing configuration
        LayerCfg.WindowX1    	 = LCD_GetXSizeEx(LayerIndex);
        LayerCfg.WindowY0		 = 0;
        LayerCfg.WindowY1		 = LCD_GetYSizeEx(LayerIndex);
        LayerCfg.ImageWidth		 = LCD_GetXSizeEx(LayerIndex);
        LayerCfg.ImageHeight 	 = LCD_GetYSizeEx(LayerIndex);
        LayerCfg.PixelFormat 	 = _GetPixelformat(LayerIndex);		// Pixel Format configuration
        LayerCfg.Alpha  		 = 255;								// Alpha constant (255 totally opaque)
        LayerCfg.Alpha0 		 = 0;
        LayerCfg.Backcolor.Blue  = 0;								// Back Color configuration
        LayerCfg.Backcolor.Green = 0;
        LayerCfg.Backcolor.Red   = 0;
        LayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;		// Configure blending factors
        LayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
        LayerCfg.FBStartAdress	 = _aAddr[LayerIndex];				// Input Address configuration
        HAL_LTDC_ConfigLayer(&hltdc, &LayerCfg, LayerIndex);		// Configure Layer

        if (LCD_GetBitsPerPixelEx(LayerIndex) <= 8)
        {
            HAL_LTDC_EnableCLUT(&hltdc, LayerIndex);				// Enable usage of LUT for all modes with <= 8bpp*/
        }
    }
}
/*********************************************************************
*
*       _LCD_SetLayerPos
*/
static void _LCD_SetLayerPos(int LayerIndex, int xPos, int yPos)
{
    HAL_LTDC_SetWindowPosition(&hltdc, xPos, yPos, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetLayerSize
*/
static void _LCD_SetLayerSize(int LayerIndex, int xSize, int ySize)
{
    HAL_LTDC_SetWindowSize(&hltdc, xSize, ySize, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetLayerAlpha
*/
static void _LCD_SetLayerAlpha(int LayerIndex, int Alpha)
{
    HAL_LTDC_SetAlpha(&hltdc, 255 - Alpha, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetChromaMode
*/
static void _LCD_SetChromaMode(int LayerIndex, int ChromaMode)
{
    if (ChromaMode) HAL_LTDC_EnableColorKeying (&hltdc, LayerIndex);
    else   			HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);
}

/*********************************************************************
*
*       _LCD_SetChroma
*/
static void _LCD_SetChroma(int LayerIndex, LCD_COLOR ChromaMin, LCD_COLOR ChromaMax)
{
    uint32_t RGB_Value;

    RGB_Value = ((ChromaMin & 0xFF0000) >> 16) |
                (ChromaMin & 0x00FF00)        |
                ((ChromaMin & 0x0000FF) << 16);
    HAL_LTDC_ConfigColorKeying(&hltdc, RGB_Value, LayerIndex);
}
/*********************************************************************
*
*       HAL_LTDC_LineEvenCallback
*
* Purpose:
*   Line Event callback for managing multiple buffering
*/
void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc)
{
    U32 Addr;
    int i;

    for (i = 0; i < GUI_NUM_LAYERS; i++)
    {
        if (_aPendingBuffer[i] >= 0)
        {
            Addr = _aAddr[i] + _axSize[i] * _aySize[i] *
                   _aPendingBuffer[i] * _aBytesPerPixels[i];	// Calculate address of buffer to be used  as visible frame buffer
            HAL_LTDC_SetAddress(hltdc, Addr, i);				// Set address
            __HAL_LTDC_RELOAD_CONFIG(hltdc);					// Reload configuration
            GUI_MULTIBUF_ConfirmEx(i, _aPendingBuffer[i]);		// Tell emWin that buffer is used
            _aPendingBuffer[i] = -1;							// Clear pending buffer flag of layer
        }
    }

    HAL_LTDC_ProgramLineEvent(hltdc, 0);
}


/*********************************************************************
*
*       HAL_LTDC_MspInit
*
* Purpose:
*   This routine should be called
*   to set port pins to their initial values
*/
void HAL_LTDC_MspInit(LTDC_HandleTypeDef* hltdc)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_LTDC_CLK_ENABLE();  // Enable LTDC Clock
    __HAL_RCC_DMA2D_CLK_ENABLE(); // Enable DMA2D Clock

    /* Enable GPIOs clock */
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();
    __GPIOF_CLK_ENABLE();
    __GPIOG_CLK_ENABLE();


    /**LTDC GPIO Configuration
    PF10     ------> LTDC_DE
    PC0     ------> LTDC_R5
    PA3     ------> LTDC_B5
    PA4     ------> LTDC_VSYNC
    PA5     ------> LTDC_R4
    PA6     ------> LTDC_G2
    PB0     ------> LTDC_R3
    PB1     ------> LTDC_R6
    PB10     ------> LTDC_G4
    PB11     ------> LTDC_G5
    PG6     ------> LTDC_R7
    PG7     ------> LTDC_CLK
    PC6     ------> LTDC_HSYNC
    PC7     ------> LTDC_G6
    PD3     ------> LTDC_G7
    PG10     ------> LTDC_G3
    PG11     ------> LTDC_B3
    PG12     ------> LTDC_B4
    PB8     ------> LTDC_B6
    PB9     ------> LTDC_B7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2,  GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


//	GPIO_InitTypeDef GPIO_InitStructure;

//	__HAL_RCC_LTDC_CLK_ENABLE();  // Enable LTDC Clock
//	__HAL_RCC_DMA2D_CLK_ENABLE(); // Enable DMA2D Clock
//
//	/* Enable GPIOs clock */
//	__GPIOE_CLK_ENABLE();
//	__GPIOG_CLK_ENABLE();
//	__GPIOI_CLK_ENABLE();
//	__GPIOJ_CLK_ENABLE();
//	__GPIOK_CLK_ENABLE();

//	/* GPIOs configuration */
//	/*
//	+------------------+-------------------+-------------------+
//	+                   LCD pins assignment                    +
//	+------------------+-------------------+-------------------+
//	|  LCD_R0 <-> PI15 |  LCD_G0 <-> PJ7   |  LCD_B0 <-> PE4   |
//	|  LCD_R1 <-> PJ0  |  LCD_G1 <-> PJ8   |  LCD_B1 <-> PJ13  |
//	|  LCD_R2 <-> PJ1  |  LCD_G2 <-> PJ9   |  LCD_B2 <-> PJ14  |
//	|  LCD_R3 <-> PJ2  |  LCD_G3 <-> PJ10  |  LCD_B3 <-> PJ15  |
//	|  LCD_R4 <-> PJ3  |  LCD_G4 <-> PJ11  |  LCD_B4 <-> PG12  |
//	|  LCD_R5 <-> PJ4  |  LCD_G5 <-> PK0   |  LCD_B5 <-> PK4   |
//	|  LCD_R6 <-> PJ5  |  LCD_G6 <-> PK1   |  LCD_B6 <-> PK5   |
//	|  LCD_R7 <-> PJ6  |  LCD_G7 <-> PK2   |  LCD_B7 <-> PK6   |
//	------------------------------------------------------------
//	|  LCD_HSYNC <-> PI10         |  LCD_VSYNC <-> PI9         |
//	|  LCD_CLK   <-> PI14         |  LCD_DE    <-> PK7         |
//	|  LCD_DISP  <-> PI12 (GPIO)  |  LCD_INT   <-> PI13        |
//	------------------------------------------------------------
//	|  LCD_SCL <-> PH7 (I2C3 SCL) | LCD_SDA <-> PH8 (I2C3 SDA) |
//	------------------------------------------------------------
//	|  LCD_BL_CTRL <-> PK3 (GPIO) |
//	-------------------------------
//	*/
//	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
//	GPIO_InitStructure.Pull = GPIO_NOPULL;
//	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;

//	GPIO_InitStructure.Alternate= GPIO_AF9_LTDC;

//	/* GPIOG configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_12;
//	HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

//	GPIO_InitStructure.Alternate= GPIO_AF14_LTDC;

//	/* GPIOE configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_4;
//	HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

//	/* GPIOI configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
//	HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);

//	/* GPIOJ configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  |
//						   GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7  |
//						   GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 |
//										 GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//	HAL_GPIO_Init(GPIOJ, &GPIO_InitStructure);

//	/* GPIOK configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  |
//						   GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7;
//	HAL_GPIO_Init(GPIOK, &GPIO_InitStructure);

//	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;

//	/* GPIOI PI12 configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_12;
//	HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);

//	/* GPIOK PK3 configuration */
//	GPIO_InitStructure.Pin = GPIO_PIN_3;
//	HAL_GPIO_Init(GPIOK, &GPIO_InitStructure);
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*
* Return Value:
*   < -1 - Error
*     -1 - Command not handled
*      0 - Ok
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * p)
{
    int r = 0;

    switch (Cmd)
    {
    case LCD_X_INITCONTROLLER:	// Called during the initialization process in order to set up the display controller and put it into operation.
    {
        _LCD_InitLayer(LayerIndex);
        break;
    }

    case LCD_X_SETORG: 			// Required for setting the display origin which is passed in the 'xPos' and 'yPos' element of p
    {
        LCD_X_SETORG_INFO * pSetOrgInfo;
        pSetOrgInfo = (LCD_X_SETORG_INFO *)p;
        _LCD_SetOrg(LayerIndex, pSetOrgInfo->xPos,pSetOrgInfo->yPos);
        break;
    }

    case LCD_X_SHOWBUFFER: 		// Required if multiple buffers are used. The 'Index' element of p contains the buffer index.
    {
        LCD_X_SHOWBUFFER_INFO * pShowBuffInfo;
        pShowBuffInfo = (LCD_X_SHOWBUFFER_INFO *)p;
        _aPendingBuffer[LayerIndex] = pShowBuffInfo->Index;
        break;
    }

    case LCD_X_SETLUTENTRY: 	// Required for setting a lookup table entry which is passed in the 'Pos' and 'Color' element of p
    {
        LCD_X_SETLUTENTRY_INFO * pSetLutEntryInfo;
        pSetLutEntryInfo = (LCD_X_SETLUTENTRY_INFO *)p;
        _LCD_SetLUTEntry(LayerIndex, pSetLutEntryInfo->Color, pSetLutEntryInfo->Pos);
        break;
    }

    case LCD_X_ON: 				// Required if the display controller should support switching on and off
    {
        _LCD_DisplayOn();
        break;
    }

    case LCD_X_OFF: 			// Required if the display controller should support switching on and off
    {
        _LCD_DisplayOff();
        break;
    }

    case LCD_X_SETVIS: 			// Required for setting the layer visibility which is passed in the 'OnOff' element of pData
    {
        LCD_X_SETVIS_INFO * pSetvisInfo;
        pSetvisInfo = (LCD_X_SETVIS_INFO *)p;
        _LCD_SetVis(LayerIndex, pSetvisInfo->OnOff);
        break;
    }

    case LCD_X_SETPOS: 			// Required for setting the layer position which is passed in the 'xPos' and 'yPos' element of pData
    {
        LCD_X_SETPOS_INFO * pSetposInfo;
        pSetposInfo = (LCD_X_SETPOS_INFO *)p;
        _LCD_SetLayerPos(LayerIndex, pSetposInfo->xPos, pSetposInfo->yPos);
        break;
    }

    case LCD_X_SETSIZE: 		// Required for setting the layer position which is passed in the 'xPos' and 'yPos' element of pData
    {
        int xPos, yPos;
        LCD_X_SETSIZE_INFO * pSetSizeInfo;
        GUI_GetLayerPosEx(LayerIndex, &xPos, &yPos);
        pSetSizeInfo = (LCD_X_SETSIZE_INFO *)p;

        if (LCD_GetSwapXYEx(LayerIndex))
        {
            _axSize[LayerIndex] = pSetSizeInfo->ySize;
            _aySize[LayerIndex] = pSetSizeInfo->xSize;
        }
        else
        {
            _axSize[LayerIndex] = pSetSizeInfo->xSize;
            _aySize[LayerIndex] = pSetSizeInfo->ySize;
        }

        _LCD_SetLayerSize(LayerIndex, xPos, yPos);
        break;
    }

    case LCD_X_SETALPHA: 		// Required for setting the alpha value which is passed in the 'Alpha' element of pData
    {
        LCD_X_SETALPHA_INFO * pSetAlphaInfo;
        pSetAlphaInfo = (LCD_X_SETALPHA_INFO *)p;
        _LCD_SetLayerAlpha(LayerIndex, pSetAlphaInfo->Alpha);
        break;
    }

    case LCD_X_SETCHROMAMODE:	// Required for setting the chroma mode which is passed in the 'ChromaMode' element of pData
    {
        LCD_X_SETCHROMAMODE_INFO * pSetChromaModeInfo;
        pSetChromaModeInfo = (LCD_X_SETCHROMAMODE_INFO *)p;
        _LCD_SetChromaMode(LayerIndex, pSetChromaModeInfo->ChromaMode);
        break;
    }

    case LCD_X_SETCHROMA:		// Required for setting the chroma value which is passed in the 'ChromaMin' and 'ChromaMax' element of pData
    {
        LCD_X_SETCHROMA_INFO * pSetChromaInfo;
        pSetChromaInfo = (LCD_X_SETCHROMA_INFO *)p;
        _LCD_SetChroma(LayerIndex, pSetChromaInfo->ChromaMin, pSetChromaInfo->ChromaMax);
        break;
    }

    default:
    {
        r = -1;
        break;
    }
    }

    return r;
}

/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*
*/
void LCD_X_Config(void)
{
    int i;
    //U32 PixelFormat;

    HAL_LTDC_DeInit(&hltdc);

    hltdc.Instance 					= LTDC;						// LTDC configuration
    hltdc.Init.HSPolarity 			= LTDC_HSPOLARITY_AL;		// Horizontal synchronization polarity as active low
    hltdc.Init.VSPolarity 			= LTDC_VSPOLARITY_AL;		// Vertical synchronization polarity as active low
    hltdc.Init.DEPolarity 			= LTDC_DEPOLARITY_AL;		// Data enable polarity as active low
    hltdc.Init.PCPolarity 			= LTDC_PCPOLARITY_IPC;		// Pixel clock polarity as input pixel clock
    hltdc.Init.Backcolor.Red   		= (BK_COLOR >>  0) & 0xFF;	// Configure R,G,B component values for LCD background color
    hltdc.Init.Backcolor.Green 		= (BK_COLOR >>  8) & 0xFF;
    hltdc.Init.Backcolor.Blue  		= (BK_COLOR >> 16) & 0xFF;
    hltdc.Init.HorizontalSync		= HSW;                    	// Horizontal synchronization width
    hltdc.Init.VerticalSync       	= VSW;                    	// Vertical synchronization height
    hltdc.Init.AccumulatedHBP     	= HBP;                    	// Accumulated horizontal back porch
    hltdc.Init.AccumulatedVBP     	= VBP;                    	// Accumulated vertical back porch
    hltdc.Init.AccumulatedActiveW 	= HBP + XSIZE_PHYS;       	// Accumulated active width
    hltdc.Init.AccumulatedActiveH 	= VBP + YSIZE_PHYS;       	// Accumulated active height
    hltdc.Init.TotalWidth         	= HBP + XSIZE_PHYS + HFP; 	// Total width
    hltdc.Init.TotalHeigh         	= VBP + YSIZE_PHYS + VFP;						// Total height

    HAL_LTDC_Init(&hltdc); 										// Initialize LTDC
    HAL_LTDC_ProgramLineEvent(&hltdc, 0);						// Set LTDC interrupt event
    HAL_LTDC_EnableDither(&hltdc);								// Enable dithering

    HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0); 					// Set LTDC Interrupt to the lowest priority
    HAL_NVIC_EnableIRQ(LTDC_IRQn);								// Enable LTDC Interrupt

    HAL_NVIC_SetPriority(DMA2D_IRQn, 0xE, 0);					// Set DMA2DInterrupt to the lowest priority
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);								// Enable DMA2D Interrupt
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);			// Display enable
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5,  GPIO_PIN_SET);		// Display LED On

#if (NUM_BUFFERS > 1)

    for (i = 0; i < GUI_NUM_LAYERS; i++) 										// At first initialize use of multiple buffers on demand
    {
        GUI_MULTIBUF_ConfigEx(i, NUM_BUFFERS);
    }
#endif

    GUI_DEVICE_CreateAndLink(DSP_DRIVER_0, COLOR_CONVERSION_0, 0, 0);					// Set display driver and color conversion for 1st layer

    if (LCD_GetSwapXYEx(0)) 																// Set size of 1st layer
    {
        LCD_SetSizeEx (0, YSIZE_0, XSIZE_0);
        LCD_SetVSizeEx(0, YSIZE_0 * NUM_VSCREENS, XSIZE_0);
    }
    else
    {
        LCD_SetSizeEx (0, XSIZE_0, YSIZE_0);
        LCD_SetVSizeEx(0, XSIZE_0, YSIZE_0 * NUM_VSCREENS);
    }
    LCD_SetVisEx(0, 1); // Layer 0 On


#if (GUI_NUM_LAYERS > 1)

    GUI_DEVICE_CreateAndLink(DSP_DRIVER_1, COLOR_CONVERSION_1, 0, 1);		// Set display driver and color conversion for 2nd layer

    if (LCD_GetSwapXYEx(1))														// Set size of 2nd layer
    {
        LCD_SetSizeEx (1, YSIZE_0, XSIZE_1);
        LCD_SetVSizeEx(1, YSIZE_1 * NUM_VSCREENS, XSIZE_1);
    }
    else
    {
        LCD_SetSizeEx (1, XSIZE_1, YSIZE_1);
        LCD_SetVSizeEx(1, XSIZE_1, YSIZE_1 * NUM_VSCREENS);
    }
    LCD_SetVisEx(1, 1); // Layer 1 On
#endif

    for (i = 0; i < GUI_NUM_LAYERS; i++)													// Setting up VRam address and remember pixel size
    {
        LCD_SetVRAMAddrEx(i, (void *)(_aAddr[i]));								// Setting up VRam address
        _aBytesPerPixels[i] = LCD_GetBitsPerPixelEx(i) >> 3;                  	// Remember pixel size
    }

    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYBUFFER, (void(*)(void))_LCD_CopyBuffer);				// Set custom function for copying complete buffers (used by multiple buffering) using DMA2D
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYRECT, (void(*)(void))_LCD_CopyRect);					// Set custom function for copy recxtangle areas (used by GUI_CopyRect()) using DMA2D
    LCD_SetDevFunc(i, LCD_DEVFUNC_FILLRECT, (void(*)(void))_LCD_FillRect);					// Set custom function for filling operations using DMA2D
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_32BPP, (void(*)(void))_LCD_DrawBitmap32bpp);		// Set up drawing routine for 32bpp bitmap using DMA2D. Makes only sense with ARGB8888 */
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_16BPP, (void(*)(void))_LCD_DrawBitmap16bpp);		// Set up drawing routine for 16bpp bitmap using DMA2D. Makes only sense with RGB565
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_8BPP, (void(*)(void))_LCD_DrawBitmap8bpp);		// Set up custom drawing routine for index based bitmaps using DMA2D

    /********************************************************************************************/
    /*		 			Set up custom color conversion using DMA2D, 							*/
    /*		works only for direct color modes because of missing LUT for DMA2D destination		*/
    /********************************************************************************************/
    GUICC_M1555I_SetCustColorConv	(_Color2IndexBulk_M1555I_DMA2D, _Index2ColorBulk_M1555I_DMA2D); 	// Set up custom bulk color conversion using DMA2D for ARGB1555
    GUICC_M565_SetCustColorConv  	(_Color2IndexBulk_M565_DMA2D,   _Index2ColorBulk_M565_DMA2D);   	// Set up custom bulk color conversion using DMA2D for RGB565 (does not speed up conversion, default method is slightly faster!)
    GUICC_M4444I_SetCustColorConv	(_Color2IndexBulk_M4444I_DMA2D, _Index2ColorBulk_M4444I_DMA2D); 	// Set up custom bulk color conversion using DMA2D for ARGB4444
    GUICC_M888_SetCustColorConv  	(_Color2IndexBulk_M888_DMA2D,   _Index2ColorBulk_M888_DMA2D);   	// Set up custom bulk color conversion using DMA2D for RGB888
    GUICC_M8888I_SetCustColorConv	(_Color2IndexBulk_M8888I_DMA2D, _Index2ColorBulk_M8888I_DMA2D); 	// Set up custom bulk color conversion using DMA2D for ARGB8888
    GUI_SetFuncAlphaBlending		(_DMA_AlphaBlending);     											// Required to load a bitmap palette into DMA2D CLUT in case of a 8bpp indexed bitmap                                           // Set up custom alpha blending function using DMA2D
    GUI_SetFuncGetpPalConvTable		(_LCD_GetpPalConvTable);											// Set up custom function for translating a bitmap palette into index values.
    GUI_SetFuncMixColors			(_DMA_MixColors);													// Set up a custom function for mixing up single colors using DMA2D
    GUI_SetFuncMixColorsBulk		(_LCD_MixColorsBulk);												// Set up a custom function for mixing up arrays of colors using DMA2D

#if (USE_TOUCH == 1)
    GUI_TOUCH_SetOrientation(DSP_ORIENTATION);// Set orientation of touch screen
    GUI_TOUCH_Calibrate(GUI_COORD_X, 0, WIDTH  - 1, TOUCH_LEFT, TOUCH_RIGHT);// Calibrate touch screen
    GUI_TOUCH_Calibrate(GUI_COORD_Y, 0, HEIGHT - 1, TOUCH_TOP,  TOUCH_BOTTOM);
#endif
}








/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
