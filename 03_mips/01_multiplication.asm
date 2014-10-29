# implement a multiplier
# and calc 32766 * 10383

    .text
    .globl main

main:
    # init
    lw $t0, lbit        # t0 is used for lowest bit check

    lw $s0, mcand       # s0 is used to store mcand
    lw $s1, mplier      # s1 is used to store mplier
    li $v0, 0           # v0 is used to store the answer

    # loop
    li $t1, 32          # loop times
    li $t2, 0           # loop counter
loop:
    and $t3, $s1, $t0   # check the lower bit of mplier
    beq $zero, $t3, skip    # if (s1 & 0x1) == 0 then skip
    add $v0, $v0, $s0
skip:
    sll $s0, $s0, 1          # shift left mcand 
    srl $s1, $s1, 1          # shift right mplier

    addi $t2, $t2, 1    # loop check
    beq $t2, $t1, loop_exit
    j loop
loop_exit:

    # print
    move $a0, $v0
    li $v0, 1           # syscall = print_int
    syscall

    # exit
    li $v0, 10          # syscall = exit
    syscall

    .data
lbit:   .word 1
mcand:  .word 32766
mplier: .word 10383
