// Timer support

// Written by Bernie Roehl, March 2017

#define JOY_LEFT   0x40
#define JOY_RIGHT  0x10
#define JOY_UP     0x08
#define JOY_DOWN   0x20
#define JOY_PRESS  0x01

void joystick_setup(void);
uint32_t joystick_read(void);  // returns one of the constants above

void pushbutton_setup(void);
uint32_t pushbutton_read(void);    // returns true if the button is pressed
