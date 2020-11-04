/*
 * Demo of I2C ULP routines
 */

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

#include "stack.S"


/* Define variables, which go into .bss section (zero-initialized data) */
	.bss

.set LATENCY_PIN, 12              // gpio pin 2 is rtc pin 12

	.global latency_flag
latency_flag:
	.long 0

	.global latency_count
latency_count:
	.long 0

	/* Code goes into .text section */
	.text
	.global entry
entry:
    //
    // init gpio 2 rtc 12 / touch 2  as output and enable it
    //
    WRITE_RTC_REG(RTC_IO_TOUCH_PAD2_REG, RTC_IO_TOUCH_PAD2_MUX_SEL_S, 1, 1)
    WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + LATENCY_PIN, 1, 1)
    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S + LATENCY_PIN, 1, 1)
	// start off
    WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + LATENCY_PIN, 1, 1)

	move r2, latency_count
	move r3, latency_flag
	move r0, 0
	st r0, r2, 0
	st r0, r3, 0

loop_off:
	ld r0,r3,0
	jumpr loop_off,1,lt


	// bit on
    WRITE_RTC_REG(RTC_GPIO_OUT_W1TS_REG, RTC_GPIO_OUT_DATA_W1TS_S + LATENCY_PIN, 1, 1)
#if 0
	// increment latency count
	ld r0, r2, 0
	add r0,r0,1
	st r0,r2,0
#endif

loop_on:
	ld r0,r3,0
	jumpr loop_on,1,ge

	// bit off
    WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + LATENCY_PIN, 1, 1)
	jump loop_off

#if 0
prev_temp:	.long 0
prev_pressure: .long 0
prev_pressure2: .long 0

	.global	counter
counter: .long 0

	.global stack
stack:
	.skip 100
	.global stackEnd
stackEnd:
	.long 0


	/* Code goes into .text section */
	.text
	.global entry
entry:
	move r3,stackEnd

	// Read the BMP-180 every 4 timer cycles:
	move r1,counter
	ld r0,r1,0
	add r0,r0,1
	st r0,r1,0 // increment counter
	and r0,r0,0x3
	jumpr waitNext,1,ge

	// GPIO2 LED ON
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + 12, 1, 1)

	psr
	jump readBMP

	// GPIO2 LED OFF
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + 12, 1, 0)

	/* wake up after significant change */
	move r1,pressure
	ld r0,r1,0
	move r1,prev_pressure
	ld r2,r1,0
	sub r0,r0,r2
	psr
	jump abs
	jumpr testTemp,6,lt
	jump wakeUp

testTemp:
	move r1,temp
	ld r0,r1,0
	move r1,prev_temp
	ld r2,r1,0
	sub r0,r0,r2
	psr
	jump abs
	jumpr waitNext,10,lt

wakeUp:
	/* save new pressure and temp */
	move r1,pressure
	ld r0,r1,0
	move r1,prev_pressure
	st r0,r1,0
	move r1,temp
	ld r0,r1,0
	move r1,prev_temp
	st r0,r1,0
	/* Wake up the SoC, end program */
	wake
	/* Stop the wakeup timer so it does not restart ULP */
	WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)

waitNext:
	halt

// Compute abs value of R0
abs:
	and r1,r0,0x8000
	jump noNegate,eq
	move r1,0
	sub r0,r1,r0
noNegate:
	ret
#endif
