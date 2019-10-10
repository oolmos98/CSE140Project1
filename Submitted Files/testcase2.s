.text

addiu $sp, $sp, -8

addiu $t1, $zero, 3
addiu $t2, $zero, 5

sw $t2, 0($sp)
sw $t1, -4($sp)

jal BeginTest

lw $t1, -4($sp)
lw $t2, 0($sp)
j End

BeginTest:
addiu $t1, $zero, 1
addiu $t2, $zero, 3

Loop:
beq $t1, $t2, TestCase
addiu $t1, $t1, 1
j Loop

TestCase:
jr $ra

End:
addiu $sp, $sp, 8
addi $t3, $zero, 100