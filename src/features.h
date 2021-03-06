

//#define DRIVE_SERVO
#define DRIVE_PWM

#define WHEEL_ENCODER  // REM: Not able to compile without this

#if !(defined(DRIVE_SERVO)|| defined(DRIVE_PWM))
#error "DEPENDENCY ERROR: Need to be defined either DRIVE_SERVO or DRIVE_PWM in features.h"
#endif

#define ENABLE_SERIAL_COMMUNICATION 1     // Enable or disable serial monitor for libraries
#define ENABLE_MEMORY

#define ENABLE_DISTANCE_SENSOR            // Enable GP2Y0A21YK0F analog distance sensor, GP2Y0A21YK0F (required 2kB space)
#define ENABLE_COLOR_SENSOR               // Enable TCS34725 Color sensor (required 2kB space)
#define ENABLE_COMPASS_SENSOR             // Enable GY-511 compass + accelerometer (required 3kB space)

#define ENABLE_EXTERNAL_PORT              // Enable PCF8564 I2C port expander (required 1kB space)

#define ENABLE_INFARED                    // Enable IR Transmiter and receiver (required 1kB space)
#define ENABLE_NEOPIXEL_RING              // Enable NeoPixel Ring

#define ENABLE_OTA_UPLOAD                 // Enable or disable OTA (On the air upload via WiFi, required 531kB space)
#define ENABLE_WIFI_MONITOR               // Enable Wifi Monitor
#define ENABLE_WIFI_CLIENT
