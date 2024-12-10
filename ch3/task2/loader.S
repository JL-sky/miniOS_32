%include "boot.inc"
 
SECTION vstart=LOADER_BASE_ADDR
;------------------------------------------
;将要显示的字符串写入到显存中
    mov byte [gs:0x00],'2';在第一个字节的位置写入要显示的字符“1”
    ;在第二个字节的位置写入显示字符（也就是字符1）的属性，其中A表示绿色背景，4表示前景色为红色
    mov byte [gs:0x01],0xA4
 
    mov byte [gs:0x02],' '
    mov byte [gs:0x03],0xA4
    
    mov byte [gs:0x04],'L'
    mov byte [gs:0x05],0xA4
    
    mov byte [gs:0x06],'O'
    mov byte [gs:0x07],0xA4
 
    mov byte [gs:0x08],'A'
    mov byte [gs:0x09],0xA4
 
    mov byte [gs:0x0a],'D'
    mov byte [gs:0x0b],0xA4
 
    mov byte [gs:0x0c],'E'
    mov byte [gs:0x0d],0xA4
 
    mov byte [gs:0x0e],'R'
    mov byte [gs:0x0f],0xA4
 
    jmp $
