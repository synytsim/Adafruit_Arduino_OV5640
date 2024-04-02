// #include "OV5640.h"
// #include <wiring_private.h>
// #include <Adafruit_ZeroDMA.h>
// #include <Arduino.h>

// static Adafruit_ZeroDMA dma;
// static DmacDescriptor *descriptor;
// static volatile bool frameReady = false;
// static volatile bool suspended = false;

// static void startFrame(void) {
//     if (!suspended) {
//         frameReady = false;
//         (void)dma.startJob();
//     }
// }

// static void dmaCallback(Adafruit_ZeroDMA *dma) { frameReady = true; }

// void OV5640::suspend(void) {
//     while (!frameReady)
//         ;

//     suspended = true;
// }

// void OV5640::resume(void) {
//     frameReady = false;
//     suspended = false;
// }

// void _SCCB16CameraBase::_SCCB16CameraBase(void) {
//     dma.setAction(DMA_TRIGGER_ACTION_BEAT);
//     dma.setTrigger(PCC_DMAC_ID_RX);
//     dma.setCallback(dmaCallback);
//     dma.setPriority(DMA_PRIORITY_3);

//     descriptor = dma.addDescriptor((void *)(&PCC->RHR.reg),
//                                    (void *)buffer,
//                                    _w * _h / 2,
//                                    DMA_BEAT_SIZE_WORD,
//                                    false,
//                                    true);

//     attachInterrupt(PIN_PCC_DEN1, startFrame, FALLING);
// }

// void OV5640::capture(void) {
//     volatile uint32_t *vsync_reg, *hsync_reg;
//     uint32_t vsync_bit, hsync_bit;

//     vsync_reg = &PORT->Group[g_APinDescription[PIN_PCC_DEN1].ulPort].IN.reg;
//     vsync_bit = 1ul << g_APinDescription[PIN_PCC_DEN1].ulPin;
//     hsync_reg = &PORT->Group[g_APinDescription[PIN_PCC_DEN2].ulPort].IN.reg;
//     hsync_bit = 1ul << g_APinDescription[PIN_PCC_DEN2].ulPin;

//     while (*vsync_reg & vsync_bit)
//         ;
//     noInterrupts();
//     while (!*vsync_reg & vsync_bit)
//         ;
//     uint16_t width = _w / 2;
//     for (uint16_t y = 0; y < _h; y++) {
//         while(*hsync_reg & hsync_bit)
//             ;
//         while(!*hsync_reg & hsync_bit)
//             ;
//         for (int x = 0; x < width; x++) {
//             while (!PCC->ISR.bit.DRDY)
//                 ;
//             *buffer++
//         }
//     }

// }