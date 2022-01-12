/********************************************************
 * hub75.h
 ********************************************************
 * Handle the LED matrix display.
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "project.h"
#include "./ipc/mlogger.h"
#include "./utils/stringFormat.h"
#include "./sys/nvm.h"
#include "./utils/bmFonts.h"

// pixels are stuffed into a 16-bit integer
typedef uint16_t pixel_buff_t;

#define LED_ROWS            16                          // rows in the display
#define LED_COLS            32                          // columns in each row
#define COLOR_DEPTH         4                           // color depth in bits
#define MAX_COLOR           ((1 << COLOR_DEPTH) - 1)    // maximum color value
#define DEPTH_MASK          0X0f
#define RED_MASK            0x000f
#define GRN_MASK            0x00f0
#define BLU_MASK            0x0f00
#define COLOR_MASK          (RED_MASK | GRN_MASK | BLU_MASK)
#define FRAME_BUFFER_SIZE   (size_t)(LED_ROWS * LED_COLS * sizeof(pixel_buff_t))


// the most basic pixel you can imagine
class pixel
{
public:
    pixel() {}
    ~pixel () {}

    void makePixel(uint8_t r, uint8_t g, uint8_t b);
    void pixelFromBuff(pixel_buff_t p);
    pixel_buff_t pixelToBuf();

    void setRGB(uint8_t r, uint8_t g, uint8_t b);

    void setRed(uint8_t r)              { red = (r & DEPTH_MASK); }
    void setGrn(uint8_t g)              { grn = (g & DEPTH_MASK); }
    void setBlu(uint8_t b)              { blu = (b & DEPTH_MASK); }

    uint8_t getRed()                    { return(red); }
    uint8_t getGrn()                    { return(grn); }
    uint8_t getBlu()                    { return(blu); }

private:
    uint8_t red;     
    uint8_t grn;
    uint8_t blu;
};

// Class to handle an LED Matrix using the Hub75 protocol
// in this case, it's a fixed 16 by 32 array
class hub75
{
public:
    hub75() {}
    ~hub75() {}

    void init();
    void clear();
    void fillColor(pixel& pxl);

    void update();

private:
    uint16_t currentRow;
    uint16_t currentCol;

    logger* log;
    nvm* data;

    pixel_buff_t frameBuffer[LED_ROWS][LED_COLS * sizeof(pixel_buff_t)];
};
