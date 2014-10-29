# definition
#   combination(n,0) = 1
#   combination(n,n) = 1
#   combination(n,r) = combination(n-1,r) + combination(n-1,r-1);

# C-code 
#  int combination(int n, int r)
#   {
#       if (r==0 || n==r) {
#           return(1);
#       } else {
#           return(combination(n-1,r) + combination(n-1,r-1));
#       }
#   }

# ABI
# Incoming parameters
# Return Address
# Callee Save Register
# Local Vars
# Spilled Registers
# Caller Save Registers
# Outgoing parameters

    .text
    .globl main

main:
    lw $a0, n
    lw $a1, r
    jal combination

    # print
    move $a0, $v0
    li $v0, 1
    syscall
    
    # exit
    li $v0, 10
    syscall

combination:
    # use callee-save
    addi $sp, $sp, -4           # save n
    sw $a0, 0($sp)
    addi $sp, $sp, -4           # save r
    sw $a1, 0($sp)
    addi $sp, $sp, -4
    sw $ra, 0($sp)

    lw $t0, 8($sp)              # n
    lw $t1, 4($sp)              # r
    beq $t1, $zero, return_1
    beq $t1, $t0, return_1

    addi $a0, $t0, -1           # $a0 = n-1
    move $a1, $t1               # $a1 = r
    jal combination
    addi $sp, $sp, -4           # caller-save v0
    sw $v0, 0($sp)

    lw $t0, 12($sp)             # n
    lw $t1, 8($sp)              # r
    addi $a0, $t0, -1           # $a0 = n-1
    addi $a1, $t1, -1           # $a1 = r-1
    jal combination

    move $t1, $v0               
    lw $t2, 0($sp)              # caller-load v0
    addi $sp, $sp, 4
    add $v0, $t1, $t2           # add them
    
    j return

    return_1:
        li $v0, 1
        j return

    return:
        lw $ra, 0($sp)
        addi $sp, $sp, 12       # skip a0, a1, ra

        jr $ra

    .data
n:  .word 10
r:  .word 3
