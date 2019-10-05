# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

		.text
		addiu	$a0,$0,3
		addiu	$a1,$0,2
		jal	Test
		addi	$0,$0,0 #unsupported instruction, terminate

Test:
		addiu	$v0,$0,0
		addiu	$v1,$0,5
		addiu	$a2,$0,5

Loop:
		beq	$a0,$s0,Done
		addu	$v0,$v0,$a1
		addu	$v1,$v1,$a0
		addu	$t1,$v0,$v1
		addiu	$a0,$a0,1
		j Loop
Done:	
		jr	$ra
