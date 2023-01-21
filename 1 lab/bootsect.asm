use16
org 0x7c00

start:
mov ax, cs
mov ds, ax
mov ss, ax
mov sp, start


mov ch, 0x01
output:
mov ax, 0x0003
int 0x10
mov ax, 0x0e

cmp ch, 0x01
je red_color
cmp ch, 0x02
je green_color
cmp ch, 0x03
je blue_color
cmp ch, 0x04
je yellow_color
cmp ch, 0x05
je white_color
cmp ch, 0x06
je gray_color

input:
mov ah, 0x00
int 0x16
cmp ah, 0x48
je plus
cmp ah, 0x50
je minus
cmp ah, 0x1c
je kernel
jmp input

plus:
add ch, 0x01
jmp check_reverse
minus:
sub ch, 0x01
jmp check_reverse

check_reverse:
cmp ch, 0x07
je up_reverse
cmp ch, 0x00
je down_reverse
jmp output

up_reverse:
mov ch, 1
jmp output
down_reverse:
mov ch, 6
jmp output

red_color:
mov bx, red
call puts
jmp input
green_color:
mov bx, green
call puts
jmp input
blue_color:
mov bx, blue
call puts
jmp input
yellow_color:
mov bx, yellow
call puts
jmp input
white_color:
mov bx, white
call puts
jmp input
gray_color:
mov bx, gray
call puts
jmp input

puts:
mov al, [bx]
test al, al
jz end_puts
mov ah, 0x0e
int 0x10
add bx, 1
jmp puts

end_puts:
ret

red:
db "Red", 0
green:
db "Green", 0
blue:
db "Blue", 0
yellow:
db "Yellow", 0
white:
db "White", 0
gray:
db "Gray", 0

kernel:
mov [0x9000], ch

mov ax, 0x1000
mov es, ax

mov bx, 0x00
mov ah, 0x02
mov dl, 1
mov dh, 0x00
mov cl, 0x01
mov ch, 0x00
mov al, 18
int 0x13


prepare:
cli
lgdt [gdt_info]

in al, 0x92
or al, 2
out 0x92, al

mov eax, cr0
or al, 1
mov cr0, eax
jmp 0x8:protected_mode

gdt_info:
dw gdt_info - gdt
dw gdt, 0

gdt:
db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

use32
protected_mode:
mov ax, 0x10
mov es, ax
mov ds, ax
mov ss, ax
call 0x10000

times (512 - ($ - start) - 2) db 0
db 0x55, 0xAA
