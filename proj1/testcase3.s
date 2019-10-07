.text 
addiu $sp, $sp, -20($sp)
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

srl $t1, $t1, 1
srl $t2, $t2, 2
srl $t3, $t3, 3
srl $t4, $t4, 4
srl $t5, $t5, 5

