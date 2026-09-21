/* AAPCS ARMv5 soft-float cooperative context switch. MIT license.
 * Caller-saved registers are handled by the C call ABI; r4-r11/sp/lr survive.
 * No VFP registers are used by this Ndless build.
 */
.syntax unified
.arm
.text
.align 2
.global vm_arm_switch
.type vm_arm_switch,%function
vm_arm_switch:
    stmia r0!, {r4-r11}
    str sp, [r0], #4
    str lr, [r0]
    ldmia r1!, {r4-r11}
    ldr sp, [r1], #4
    ldr lr, [r1]
    bx lr
.size vm_arm_switch,.-vm_arm_switch
.global vm_arm_entry
.type vm_arm_entry,%function
vm_arm_entry:
    mov r0, r4
    blx r5
    b .
.size vm_arm_entry,.-vm_arm_entry
.section .note.GNU-stack,"",%progbits
