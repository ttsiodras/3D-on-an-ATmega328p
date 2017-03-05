#include <Arduino.h>

// Fixed point XYZ data (8bit fractional part, i.e 256x) for a statue of Mozart
// Wow, haven't done this kind of thing for 20 years :-)
#include "statue.h"

// Fixed point lookup tables for sin/cos
#include "sincos.h"

// Please UNCOMMENT one of the constructor lines in this
// to tell U8g2 which screen is attached to your Atmega328p
#include "myscreen.h"

// In case you need to debug - Serial output comes on pin 3 of the Atmega328p
#include "myuart.h"


///////////////////////////////////////////////////////////////////////
// I LOVE GLOBAL DATA!!!
//
// No I don't :-)
// But this is a tiny microcontroller, why would I waste
// stack space, passing these lovely data as parameters?
//
// FAMOUS LAST WORDS - LET THE CHAOS BEGIN
///////////////////////////////////////////////////////////////////////

// The angle of rotation in the Z-axis. Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

// Sine and cosine of the rotation angle (in fixed point 24.8)
static long msin = 0, mcos = 0;

// Distance from the statue (in fixed point 24.8)
long Se = 25*256 + maxx;
const long Sc = 20*256 + maxx;

// Frame counter
static int frames = 0;

// Vanity and framerate report
static char msg[] = "ttsiodras likes 3D!  FPS: 00.00 ... ";
const int msgLen = sizeof(msg)-1;

// 'Scroll' text in this one
char scrolled[msgLen+1];

///////////
// Setup 
///////////

// Setup pin 6 of PORTB for output - we'll PWM a LED there,
// with the intensity set based on the viewing distance from the statue
void led_init()
{
    DDRB |= _BV(DDB6);
}


// Called once from Arduino - initialize u8g2, uart, and control a LED via pin B6
void setup(void) {
    // Setup our screen, color and font
    u8g2.begin();
    u8g2.setColorIndex(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    // Setup the serial
    uart_init();
    uart_debug("Execution starts...\r\n");
    // Setup the LED
    led_init();
}


///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoint(int pt)
{
    const int width=128;
    const int height=64;
    // Read the statue data from the .text segment.
    // Even though the microcontroller has 32KB of .text space, it only
    // has 2K of RAM! The flash that stores the code can be used as storage
    // for our constant data as well.
    long wx = (long) (int) pgm_read_word_near(&(points[pt][0]));
    long wy = (long) (int) pgm_read_word_near(&(points[pt][1]));
    long wz = (long) (int) pgm_read_word_near(&(points[pt][2]));

    // Now that we read the X,Y,Z data, project them to 2D
    long wxnew = (mcos*wx - msin*wy)/256L;
    long wynew = (msin*wx + mcos*wy)/256L;
    long x = width/2L + (15L*wynew*(Se-Sc)/(Se-wxnew))/256;
    long y = height/2L - (15L*wz*(Se-Sc)/(Se-wxnew))/256;

    // If the point is within the screen's range, plot it.
    if (y>=0 && y<height && x>=0 && x<width)
        u8g2.drawPixel((u8g2_uint_t)x, (u8g2_uint_t)y);
#ifdef UART_DEBUG_ON
    //uart_debug("%d drawn (%d,%d).\n", pt, (u8g2_uint_t)x, (u8g2_uint_t)y);
#endif
}

/////////////////////////////////////////////////////////////////////////
// Use PWM to illuminate the LED according to the distance of the statue.
/////////////////////////////////////////////////////////////////////////

void updateLED()
{
#ifdef LEDME
    static int ledOn = 0;

    ledOn = ledOn^1;
    if (ledOn)
        PORTB |= _BV(PORTB6);
    else
        PORTB &= ~_BV(PORTB6);
    analogWrite(5, 128+(sincos[angle].si/2));
#endif
}


/////////////////////////////////////////////////////////////////
// Keep track of how many frames we draw per second, and add them
// in the report we print (to serial or the screen).
//
// But not on every frame! Only every 32 frames drawn...
/////////////////////////////////////////////////////////////////

void updatePerformanceReport()
{
#ifdef PERFORMANCE_REPORT
    static unsigned long prev = 0;

    if ((frames & 31) == 31) {
        if (!prev)
            prev = millis();
        else {
            unsigned fps = 3200000/(millis()-prev);
#ifdef UART_DEBUG_ON
            uart_debug("FPS: %2d.%02d\r\n", fps/100, fps%100);
#else
            int l = sprintf(&msg[26], "%2d.%02d", fps/100, fps%100);
            msg[26+l] = ' ';
#endif
            prev = millis();
        }
    }
#endif
}


////////////////////////////////////////////////////
// Scroll the vanity and FPS message on the display.
////////////////////////////////////////////////////

void updateBanner()
{
#ifdef VANITY
    int src=frames%msgLen, dst=0;
    while(dst!=msgLen) {
        scrolled[dst++] = msg[src];
        src = (src+1) % msgLen;
    }
    scrolled[dst] = 0;
    u8g2.drawStr(0, 62, scrolled);
#endif
}


/////////////////////////////////////////
// The main loop, called by Arduino's RTL
/////////////////////////////////////////

void loop(void) {
    // Rotate by 5 degrees on each iteration (360/72)
    angle = frames%72;
    // Recompute sin/cos from the lookup table
    msin = sincos[angle].si;
    mcos = sincos[angle].co;
    // Recompute the distance of the eyepoint
    Se = 25*256 + maxx + 3*sincos[angle].si;

    u8g2.firstPage();
    do {
        updateBanner();
        for(int i=0; i<sizeof(points)/sizeof(points[0]); i++)
            drawPoint(i);
    } while(u8g2.nextPage());

    // _delay_ms(100);
    updateLED();
    updatePerformanceReport();
    frames++;
}
