/* simics.S - Simics interface (user side) */

.global SIM_break
.global SIM_halt
.global SIM_notify_bootup
.global SIM_puts
.global SIM_register_user_from_parent
.global SIM_register_user_proc
.global SIM_switch
.global SIM_unregister_user_proc

#define SIM_NOARG(opcode) \
    pushl %ebx; \
    movl $opcode, %ebx; \
    xchg %ebx, %ebx; \
    popl %ebx; \
    ret

#define SIM_ARG(opcode) \
    movl 4(%esp), %eax; \
    SIM_NOARG(opcode)

#define SIM_2ARG(opcode) \
    movl 8(%esp), %ecx; \
    SIM_ARG(opcode)

SIM_puts:                      SIM_ARG(  0x1BADD00D)
SIM_break:                     SIM_NOARG(0x2BADD00D)

SIM_register_user_proc:        SIM_2ARG( 0x5BADD00D)
SIM_unregister_user_proc:      SIM_ARG(  0x6BADD00D)
SIM_switch:                    SIM_ARG(  0x7BADD00D)
SIM_register_user_from_parent: SIM_2ARG(0x8BADD00D)

SIM_notify_bootup:             SIM_ARG(  0xB00710AD)

SIM_halt:                      SIM_NOARG(0xDEADD00D)

# int magic_readline(int size, char *buf)
# can't use SIM_NOARG because of that pesky "ret"
# this isn't SIM_readline() because of mystical_readline().
.global magic_readline
magic_readline:
    pushl %ebp
    movl %esp, %ebp

    pushl %ebx
    movl $0xdeadbeef, %ebx
    xchg %bx, %bx     # probably "nothing happens here"
    popl %ebx

    pushl 12(%ebp)
    pushl 8(%ebp)
    call readline     # most not mess with %eax henceforth
    addl $8, %esp

    popl %ebp
    ret

# int mystical_readline(int len, char *buf) goes here
