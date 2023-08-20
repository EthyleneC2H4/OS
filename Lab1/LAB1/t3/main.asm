SECTION .data;已初始化
message db "Please input x and y:", 0h
message_error db "Your input is illegal!", 0h
space db ' ', 0h;' '

res1: times 105 db 0;quo
res2: times 105 db 0;rem
tmp: times 105 db 0;运算时余数
count: db 0h
x: times 105 db 0h
y: times 105 db 0h;x/y

SECTION .bss;未初始化

input_string: resb 255

SECTION .text;代码段
global _start;_start为入口

quit:;终止程序
    mov ebx, 0
    mov eax, 1
    int 80h
strlen:;strlen函数 eax存储目标字符串地址
    push ebx
    mov ebx, eax
.loop:
    cmp byte[ebx], 0
    je .finish
    inc ebx
    jmp .loop
.finish:
    sub ebx, eax;根据相对地址偏移计算字符串长度
    mov eax, ebx;eax存储strlen
    pop ebx     ;ebx不受影响
    ret
;输出字符串
puts:;调用函数之前需要eax存储输出的字符串地址
    push edx
    push ecx
    push ebx
    push eax
    ;保存当前状态
    mov ecx, eax;此时eax存储message
    call strlen;调用strlen之后eax存储字符串中长度
    mov edx, eax;strlen放入edx
    mov ebx, 1;stdout
    mov eax, 4;sys_write
    int 80h;执行系统调用 write
    ;ecx:message_address
    ;edx:strlen(message)
    ;写入标准输出: eax=4 ebx=1 ecx指向写入内容 edx=写入的字节数

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
getline:;调用函数之前需要ebx存储字符串字节数，eax存储缓冲字符串地址
    push edx
    push ecx
    push ebx
    push eax

    mov edx, ebx;ebx=字符串字节数
    mov ecx, eax;缓冲字符串地址
    mov ebx, 0;stdin
    mov eax, 3;sys_read
    int 80h

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
;初始化清空

split:;将整行的字符串分割为两个数组
    push ecx
    push ebx
    push eax

    mov eax, input_string
    mov ebx, x
    mov ecx, 0
.first:
    cmp byte[eax], 20h;space
    je .second_pre
    ;if '\0' TODO

    mov cl, byte[eax]
    mov byte[ebx], cl
    inc eax
    inc ebx
    jmp .first
.second_pre:
    mov ecx, 0
    mov ebx, y
    inc eax
.second:
    cmp byte[eax], 0ah;\n
    je .finish
    mov cl, byte[eax]
    mov byte[ebx], cl
    inc eax
    inc ebx
    jmp .second

.finish:
    pop eax
    pop ebx
    pop ecx
    ret

check:;检查输入的字符串是否合法
    push edx
    push ecx
    push ebx
    push eax

    mov eax, x;检查x
    call check_x 
    cmp dh, 1
    je input_error
    cmp dh, 2
    je input_error
    cmp dh, 3
    je input_error
    cmp dh, 4
    je input_error

    xor edx, edx;dh置零

    mov eax, y;检查y
    call check_y
    cmp dh, 1
    je input_error
    cmp dh, 2
    je input_error
    cmp dh, 3
    je input_error
    cmp dh, 4
    je  input_error
    cmp dh, 5
    je input_error
.end:
    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
check_x:;1:数字不合法 2:输入非数字 3:没有空格符 4:数字超出处理范围
    push ecx
    push ebx
    push eax
    mov bl, [eax]
    cmp bl, 48
    je .error1
    mov ecx, eax
    call strlen
    cmp eax, 100
    jg .error4
    mov eax, ecx
    cmp byte[eax], 0
    je .error3
.next:
    mov bl, [eax]
    cmp byte[eax], 0
    je .finish
    cmp bl, 48;'0'
    jl .error2
    cmp bl, 57;'9'
    jg .error2
    inc eax
    jmp .next
.error1:;开头为0
    mov ecx, eax
    inc ecx
    mov cl, [ecx]
    cmp cl, 0
    je .next
    mov dh, 1
    jmp .finish    
.error2:;非数字
    mov dh, 2
    jmp .finish
.error3:;只有一个数字
    mov dh, 3
    jmp .finish
.error4:;位数过大
    mov dh, 4
.finish:
    pop eax
    pop ebx
    pop ecx
    ret
check_y:
    push ecx
    push ebx
    push eax
    mov bl, [eax]
    cmp bl, 48
    je .error1
    mov ecx, eax
    call strlen
    cmp eax, 100
    jg .error4
    mov eax, ecx
    cmp byte[eax], 0
    je .error3
.next:
    mov bl, [eax]
    cmp byte[eax], 0
    je .finish
    cmp bl, 48;'0'
    jl .error2
    cmp bl, 57;'9'
    jg .error2
    inc eax
    jmp .next
.error1:;开头为0
    mov ecx, eax
    inc ecx
    mov cl, [ecx]
    cmp cl, 0
    je .error5
    mov dh, 1
    jmp .finish    
.error2:;非数字
    mov dh, 2
    jmp .finish
.error3:;只有一个数字
    mov dh, 3
    jmp .finish
.error4:;位数过大
    mov dh, 4
    jmp .finish
.error5:;除数为零
    mov dh, 5
.finish:
    pop eax
    pop ebx
    pop ecx
    ret
check_divisor_is_zero:;判断除数是否为零
    push ecx
    push ebx
    push eax
    mov ebx, eax;ebx存储字符串
    call strlen
    cmp byte[eax], 1
    je .ifzero
    jmp .end
.ifzero:
    cmp byte[ebx], 48
    je .iszero
    jmp .end
.iszero:
    mov dh, 5
.end:
    pop eax
    pop ebx
    pop ecx
    ret
input_error:
    mov eax, message_error
    call puts
    call quit
print_res:;打印结果 eax存储quo ebx存储rem
    push ebx
    push eax

    call puts
    mov eax, space
    call puts
    mov eax, ebx
    call puts

    pop eax
    pop ebx
    ret
;===========================以上为IO函数==================================
;===========================以下为运算函数=================================
compare:;(eax>ebx)? dl=1 : dl=0
    push ecx
    push ebx
    push eax

    call strlen
    mov ecx, eax
    mov eax, ebx
    call strlen
    mov edx, eax
    ;ecx=strlen(eax) edx=strlen(ebx)
    pop eax
    push eax;恢复eax数组
    cmp ecx, edx
    je .next
    jg .a
    jl .b
.next:
    mov dh, [ebx]
    cmp dh, 0
    je .equal
    cmp [eax], dh
    jg .a
    jl .b
    inc eax
    inc ebx
    jmp .next

.equal:
    mov dl, 2
    jmp .finish
.a:
    mov dl, 1
    jmp .finish
.b:
    mov dl, 0
.finish:
    pop eax
    pop ebx
    pop ecx
    ret

reverse:;反转eax数组
    push edx
    push ecx
    push ebx
    push eax
    xor ecx, ecx;自加器 偏移量
    call strlen
    mov ebx, eax;ebx=strlen(eax)
    pop eax;eax=str_addr
.next1:
    cmp byte[eax+ecx], 0
    je .finish1
    dec ebx
    mov dl, [eax+ebx]
    mov [tmp+ecx], dl
    inc ecx
    jmp .next1
.finish1:
    xor ecx, ecx
.next2:
    cmp byte[eax+ecx], 0
    je .finish
    mov dl, [tmp+ecx]
    mov [eax+ecx], dl
    inc ecx
    jmp .next2
.finish:
    pop ebx
    pop ecx
    pop edx
    ret
mul_ten:;余数（ebx）乘10
    push edx
    push ecx
    push ebx
    push eax
    mov eax, ebx
    call reverse
    mov ebx, eax
    call strlen
.loop:
    cmp eax, 0
    je .finish
    mov dl, [eax+ebx-1]
    mov [eax+ebx], dl
    dec eax
    jmp .loop

.finish:
    mov dl, 48
    mov [ebx], dl
    mov eax, ebx
    call reverse
    mov ebx, eax

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
div_ten:;余数（ebx）除10
    push edx
    push ecx
    push ebx
    push eax

    mov eax, ebx
    call reverse
    mov ebx, eax
    call strlen
    mov ecx, ebx
.loop:
    cmp eax, 0
    je .finish
    mov dl, [ebx+1]
    mov [ebx], dl
    dec eax
    inc ebx
    jmp .loop

.finish:
    mov ebx, ecx
    mov eax, ebx
    call reverse
    mov ebx, eax
    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
_add:;大数加法 eax+ebx
    push edx
    push ecx
    push ebx
    push eax
    call reverse
    mov dl, [eax]

    add dl, 1
    mov [eax], dl
    xor ecx, ecx;偏移量
.carry:
    cmp byte[eax+ecx], 58;'9'
    jl .finish
    mov dl, [eax+ecx]
    sub dl, 10
    mov [eax+ecx], dl
    inc ecx
    mov dl, [eax+ecx]
    cmp dl, 0
    jne .continue
    add dl, 48
.continue:
    inc dl
    mov [eax+ecx], dl
    jmp .carry
.finish:
    call reverse
    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
_sub:;大数减法 eax-ebx
    push edx
    push ecx
    push ebx
    push eax

    call reverse
    mov ecx, eax
    mov eax, ebx
    call reverse
    mov ebx, eax
    mov eax, ecx
.sub:
    mov dl, [ebx]
    mov dh, [eax]
    cmp dl, 0;判断ebx是否读取结束
    je .finish
    sub dh, dl
    add dh, 48
    mov [eax], dh
    cmp dh, 48;比较当前位的大小查看是否需要结尾
    jnl .borrow_no
    mov ch, 0


.borrow1:
    mov dh, [eax]
    add dh, 10
    mov [eax], dh
    inc eax
    inc ch
    mov dh, [eax]
    dec dh
    mov [eax], dh
    cmp dh, 48
    jnl .borrow2
    jmp .borrow1

.borrow2:
    cmp ch, 0
    je .borrow_no
    dec eax
    dec ch
    jmp .borrow2
.borrow_no:
    inc eax
    inc ebx
    jmp .sub
.finish:
    pop eax
    pop ebx
    call reverse
    mov ecx, eax
    mov eax, ebx
    call reverse
    mov ebx, eax
    mov eax, ecx
    pop ecx
    pop edx
    call remove
    ret
remove:;删除前导零
    push edx
    push ecx
    push ebx
    
    mov ecx, eax
    xor ebx, ebx
    mov dl, [eax]
    cmp dl, 48;无前导零直接返回
    jne .finish
.rmv:
    mov dl, [ecx]
    cmp dl, 48
    jne .put
    inc ecx
    jmp .rmv
.put:
    mov dl, [ecx]
    cmp dl, 0
    je .pre
    mov [eax+ebx], dl
    inc ebx
    inc ecx
    jmp .put
.pre:
    mov dh, [eax+ebx]
    cmp dh, 0
    je .finish
    mov dl, 0
    mov [eax+ebx], dl
    inc ebx
    jmp .pre
.finish:
    pop ebx
    pop ecx
    pop edx
    ret
_div:;大数除法 eax=x ebx=y
;用数组存储大数和结果，运用每一位的余数进行运算，最终删除数组中的前导0，再统一输出
    push edx
    push ecx
    push ebx
    push eax
    mov dh, [count];对齐
.cnt1:
    call mul_ten
    mov dh, [count]
    inc dh
    mov [count], dh
    call compare
    cmp dl, 0
    je .next
    jmp .cnt1
.next:
    call div_ten
    mov dh, [count]
    dec dh
    mov [count], dh
.sub:
    call compare
    cmp dl, 0
    je .cnt2
    call _sub
    mov ecx, eax
    mov eax, res1
    call _add
    mov eax, ecx
    jmp .sub
.cnt2:
    mov dh, [count]
    cmp dh, 0
    je .pre
    mov ecx, ebx
    mov ebx, res1
    call mul_ten
    mov ebx, ecx
    jmp .next

.pre:
    mov ebx, eax
    mov eax, res1
    mov dh, [ebx]
    cmp dh, 0
    jne .finish
    mov dh, 48
    mov [ebx], dh
.finish:
    call print_res
    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
;========================================================================
_start:
;初始化
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx;各寄存器置零清空
;输出message
    mov eax, message
    call puts
;输入x，y字符串
    mov eax, input_string
    mov ebx, 255
    call getline
;将整行的字符串分为两个数组
    mov edx, 0
    call split
    call check
    mov eax, y
    mov ebx, x
    call compare;if x>y dl==0
    cmp dl, 1
    jne .process
    mov eax, 48;'0'
    mov ebx, x
    call print_res
    jmp .quit
.process:;x>y 执行大数除法
    mov eax, x
    mov ebx, y
    mov dl, 48
    mov [res1], dl
    call _div
.quit:
    mov ebx, 0
    mov eax, 1
    int 80h