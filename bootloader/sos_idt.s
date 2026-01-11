# sos_idt.s - FIXED Interrupt stub handlers
.section .text
.code32

# Macro for ISRs without error code
.macro ISR_NOERRCODE num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $0          # Push dummy error code
    pushl $\num       # Push interrupt number
    jmp isr_common_stub
.endm

# Macro for ISRs with error code
.macro ISR_ERRCODE num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $\num       # Push interrupt number (note: error code already on stack)
    jmp isr_common_stub
.endm

# CPU Exception handlers (0-31)
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

# Common ISR stub
isr_common_stub:
    pusha              # Save all registers
    
    movw $0x10, %ax    # Load kernel data segment
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # FIXED: Stack layout after pusha:
    # [eax][ecx][edx][ebx][esp][ebp][esi][edi] = 32 bytes
    # [int_no][err_code] = 8 bytes (pushed before pusha)
    # Total offset to int_no = 32 bytes
    # Total offset to err_code = 36 bytes
    
    # Push arguments in CORRECT order for C: (err_code, int_no)
    movl 36(%esp), %eax   # err_code at esp+36
    pushl %eax
    movl 36(%esp), %eax   # int_no at esp+36 (note: esp changed by previous push)
    pushl %eax
    
    call isr_handler      # Call C handler with (int_no, err_code)
    
    addl $8, %esp         # Clean up pushed parameters
    
    popa                  # Restore registers
    addl $8, %esp         # Clean up error code and int number
    sti
    iret                  # Return from interrupt

# Macro for IRQ handlers
.macro IRQ num, isr_num
.global irq\num
.type irq\num, @function
irq\num:
    cli
    pushl $0           # Dummy error code
    pushl $\isr_num    # Push IRQ number (32-47)
    jmp irq_common_stub
.endm

# IRQ handlers (32-47)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40    # RTC - This is IRQ 8 mapped to interrupt 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

# Common IRQ stub - FIXED VERSION
irq_common_stub:
    pusha              # Save all registers (32 bytes)
    
    movw $0x10, %ax    # Load kernel data segment
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # FIXED: After pusha, stack layout is:
    # [eax][ecx][edx][ebx][esp][ebp][esi][edi] = 32 bytes
    # [irq_num][dummy] = 8 bytes (pushed before pusha)
    # IRQ number is at esp+32
    
    movl 32(%esp), %eax   # Get IRQ number
    pushl %eax            # Push as argument for C handler
    
    call irq_handler      # Call C handler with (int_no)
    
    addl $4, %esp         # Clean up parameter
    
    popa                  # Restore registers
    addl $8, %esp         # Clean up dummy and IRQ number
    sti
    iret                  # Return from interrupt