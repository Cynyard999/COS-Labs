  org 07c00h ;伪指令，汇编器使用，告诉汇编器这段代码在这个地址（绝对地址）
  mov ax, cs
  mov ds, ax
  mov es, ax
  call    DispStr ;调用DisplayString例程
  jmp $
DispStr:
  mov ax, BootMessage
  mov bp, ax
  mov cx, 16
  mov ax, 01301h
  mov bx, 000ch
  mov dl, 0
  int 10h
  ret
BootMessage:        db "Hello, OS world!"
times   510-($-$$)  db 0 ;填充空间，使得生成的二进制代码为512bytes
dw 0xaa55 ;结束