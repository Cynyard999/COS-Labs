section .data
    notice: db 'Please input x and y:', 0Ah
    minus_sign: db 2Dh
    number10: db 10
    next_line: db 0Ah

section .bss
    temp_char: resb 1
    bits_count_one: resb 4
    bits_count_two: resb 4
    first_num: resb 21
    first_num_negative: resb 1
    second_num: resb 21
    second_num_negative: resb 1
    add_carry: resb 1
    sub_carry: resb 1
    cmp_pointer: resb 4
    abs_bigger_num: resb 1
    add_res: resb 22
    sub_res: resb 22
    sub_res_sign_positive: resb 1
    sub_counter: resb 1
    mul_res: resb 42
    has_zero: resb 1
    outer_loop_count: resb 4

section .text
global _start

_start:
    call init
    call notice_print
    call get_first_num
    call get_second_num
    call do_add
    call do_mul
    call end

init:
    mov dword[bits_count_one], 0
    mov dword[bits_count_two], 0
    mov byte[first_num_negative], 0
    mov byte[second_num_negative], 0
    mov byte[has_zero], 0
    mov byte[abs_bigger_num], 0
    mov byte[sub_res_sign_positive], 0
    mov dword[cmp_pointer], 0
    mov byte[add_carry], 0
    mov byte[sub_carry], 0
    mov byte[sub_counter], 0
    mov dword[outer_loop_count], 0
    ret


notice_print: 
    mov edx, 22     
    mov ecx, notice
    mov ebx, 1      
    mov eax, 4      
    int 80h
    ret
; 存放数字的时候不会存放正负号，方便计算
get_first_num:
    mov edx, 1
    mov ecx, temp_char
    mov ebx, 0
    mov eax, 3
    int 80h
    cmp byte[temp_char], 2Dh
    je mark_fisrt_num_negative
    cmp byte[temp_char], 20h
    je check_first_num_zero
    mov eax, first_num
    add eax, dword[bits_count_one]
    mov bl, byte[temp_char]
    sub bl,30h
    mov byte[eax], bl
    inc dword[bits_count_one]
    jmp get_first_num
mark_fisrt_num_negative:
    mov byte[first_num_negative], 1
    jmp get_first_num

check_first_num_zero:
    mov eax, dword[bits_count_one]
    ; 如果长度是1，并且第一位就是0，那就说明这个数是0
    cmp eax, 1
    jne mov_first_num
    mov al, byte[first_num]
    cmp al, 0
    jne mov_first_num
    mov byte[has_zero], 1

;将得到的数字整体往高地址移动, eg 1234000>0001234
mov_first_num:
    ; eax, ebx分别是两个地址指针
    mov eax, first_num
    add eax, 21
    mov ebx, first_num
    add ebx, dword[bits_count_one]
    mov ecx, dword[bits_count_one]
loop1:
    ; dl临时变量
    dec eax
    dec ebx
    mov dl, byte[ebx]
    mov byte[eax],dl
    mov byte[ebx],0
    dec ecx
    cmp ecx, 0
    jg loop1
    ret
    
return:
    ret


get_second_num:
    mov edx, 1
    mov ecx, temp_char
    mov ebx, 0
    mov eax, 3
    int 80h
    cmp byte[temp_char], 2Dh
    je mark_second_num_negative
    cmp byte[temp_char], 0AH
    je check_second_num_zero
    mov eax, second_num
    add eax, dword[bits_count_two]
    mov bl, byte[temp_char]
    sub bl, 30h
    mov byte[eax], bl
    inc dword[bits_count_two]
    jmp get_second_num
mark_second_num_negative:
    mov byte[second_num_negative], 1
    jmp get_second_num

check_second_num_zero:
    mov eax, dword[bits_count_two]
    ; 如果长度是1，并且第一位就是0，那就说明这个数是0
    cmp eax, 1
    jne mov_second_num
    mov al, byte[second_num]
    cmp al, 0
    jne mov_second_num
    mov byte[has_zero], 1

mov_second_num:
    mov eax, second_num
    add eax, 21
    mov ebx, second_num
    add ebx, dword[bits_count_two]
    mov ecx, dword[bits_count_two]
loop2: 
    dec eax
    dec ebx
    ; dl临时变量
    mov dl, byte[ebx]
    mov byte[eax],dl
    mov byte[ebx],0
    dec ecx
    cmp ecx, 0
    jg loop2
    ret

do_add:
    ; 如果负号相反，做减法
    mov al, byte[first_num_negative]
    xor al, byte[second_num_negative]
    cmp al, 1
    je do_sub
    mov eax, first_num
    mov ebx, second_num
    mov ecx, add_res
    add eax, 21
    add ebx, 21
    add ecx, 22
loop3:
    dec eax
    dec ebx
    dec ecx
    mov dl, byte[eax]
    add dl, byte[ebx]
    add dl, byte[add_carry]
    cmp dl, 9
    jg if
    jmp else
if: 
    mov byte[add_carry],1
    sub dl, 10
    jmp then
else: 
    mov byte[add_carry],0
then:
    mov byte[ecx],dl
    ; 是否loop了21次,加法结束
    cmp eax,first_num
    jne loop3
    ; 结束后，是否产生了最高位加后的进位
    cmp byte[add_carry],1
    jne print_add_res
    dec ecx
    mov byte[ecx], 1
add_end:
    jmp print_add_res

; 比较两个异号的数字的绝对值大小，并且更大的数的地址在eax，更小的在ebx， dl临时变量
do_cmp:
cmp_loop:
    ; 如果已经比完了，两数绝对值相同
    cmp dword[cmp_pointer], 21
    je mark_abs_equal
    mov eax, first_num
    mov ebx, second_num
    add eax, dword[cmp_pointer]
    add ebx, dword[cmp_pointer]
    inc dword[cmp_pointer]
    mov dl, byte[ebx]
    cmp byte[eax], dl
    jg mark_abs_first_num_bigger
    je cmp_loop
    jl mark_abs_second_num_bigger
mark_abs_equal:
    mov eax, first_num
    mov ebx, second_num
    mov byte[abs_bigger_num], 0
    jmp mark_sub_res_sign_positive
mark_abs_first_num_bigger:
    mov eax, first_num
    mov ebx, second_num
    mov byte[abs_bigger_num], 1
    ; 第一个数字绝对值更大，并且第一个数字是负数，结果就是负数
    cmp byte[first_num_negative], 1
    je mark_sub_res_sign_negative
    jmp mark_sub_res_sign_positive
mark_abs_second_num_bigger:
    mov eax, second_num
    mov ebx, first_num
    mov byte[abs_bigger_num], 2
    ; 第二个数字绝对值更大，并且第二个数字是负数，结果就是负数
    cmp byte[second_num_negative], 1
    je mark_sub_res_sign_negative
    jmp mark_sub_res_sign_positive

mark_sub_res_sign_positive:
    mov byte[sub_res_sign_positive], 1
    jmp sub_continue
mark_sub_res_sign_negative:
    mov byte[sub_res_sign_positive], 0
    jmp sub_continue

do_sub:
    jmp do_cmp
sub_continue:
    mov ecx, sub_res
    add eax, 21
    add ebx, 21
    add ecx, 22
sub_loop:
    dec eax
    dec ebx
    dec ecx
    mov dl, byte[eax]
    sub dl, byte[ebx]
    sub dl, byte[sub_carry]
    inc byte[sub_counter]
    cmp dl, 0
    jl sub_if
    jmp sub_else
; 如果小于0, 需要借位
sub_if: 
    mov byte[sub_carry], 1
    add dl, 10
    jmp sub_then
sub_else: 
    mov byte[sub_carry], 0
sub_then:
    mov byte[ecx],dl
    ; 是否loop了21次,减法结束
    cmp byte[sub_counter],21
    jne sub_loop
; 由于sub_res是22bytes，从后往前sub，这里把第一个byte设置为0
sub_end:
    dec ecx
    mov byte[ecx],0
    jmp print_sub_res

do_mul:
    ;inspired by https://leetcode-cn.com/problems/multiply-strings/solution/si-lu-qing-xi-by-lllllliuji-2/
    ; eax临时变量
    mov ecx, second_num
    add ecx, 21
outer_loop:
    ; 下面一行的指针ecx指向新的一个数字
    dec ecx
    ; 上面一行的指针从个数开始
    mov ebx, first_num
    add ebx, 21
    ; 每一轮res指针的起点都要往左边移动一格
    mov edx, mul_res
    add edx, 42
    sub edx, dword[outer_loop_count]
    inc dword[outer_loop_count]
    ; 判断下面那一行数是不是乘完了，乘完则乘法结束
    mov eax, dword[outer_loop_count]
    cmp eax, dword[bits_count_two]
    jg print_mul_res
inner_loop:    
    dec ebx
    dec edx 
    cmp ebx, first_num
    ; 判断上面那一行数是不是乘完了，乘完则进入outer_loop
    jl outer_loop
    ; 两个数字的某一位数相乘，结果保存在al
    mov al, byte[ebx]
    mul byte[ecx]
    ; al添加到现在edx存的指针指向的那个byte中
    ; 一个byte最多存127，这里计算出来最大的数为9*9+9（原来位上可能有9），所以能存放进去
    add al, byte[edx]
    mov byte[edx],al
    ; 存放进去后如果是两位数，那么计算进位和余数
    jmp cal_mul_carry
cal_over:
    jmp inner_loop
cal_mul_carry:
    cmp byte[edx],10
    jl cal_over
    mov ah, 0
    mov al, byte[edx]
    div byte[number10]
    ; 余数
    mov byte[edx],ah
    ; 商进位
    add al, byte[edx-1]
    mov byte[edx-1],al
    jmp cal_over

print_add_res:
    mov al, byte[first_num_negative]
    and al, byte[second_num_negative]
    ; 是否两个都是负数
    cmp al, 1
    jne move_add_res_addr
    ; 先打印一个负号
    mov edx, 1    
    mov ecx, minus_sign
    mov ebx, 1      
    mov eax, 4      
    int 80h
move_add_res_addr:
    mov ecx, add_res
    mov edx, 22
    jmp print_res

print_sub_res:
    mov al, byte[sub_res_sign_positive]
    cmp al, 1
    je move_sub_res_addr
    ; 减法后是负数，先打印一个负号
    mov edx, 1    
    mov ecx, minus_sign
    mov ebx, 1      
    mov eax, 4      
    int 80h
move_sub_res_addr:
    mov ecx, sub_res
    mov edx, 22
    jmp print_res

print_mul_res:
    ; 先打印一个空行
    mov edx, 1     
    mov ecx, next_line
    mov ebx, 1      
    mov eax, 4      
    int 80h
    ; 异或结果是1并且res不为0，需要标记先打印负号
    mov al, byte[first_num_negative]
    xor al, byte[second_num_negative]
    cmp al, 1
    ; 异或结果不为1就不用打印'-'
    jne move_mul_res_addr
    ; 如果有一个因数是0，那么res肯定是0
    mov al, byte[has_zero]
    cmp al, 1
    ; has_zero为1就不用打印'-'
    je move_mul_res_addr
    mov edx, 1
    mov ecx, minus_sign
    mov ebx, 1      
    mov eax, 4      
    int 80h
; 再赋值需要打印的地址和长度
move_mul_res_addr:
    mov ecx, mul_res
    mov edx, 42
    jmp print_res
; eax用来遍历，将数字变成对应的字符串（+30h）
; ebx用来判断是否遍历到最后一位了
; ecx保存起始地址
; edx保存有效字符串长度
print_res:
    mov eax, ecx
    mov ebx, ecx
    add ebx, edx
    dec eax
; 从低位开始，找到第一个非0的字符,然后往后每个字符一直加30h
find_first_valid_char: 
    ; 如果都找到最后一个，并且还是0，就让eax指向最后这一位然后convert为‘0’
    inc eax
    cmp eax, ebx
    je zero_res
    cmp byte[eax], 0
    je find_first_valid_char
convert_char:
    mov dl, byte[eax]
    add dl, 30h
    mov byte[eax],dl
    inc eax
    cmp eax,ebx
    jne convert_char
    jmp start_print
zero_res:
    dec eax
    mov dl, byte[eax]
    add dl, 30h
    mov byte[eax],dl
    jmp start_print
    
start_print:
    mov ebx, 1      
    mov eax, 4      
    int 80h
    ret

end:
    mov ebx, 0      ; return 0 status on exit - 'No Errors'
    mov eax, 1      ; invoke SYS_EXIT (kernel opcode 1)
    int 80h         ; request an interrupt on libc using INT 80h.