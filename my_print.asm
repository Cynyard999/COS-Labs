section .data
    red: db 1Bh,"[31m"
    red_size: equ $-red
    defalut: db 1Bh,"[0m"
    defalut_size: equ $-defalut


section .text
global print_in_red
global print_in_default


print_in_red:
; 设置为打印红色
    mov eax,4
	mov ebx,1
	mov ecx,red
	mov edx,red_size
	int 80h
; 开始打印
    mov eax,4
	mov ebx,1
	mov ecx,[esp+4]
	mov edx,[esp+8]
	int 80h
; 恢复为默认
    mov eax,4
	mov ebx,1
	mov ecx,defalut
	mov edx,defalut_size
	int 80h
    ret

print_in_default:
	mov eax,4
	mov ebx,1
	mov ecx,[esp+4]
	mov edx,[esp+8]
	int 80h
	ret
    


