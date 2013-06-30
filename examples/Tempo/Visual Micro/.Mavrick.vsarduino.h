//Board = Arduino Uno
#define ARDUINO 103
#define __AVR_ATmega328P__
#define F_CPU 16000000L
#define __AVR__
#define __cplusplus
#define __attribute__(x)
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__
#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define prog_void
#define PGM_VOID_P int
#define NOINLINE __attribute__((noinline))

typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {}

//already defined in arduno.h
//already defined in arduno.h
void Pin2_it();
void Pin3_it();
void NRF_Rx_interrupt(void *Npointer);
void ledCycle(uint8_t repeatT, uint16_t delayT);
void ledBlink(uint8_t repeatT, uint16_t delayT);

#include "F:\TDDOWNLOAD\others\arduino-1.0.3\hardware\arduino\variants\standard\pins_arduino.h" 
#include "F:\TDDOWNLOAD\others\arduino-1.0.3\hardware\arduino\cores\arduino\arduino.h"
#include "F:\TDDOWNLOAD\others\arduino-1.0.3\libraries\NRF24E\examples\Mavrick\Mavrick.pde"
#include "F:\TDDOWNLOAD\others\arduino-1.0.3\libraries\NRF24E\examples\Mavrick\portConfig.h"
#include "F:\TDDOWNLOAD\others\arduino-1.0.3\libraries\NRF24E\examples\Mavrick\printf.h"
