/**
 * @file 17_Senkiv.ino
 * @addtogroup IP-21
 * @paragraph Lab1
 * @author Сеньків Олександр Сергійови (you@domain.com)
 * @brief  17 var 
 * @todo
 * Кнопка	Світлодіоди	Затримка	Алгоритм
 *   PK2	  Port-C	1,2 сек.	1
 * @attention 
 *  Лінійка з 8-ми одноколірних світлодіодів. При натисканні 
 *  кнопки світлодіоди почергово блимають від 0-виводу порту до 7. 
 *  P0→P1→P2→P3→P4→P5→P6→P7
 * @version 0.2
 * @date 2024-02-25
 * 
 * @note It is a doxygen style comment 
 */

//TODO: 18var?

#define __AVR_ATmega2560__ 
#include <Arduino.h>

//#define first_led 37  //! It's convention to use uppercase letters when defining a macro
#define FIRST_LED 37
// #define last_led 30 //! Making your macros all upper-case, you can avoid collisions. 
#define LAST_LED 30


// int buttonPin = 64;
// const int buttonPin = 64;
// const uint8_t buttonPin = PIN_A10; //! in one style
#define BUTTON_PIN PIN_A10

uint8_t buttonCurrentState = 0; //! todo: make software debounce

// bool btnPressedFlag = false;      
bool btnPressedFlag;  //* it is preferred for flags to assign basic value in init function

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1 
// BITMASK_CHECK(x,y) ((x) & (y)) must be ((x) & (y)) == (y)

#define BITMASK_SET(x, mask) ((x) |= (mask))
#define BITMASK_CLEAR(x, mask) ((x) &= (~(mask)))
#define BITMASK_FLIP(x, mask) ((x) ^= (mask))
#define BITMASK_CHECK_ALL(x, mask) (!(~(x) & (mask))) //!~((~(y))|(x))
#define BITMASK_CHECK_ANY(x, mask) ((x) & (mask))

//-------------------------------------------------------
//     LSB  P0  P1  P2   P3   P4   P5,  P6,  P7  MSB
//-------------------------------------------------------
// PORTA = {22, 23, 24,  25,  26,  27,  28,  29};       
// PORTF = {A0, A1, A2,  A3,  A4,  A5,  A6,  A7};         
// PORTK = {A8, A9, A10, A11, A12, A13, A14, A15}; 
// PORTC = {37, 36, 35,  34,  33,  32,  31,  30};       
// PORTL = {49, 48, 47,  46,  45,  44,  43,  42}; 
// PORTB = {53, 52, 51,  50,  10,  11,  12,  13};
// PORTG = {41, 40, 39,  xx,  xx,  4,   xx,  xx};
// PORTD = {21, 20, 19,  18,  xx,  xx,  xx,  38};
// PORTE = {0,  1,  xx,  5,   2,   3,   xx,  xx};
// PORTH = {17, 16, xx,  6,   7,   8,   9,   xx};
// PORTJ = {15, 14, xx,  xx,  xx,  xx,  xx,  xx};     
//-------------------------------------------------------

#define PORT_PIN_0   (1 << 0) //& 0b00000001
#define PORT_PIN_1   (1 << 1) //& 0b00000010
#define PORT_PIN_2   (1 << 2) //& 0b00000100
#define PORT_PIN_3   (1 << 3) //& 0b00001000
#define PORT_PIN_4   (1 << 4) //& 0b00010000
#define PORT_PIN_5   (1 << 5) //& 0b00100000
#define PORT_PIN_6   (1 << 6) //& 0b01000000
#define PORT_PIN_7   (1 << 7) //& 0b10000000 

#define MY_PORT PORTC


void setupSerial()
{
    Serial.begin(9600);
    delay(200);
    Serial.println("");
    Serial.println("Setup begin");
}


void setupLedsPort()
{
    Serial.println("setupLedsPort begin");
//^ define one PIN on PORTC as output
    //# DDRC = 0x80; 
    // DDRC = 0b10000000;
    // DDRB |= 1 << 7
    // DDRC |= 1 << DDC7; 
    // bitSet(DDRC, 7);

    //PORTC |= _BV(PC7); // _BV gets the bit value of pin C
    //bitRead(PORTC, PC7);

//^ define all PORTC pins as outputs 
    //# DDRC = 0xFF; 
    //DDRC = 0b11111111; 
    //DDRC = 255;
    
    for (uint8_t i = 0; i <= 7; i++)
    {   
        Serial.print("set bit ");
        bitSet(DDRC, i);
        Serial.println(DDRC, DEC); //BIN
    }

//^ set 1 pin to input
    // DDRC &= ~(1 << DDC7);
//^ set HIGH
    // PORTC |= 1 << PORTC7;

//^ define all PORTC pins as outputs 
    for (uint8_t i = 0; i <= 7; i++)
    {
        // Serial.print("Led port:");
        // uint8_t printPort = portOutputRegister(digitalPinToPort(30+i));
        // Serial.println(printPort);
        // uint8_t pin_mask = digitalPinToBitMask(30+i);
        // Serial.print("Led mask:");
        // Serial.println(pin_mask);

        pinMode(30+i, OUTPUT);
        digitalWrite(30+i, LOW);
    }
    
    Serial.println("setupLedsPort end");
}

void setupBtnPort()
{   
    Serial.println("setupBtnPort begin");

    uint8_t printPort = portOutputRegister(digitalPinToPort(64));
    Serial.print("Button port:");
    Serial.println(printPort);
    
    uint8_t pin_mask = digitalPinToBitMask(64);
    Serial.print("Button mask:");
    Serial.println(pin_mask);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("setupBtnPort end");

    btnPressedFlag = digitalRead(BUTTON_PIN); //? why using reverse flag?
}

void setup()
{
    setupSerial();
    setupLedsPort();
    setupBtnPort();
    Serial.println("Setup ended");
}

void loop()
{
    //todo: make another function for reading button state. 
    if (!digitalRead(BUTTON_PIN) && !btnPressedFlag) 
    { //todo  If btnFlag is active -> it triggers function to read and store digitalRead in <buttonCurrentState> value
        Serial.println("Button triggered");
        btnPressedFlag = true;
        uint32_t time = 0;
        for (uint8_t i = FIRST_LED; i >= LAST_LED; i--) // todo:  MY_PORT |= (1 << i);
        {
            digitalWrite(i, HIGH);
            delay(240); //todo: configure delay in define 
            digitalWrite(i, LOW);

            time = time + 240;
            Serial.println(time); //? 1,2s = 1200
        }
        
    }
    else if (digitalRead(BUTTON_PIN)) //^ could be just "else"
    {
        btnPressedFlag = false;
    }
}
