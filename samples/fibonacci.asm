global _main
extern _printf

section .data
    format db "Fibonacci number: %d", 35, 0 ; printfで使用するフォーマット文字列（改行付き）

section .bss
    result resd 1

section .text

_main:
    push 35             
    call fibonacci
    add esp, 4          

    push eax           
    push format
    call _printf
    add esp, 8          
    mov eax, 0
    ret

fibonacci:
    push ebp
    mov ebp, esp
    sub esp, 4          

    mov eax, [ebp+8]    
    cmp eax, 1
    ja .recursion       
    jmp .finish

.recursion:
    sub eax, 1
    push eax            
    call fibonacci      
    mov [ebp-4], eax    
    mov eax, [ebp+8]
    sub eax, 2
    push eax            
    call fibonacci
    add eax, [ebp-4]   

.finish:
    mov esp, ebp
    pop ebp
    ret