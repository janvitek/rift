; include listing.inc

INCLUDELIB MSVCRTD
INCLUDELIB OLDNAMES

PUBLIC	?spillRegistersAndScanStack@@YAXXZ				; spillRegistersAndScanStack
EXTRN	?_scanStack@@YAXXZ:PROC			; _scanStack

_TEXT	SEGMENT
?spillRegistersAndScanStack@@YAXXZ PROC					; spillRegistersAndScanStack, COMDAT
$LN3:
	push	rbp
	push	rdi
	sub	rsp, 232				
	lea	rbp, QWORD PTR [rsp+32]
	mov	rdi, rsp
	mov	ecx, 58					
	mov	eax, -858993460				
	rep stosd

	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	call	?_scanStack@@YAXXZ			

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax

	lea	rsp, QWORD PTR [rbp+200]
	pop	rdi
	pop	rbp
	ret	0
?spillRegistersAndScanStack@@YAXXZ ENDP					
_TEXT	ENDS
END
