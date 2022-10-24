#include "xlaudio.h"
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "xlaudio_armdsp.h"

int sinelookup[64] =  {
      0,  804, 1607, 2410, 3211, 4011, 4807, 5601,
   6392, 7179, 7961, 8739, 9511,10278,11038,11792,
  12539,13278,14009,14732,15446,16150,16845,17530,
  18204,18867,19519,20159,20787,21402,22004,22594,
  23169,23731,24278,24811,25329,25831,26318,26789,
  27244,27683,28105,28510,28897,29268,29621,29955,
  30272,30571,30851,31113,31356,31580,31785,31970,
  32137,32284,32412,32520,32609,32678,32727,32757};

int rowinc[4] = { 22, 25, 27, 30};
int colinc[4] = { 39, 43, 47, 52};

int qmapsine(int f) {
   if (f < 64)
      return sinelookup[f];
   else if ((f - 64) < 64)
      return sinelookup[127 - f];
   else if ((f - 128) <  64)
      return -sinelookup[f - 128];
   else
      return -sinelookup[255 - f];
}

int outputsample(int row, int col) {
   static int rowphase = 0;
   static int colphase = 0;
   rowphase = (rowphase + rowinc[row & 3]) & 255;
   colphase = (colphase + colinc[col & 3]) & 255;

   return qmapsine(rowphase) + qmapsine(colphase);
}

int dtmfcode = 0;
typedef enum {IDLE, INC, DEC} audiostate_t;

audiostate_t next_state(audiostate_t current) {
    audiostate_t next;

    next = current;    // by default, maintain the state

    switch (current) {
    case IDLE:
        if (xlaudio_pushButtonLeftDown()) {
            next = INC;
            dtmfcode = (dtmfcode + 1) % 16;
            printf("%d\n", dtmfcode);
        }
        else if (xlaudio_pushButtonRightDown()) {
            next = DEC;
            dtmfcode = (dtmfcode + 15) % 16;
            printf("%d\n", dtmfcode);
        }
        break;

    case INC:
        if (xlaudio_pushButtonLeftUp())
            next = IDLE;
        break;

    case DEC:
        if (xlaudio_pushButtonRightUp())
            next = IDLE;
        break;

    default:
        next = IDLE;
    }

return next;
}

audiostate_t glbAudioState = IDLE;

uint16_t processSample(uint16_t x) {
    // the FSM controls the value of dtmfcode (0 .. 15)
    glbAudioState = next_state(glbAudioState);

    // the DTMF generator converts the code to a sine sample
    int q = outputsample(dtmfcode / 4, dtmfcode % 4);

    // the DTMF sample is mapped to a 500mV peak-to-peak waveform
    return (8192 + q / 16);
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    xlaudio_init_intr(FS_8000_HZ, XLAUDIO_MIC_IN, processSample);
    xlaudio_run();

    return 1;
}
