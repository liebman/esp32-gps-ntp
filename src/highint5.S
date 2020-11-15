#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include "freertos/xtensa_context.h"
#include "esp_private/panic_reason.h"
#include "sdkconfig.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"
#include "soc/dport_reg.h"

#if 0
#define L5_INTR_STACK_SIZE  12
    .data
_l5_intr_stack:
    .space      L5_INTR_STACK_SIZE
#endif

    .section .iram1,"ax"
    .global     xt_highint5
    .type       xt_highint5,@function
    .align      4
xt_highint5:
#if 0
    /* Clear interupt on GPIO26 */
    movi    a14, GPIO_STATUS_W1TC_REG
    movi    a15, (1 << 26)
    s32i    a15, a14, 0

    /* Increment intr_cnt */
    movi    a14, intr_cnt
    l32i    a15, a14, 0
    addi    a15, a15, 1
    s32i    a15, a14, 0
    memw
#endif

    rsr     a0, EXCSAVE_5 // restore a0
    rfi     5

/* The linker has no reason to link in this file; all symbols it exports are already defined
   (weakly!) in the default int handler. Define a symbol here so we can use it to have the
   linker inspect this anyway. */

    .global ld_include_xt_highint5
ld_include_xt_highint5:
