.text 
addiu $sp, $sp, -20
addiu $t1, $zero, 2
addiu $t2, $zero, 4
addiu $t3, $zero, 6
addiu $t4, $zero, 8
addiu $t5, $zero, 10

sw $t1, 0($sp)
sw $t2, 4($sp)
sw $t3, 8($sp)
sw $t4, 12($sp)
sw $t5, 16($sp)

sll $t1, $t1, 1
sll $t2, $t2, 2
sll $t3, $t3, 3
sll $t4, $t4, 4
sll $t5, $t5, 5

lw $t1, 0($sp)
lw $t2, 4($sp)
lw $t3, 8($sp)
lw $t4, 12($sp)
lw $t5, 16($sp)

lui $t1, 0xFFFF
lui $t2, 200
lui $t3, 1
lui $t4, 255
lui $t5, 0x00FF

addiu $t1, $zero, 10
addiu $t2, $zero, 1
addiu $t3, $zero, 2
jal StartHaveFun
andi $t3, $t3, -1
andi $t3, $t3, -1 
ori $t3, $t3, -1 
ori $t3, $t3, -1 
j EndTheFun

StartHaveFun:
beq $t1, $zero, EndLoop
sll $t3, $t3, 1
subu $t1, $t1, $t2
j StartHaveFun

EndLoop:
jr $ra

EndTheFun:
addiu $sp, $sp, 20
addi $s0, $t1, 1