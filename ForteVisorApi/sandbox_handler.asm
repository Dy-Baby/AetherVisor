extern SandboxHandler : proc

.code

PUSHAQ macro
        push    rax
        push    rcx
        push    rdx
        push    rbx
        push    -1      ; Dummy for rsp.
        push    rbp
        push    rsi
        push    rdi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
endm

POPAQ macro
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rbx    ; Dummy for rsp (this value is destroyed by the next pop).
        pop     rbx
        pop     rdx
        pop     rcx
        pop     rax
endm

sandbox_handler_wrap proc frame
	
    .endprolog

    PUSHAQ

    mov rcx, rsp    ; pass the registers
    mov rdx,  [rsp + 8 * 16 + 1]    ; pass the return address
    
    call SandboxHandler

    POPAQ

    ret
	
sandbox_handler_wrap endp

end