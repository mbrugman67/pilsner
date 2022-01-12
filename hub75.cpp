/********************************************************
 * hub75.cpp
 ********************************************************
 * Handle the LED matrix display.
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include "hub75.h"


#define SETUP_LED_OUTPUT(x)     { gpio_init(x); gpio_set_dir(x, GPIO_OUT); gpio_put(x, false); }

void pixel::makePixel(uint8_t r, uint8_t g, uint8_t b)
{
    red = r & DEPTH_MASK;
    grn = g & DEPTH_MASK;
    blu = b & DEPTH_MASK;
}

void pixel::pixelFromBuff(pixel_buff_t p)
{
    red =(uint8_t)(p & RED_MASK);
    grn = (uint8_t)((p & GRN_MASK) >> COLOR_DEPTH);
    blu = (uint8_t)((p & BLU_MASK) >> (COLOR_DEPTH * 2));
}

pixel_buff_t pixel::pixelToBuf()
{
    pixel_buff_t pxl = 0;
    pxl |= (((uint16_t)(red)) & RED_MASK);
    pxl |= ((((uint16_t)(grn)) >> COLOR_DEPTH) & GRN_MASK);
    pxl |= ((((uint16_t)(blu)) >> (COLOR_DEPTH * 2)) & BLU_MASK);
    
    return (pxl);
}

void pixel::setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    red = r & DEPTH_MASK;
    grn = g & DEPTH_MASK;
    blu = b & DEPTH_MASK;
}

void hub75::init()
{
    SETUP_LED_OUTPUT(LED_R0);
    SETUP_LED_OUTPUT(LED_G0);
    SETUP_LED_OUTPUT(LED_B0);
    SETUP_LED_OUTPUT(LED_R1);
    SETUP_LED_OUTPUT(LED_G1);
    SETUP_LED_OUTPUT(LED_B1);

    SETUP_LED_OUTPUT(LED_CLK);
    SETUP_LED_OUTPUT(LED_LATCH);
    SETUP_LED_OUTPUT(LED_OE)

    log = logger::getInstance();
    data = nvm::getInstance();

    this->clear();
    currentCol = 0;
    currentRow = 0;
}

void hub75::clear()
{
    std::memset(frameBuffer, 0, FRAME_BUFFER_SIZE);
    pixel pxl;
    pxl.makePixel(7,0,0);
    this->fillColor(pxl);
}

void hub75::fillColor(pixel& pxl)
{
    std::memset(frameBuffer, pxl.pixelToBuf(), FRAME_BUFFER_SIZE / sizeof(pixel_buff_t));
}

#define DLAY   100

void hub75::update()
{
    for (uint16_t thisCol = 0; thisCol < LED_COLS; ++ thisCol)
    {
        pixel pxl;

        // make sure clock is low
        gpio_put(LED_CLK, false);   

        // upper row
        pxl.pixelFromBuff(frameBuffer[currentRow][thisCol]);

        if (pxl.getRed())       gpio_put(LED_R0, true);
        else                    gpio_put(LED_R0, false);

        if (pxl.getGrn())       gpio_put(LED_G0, true);
        else                    gpio_put(LED_G0, false);

        if (pxl.getBlu())       gpio_put(LED_B0, true);
        else                    gpio_put(LED_B0, false);

        // lower row
        pxl.pixelFromBuff(frameBuffer[currentRow + (LED_ROWS / 2)][thisCol]);

        if (pxl.getRed())       gpio_put(LED_R1, true);
        else                    gpio_put(LED_R1, false);

        if (pxl.getGrn())       gpio_put(LED_G1, true);
        else                    gpio_put(LED_G1, false);

        if (pxl.getBlu())       gpio_put(LED_B1, true);
        else                    gpio_put(LED_B1, false);

        // assert the clock and delay
        gpio_put(LED_CLK, true);
        sleep_us(DLAY);
    }
    
    // final drop of clock + delay
    gpio_put(LED_CLK, false);
    sleep_us(DLAY);

    // set the address lines
    gpio_put(LED_A0, (currentRow & 0x01));
    gpio_put(LED_A1, (currentRow & 0x02));
    gpio_put(LED_A2, (currentRow & 0x04));

    // blank the display
    gpio_put(LED_OE, true);
    sleep_us(DLAY);

    // latch the data (from shift register to actual output buffer on board)
    gpio_put(LED_LATCH, true);
    sleep_us(DLAY);

    // allow display to display the new line
    gpio_put(LED_LATCH, false);
    gpio_put(LED_OE, false);

    // increment row for next update
    ++currentRow;
    currentRow %= (LED_ROWS / 2);
}