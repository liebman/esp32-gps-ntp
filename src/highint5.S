#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include "freertos/xtensa_context.h"
#include "esp_private/panic_reason.h"
#include "sdkconfig.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"
#include "soc/timer_group_reg.h"

#define L5_INTR_STACK_SIZE  16
#define L5_INTR_A2_OFFSET   0
#define L5_INTR_A3_OFFSET   4
#define L5_INTR_A4_OFFSET   8
#define L5_INTR_A5_OFFSET   12
    .data
_l5_intr_stack:
    .space      L5_INTR_STACK_SIZE


#define STATUS_OFFSET   0
#define W1TC_OFFSET     8

#define LATENCY_GPIO_NUM    2
#define LATENCY_GPIO_BIT    (1<<LATENCY_GPIO_NUM)

#define PPS_DATA_SIZE   32
#define PPS_PIN_OFFSET  0  /* pin number */
#define PPS_LAST_OFFSET 4  /* timer value for last interrupt, used to computer microseconds */
#define PPS_TIME_OFFSET 8  /* time in seconds */
#define PPS_MIN         12 /* min */
#define PPS_MAX         16 /* max */
#define PPS_INTERVAL    20
#define PPS_PIN_BIT     24
#define PPS_STATUS_REG  28


    .align      4

    .global     gps_pps_data
    .type       gps_pps_data,@object
    .size       gps_pps_data,PPS_DATA_SIZE
gps_pps_data:
    .space      PPS_DATA_SIZE

    .global     rtc_pps_data
    .type       rtc_pps_data,@object
    .size       rtc_pps_data,PPS_DATA_SIZE
rtc_pps_data:
    .space      PPS_DATA_SIZE

pps_entry_end:


    .section .iram1,"ax"
    .global     xt_highint5
    .type       xt_highint5,@function
    .align      4
xt_highint5:
    /* Save A2, A3, A4, A5 so we can use those registers */
    movi    a0, _l5_intr_stack
    s32i    a2, a0, L5_INTR_A2_OFFSET
    s32i    a3, a0, L5_INTR_A3_OFFSET
    s32i    a4, a0, L5_INTR_A4_OFFSET
    s32i    a5, a0, L5_INTR_A5_OFFSET

    /* set the latency pin */
    movi    a2, GPIO_OUT_W1TS_REG
    movi    a0, LATENCY_GPIO_BIT
    s32i    a0, a2, 0

    /* load timer value into a0 */
    /* trigger timer update */
    movi    a2, TIMG_T0UPDATE_REG(0)
    s32i    a0, a2, 0
    /* read timer value lower 32 bits */
    movi    a2, TIMG_T0LO_REG(0)
    l32i    a0, a2, 0

    movi    a2, gps_pps_data /* TODO: use more generic label */

check_intr_status:

    /* generate pin mask in a3 & intr status reg in a5 */
    movi    a5, GPIO_STATUS_REG
    l32i    a3, a2, PPS_PIN_OFFSET
    beqz    a3, next_pin /* if the pin number is zero then skip it */
    movi    a4, 32
    bgeu    a4, a3, make_intr_bit
    sub     a3, a3, a4
    movi    a5, GPIO_STATUS1_REG

make_intr_bit:
    ssl     a3
    movi    a3, 1
    sll     a3, a3

    /* TODO: remove this debug hack */
    s32i    a3, a2, PPS_PIN_BIT
    s32i    a5, a2, PPS_STATUS_REG

    /* clear interrupt bit */
    s32i    a3, a5, W1TC_OFFSET

#if 0
    /* use if double interrupts? */
    l32i    a2, a5, STATUS_OFFSET
#endif

    /* store last value */
    l32i    a3, a2, PPS_LAST_OFFSET /* save previous value */
    s32i    a0, a2, PPS_LAST_OFFSET /* store new value */
    sub     a3, a0, a3              /* compute interval */

    /* increment time */
    l32i    a4, a2, PPS_TIME_OFFSET
    addi    a0, a4, 1
    s32i    a0, a2, PPS_TIME_OFFSET

    /* if this is teh first second, previous value was 0 then don't compute stats */
    blti    a4, 12, next_pin

    /* save the last interval */
    s32i    a3, a2, PPS_INTERVAL

check_max:
    l32i    a0, a2, PPS_MAX
    blt     a3, a0, check_min
    s32i    a3, a2, PPS_MAX

check_min:
    l32i    a0, a2, PPS_MIN
    beqz    a0, set_min /* if its zero then just store the first value */
    blt     a0, a3, next_pin
set_min:
    s32i    a3, a2, PPS_MIN

next_pin:
    addi    a2, a2, PPS_DATA_SIZE /* increment to the next pin */
    /* check a2 for being at or past pps_entry_end and exit */
    movi    a3, pps_entry_end
    blt     a2, a3, check_intr_status

exit_interrupt:
#if 0
    /* Clear interupt on GPIO26 */
    l32i    a3, a2, PPS_PIN_OFFSET
    ssl     a3
    movi    a3, 1
    sll     a3, a3

    movi    a2, GPIO_STATUS_W1TC_REG
    s32i    a3, a2, 0
#endif

    /* clear the latency pin */
    movi    a2, GPIO_OUT_W1TC_REG
    movi    a0, LATENCY_GPIO_BIT
    s32i    a0, a2, 0

    /* Done. Restore registers and return. */
    movi    a0, _l5_intr_stack
    l32i    a2, a0, L5_INTR_A2_OFFSET
    l32i    a3, a0, L5_INTR_A3_OFFSET
    l32i    a4, a0, L5_INTR_A4_OFFSET
    l32i    a5, a0, L5_INTR_A5_OFFSET
    rsync                                   /* ensure register restored */

    rsr     a0, EXCSAVE_5 // restore a0
    rfi     5

/* The linker has no reason to link in this file; all symbols it exports are already defined
   (weakly!) in the default int handler. Define a symbol here so we can use it to have the
   linker inspect this anyway. */

    .global ld_include_xt_highint5
ld_include_xt_highint5:
