/********************************************************
 * project.h
 ********************************************************
 * Common definitions used throughout
 * 
 * December 2021, M.Brugman
 * 
 *******************************************************/

#ifndef PROJECT_H_
#define PROJECT_H_

#define DEBUG

// GPIO PINS
#define PIN_MISO        0   //  INTERFACE TO WIFI MODULE
#define PIN_CS          1   //  INTERFACE TO WIFI MODULE
#define PIN_CLOCK       2   //  INTERFACE TO WIFI MODULE
#define PIN_MOSI        3   //  INTERFACE TO WIFI MODULE
#define PIN_READY       4   //  INTERFACE TO WIFI MODULE
#define PIN_RESET       5   //  INTERFACE TO WIFI MODULE
#define PIN_GPIO0       6   //  INTERFACE TO WIFI MODULE
#define PIN_PIO         7   //  Pin connected to temperature probe
#define PIN_PUMP        8   //  Pin connected to pump power relay
#define PIN_IR         11   //  Pin connected to I/R receiver
#define LED_R0         16   //  Upper row red bit
#define LED_G0         17   //  Upper row green bit
#define LED_B0         18   //  Upper row blue bit
#define LED_R1         19   //  Lower row red bit
#define LED_G1         20   //  Lower row green bit
#define LED_B1         21   //  Lower row blue bit
#define LED_A0         22   //  Address line 0 (A)
#define LED_A1         26   //  Address Line 1 (B)
#define LED_A2         27   //  Address Line 3 (C)
#define LED_CLK        13   //  Bit clock
#define LED_LATCH      14   //  Row latch
#define LED_OE         15   //  Output Enable (blanking)

extern bool wakeCore1;

#endif // PROJECT_H_