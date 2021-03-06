/*
 * lib/libc/x86_64/string.S
 * 
 * Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * 
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 * 
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Low level string operations.
 */

/* All functions must be direction flag independent, and must behave well on NULL input.
 * What's more str* functions must be UTF-8 aware. */

.global memzero
.global memcpy
.global memmove
.global memset
.global memcmp
.global memchr
.global ffs
.global strcpy
.global strncpy
.global strcat
.global strncat
.global strcmp
.global strncmp
.global strchr
.global strrchr
.global strlen
.global strnlen
.global mbstrlen
.global strcasecmp
.global strncasecmp

.section .text

/**
 * Set N bytes of S to zero.
 *
 * void *memzero (void *s, size_t n);
 */
memzero:
    cld
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    movq    %rdi, %rbx
    movq    %rsi, %rcx
    xorq    %rax, %rax
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   stosq
1:  movb    %sil, %cl
    andb    $0x7, %cl
    repnz   stosb
    movq    %rbx, %rax
2:  ret

/**
 * Copy N bytes of SRC to DEST, guaranteeing
 * correct behavior for overlapping strings. Shares code with memcpy()
 *
 * void *memmove (void *dest, void *src, size_t n);
 */
memmove:
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    movq    %rsi, %rax
    addq    %rdx, %rax
    cmpq    %rdi, %rax
    jb      1f
    addq    %rdx, %rdi
    addq    %rdx, %rsi
    std
    jmp     1f
/**
 * Copy N bytes of SRC to DEST.
 *
 * void *memcpy (void *dest, void *src, size_t n);
 */
memcpy:
    cld
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
1:  movq    %rdi, %rbx
    movq    %rdx, %rcx
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   movsq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   movsb
    cld
    movq    %rbx, %rax
2:  ret

/**
 * Set N bytes of S to C.
 *
 * void *memset (void *s, int c, size_t n);
 */
memset:
    cld
    orq     %rdx, %rdx
    jz      2f
    orq     %rdi, %rdi
    jz      2f
    movq    %rdx, %rcx
    movq    %rdi, %rbx
    movb    %sil, %al
    shrq    $3, %rcx
    orq     %rcx, %rcx
    jz      1f
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    repnz   stosq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   stosb
    movq    %rbx, %rax
2:  ret

/**
 * Compare N bytes of S1 and S2.
 *
 * int memcmp (void *s1, void *s2, size_t n);
 */
memcmp:
    cld
    xorq    %rax, %rax
    orq     %rdx, %rdx
    jz      1f
    cmpq    %rsi, %rdi
    je      1f
    incb    %al
    orq     %rdi, %rdi
    jz      1f
    orq     %rsi, %rsi
    jz      1f
    decb    %al
    movq    %rdx, %rcx
    rep     cmpsb
    jz      1f
    incb    %al
1:  ret

/**
 * These are not UTF-8 safe
 *
 * void *memchr (void *s, int c, size_t n);
 */
memchr:
    movq    %rdi, %rax
    orq     %rax, %rax
    jz      2f
1:  cmpb    (%rax), %sil
    je      2f
    incq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
2:  ret

/**
 * void *memrchr (void *s, int c, size_t n);
 */
memrchr:
    orq     %rdi, %rdi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    decq    %rdx
    movq    %rdi, %rax
    addq    %rdx, %rax
1:  cmpb    (%rax), %sil
    je      1f
    decq    %rax
    decq    %rdx
    jnz     1b
2:  xorq    %rax, %rax
1:  ret

/**
 * Return the position of the first bit set in I, or 0 if none are set.
 * The least-significant bit is position 1, the most-significant 64.
 *
 * uint64_t ffs (uint64_t i);
 */
ffs:
    xorq    %rax, %rax
    orq     %rdi, %rdi
    jz      1f
    bsfq    (%rdi), %rax
    jz      1f
    incq    %rax
1:  ret

/**
 * Append SRC onto DEST. Shares code with strcpy()
 *
 * char *strcat (char *dest, char *src);
 */
strcat:
1:  cmpb    $0, (%rdi)
    jz      1f
    incq    %rdi
    jmp     1b
/**
 * Copy SRC to DEST.
 *
 * char *strcpy (char *dest, char *src);
 */
strcpy:
1:  cld
    movq    %rdi, %rbx
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    stosb
    or      %al, %al
    jnz     1b
2:  movq    %rbx, %rax
    ret

/**
 * Append no more than N characters from SRC onto DEST. Shares code with strncpy()
 *
 * char *strncat (char *dest, char *src, size_t n);
 */
strncat:
1:  cmpb    $0, (%rdi)
    jz      1f
    incq    %rdi
    jmp     1b
/**
 * Copy no more than N characters of SRC to DEST.
 *
 * char *strncpy (char *dest, char *src, size_t n);
 */
strncpy:
1:  cld
    movq    %rdi, %rbx
    orq     %rdx, %rdx
    jz      2f
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    stosb
    decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rbx, %rax
    ret

/**
 * Compare S1 and S2.
 *
 * int strcmp (char *s1, char *s2);
 */
strcmp:
    cld
    xorq    %rax, %rax
    cmpq    %rsi, %rdi
    je      2f
    incb    %al
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    movb    %al, %ah
    subb    (%rdi), %al
    incq    %rdi
    or      %al, %al
    jnz     2f
    or      %ah, %ah
    jnz     1b
2:  movsx   %al, %rax
    ret
    
/**
 * Compare N characters of S1 and S2.
 *
 * int strncmp (char *s1, char *s2, size_t n);
 */
strncmp:
    cld
    xorq    %rax, %rax
    cmpq    %rsi, %rdi
    je      2f
    incb    %al
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    movb    %al, %ah
    subb    (%rdi), %al
    incq    %rdi
    dec     %rdx
    jz      2f
    or      %al, %al
    jnz     2f
    or      %ah, %ah
    jnz     1b
2:  movsx   %al, %rax
    ret

/**
 * size_t strlen (char *s);
 */
strlen:
    cld
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

/**
 * size_t strnlen (char *s, size_t maxlen);
 */
strnlen:
    cld
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

/**
 * Return the number of multibytes (UTF-8 sequences) in S.
 * For mbstrnlen(), see mblen() in stdlib.h
 *
 * size_t mbstrlen (char *s);
 */
mbstrlen:
    cld
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    btw     $7, %ax
    jnc     3f
    incq    %rsi
    movb    %al, %ah
    shrb    $5, %al
    cmpb    $0b110, %al
    je      3f
    incq    %rsi
    movb    %ah, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    je      3f
    incq    %rsi
3:  or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

/**
 * NOTE: utf-8 multibyte safe
 *
 * char *strchr (char *s, int c);
 */
strchr:
    movq    %rdi, %rax
    orq     %rax, %rax
    jz      5f
    cmpl    $256, %esi
    jb      1f
    cmpl    $65536, %esi
    jb      2f
    movl    %esi, %ebx
    shrl    $16, %ebx
    cmpl    $65536*256, %esi
    jb      3f
4:  cmpw    (%rax), %si
    je      5f
    incq    %rax
    decq    %rdx
    jnz     4b
    xorq    %rax, %rax
5:  ret
1:  cmpb    (%rax), %sil
    je      1f
    incq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
1:  ret
2:  cmpw    (%rax), %si
    je      2f
    incq    %rax
    decq    %rdx
    jnz     2b
    xorq    %rax, %rax
2:  ret
3:  cmpw    (%rax), %si
    jne     2f
    cmpb    2(%rax), %bl
    je      3f
2:  incq    %rax
    decq    %rdx
    jnz     3b
    xorq    %rax, %rax
3:  ret

/**
 * NOTE: utf-8 multibyte safe
 *
 * char *strrchr (char *s, int c);
 */
strrchr:
    orq     %rdi, %rdi
    jz      5f
    cmpb    $0, (%rdi)
    jz      5f
    movq    %rdi, %rax
1:  incq    %rax
    cmpb    $0, (%rax)
    jnz     1b
    decq    %rax
    cmpl    $256, %esi
    jb      1f
    cmpl    $65536, %esi
    jb      2f
    movl    %esi, %ebx
    shrl    $16, %ebx
    cmpl    $65536*256, %esi
    jb      3f
4:  cmpl    (%rax), %esi
    je      4f
    decq    %rax
    decq    %rdx
    jnz     4b
5:  xorq    %rax, %rax
4:  ret
1:  cmpb    (%rax), %sil
    je      1f
    decq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
1:  ret
2:  cmpw    (%rax), %si
    je      2f
    decq    %rax
    decq    %rdx
    jnz     2b
    xorq    %rax, %rax
2:  ret
3:  cmpw    (%rax), %si
    jne     2f
    cmpb    2(%rax), %bl
    je      3f
2:  decq    %rax
    decq    %rdx
    jnz     3b
    xorq    %rax, %rax
3:  ret

/**
 * Compare S1 and S2, ignoring case.
 *
 * int strcasecmp (char *s1, char *s2);
 */
strcasecmp:
    xorq    %rdx, %rdx
    decq    %rdx
/**
 * Compare no more than N chars of S1 and S2, ignoring case.
 *
 * int strncasecmp (char *s1, char *s2, size_t n);
 */
strncasecmp:
1:  xorq    %rax, %rax
    xorq    %rbx, %rbx
    orq     %rdx, %rdx
    jz      3f
    decq    %rdx
    /* get s1 character */
    movb    (%rdi), %al
    incq    %rdi
    mov     %al, %bl
    btw     $7, %ax
    jnc     2f
    /* 2 bytes */
    movb    %bl, %al
    shrb    $5, %al
    cmpb    $0b110, %al
    jne     4f
    andb    $0b11111, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    jmp     2f
    /* 3 bytes */
4:  movb    %bl, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    jne     4f
    andb    $0b1111, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    jmp     2f

    /* 4 bytes */
4:  movb    %bl, %al
    shrb    $3, %al
    cmpb    $0b11110, %al
    jne     5f
    andb    $0b111, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    movb    (%rdi), %al
    incq    %rdi
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl

2:  callq   tolower
    movq    %rax, %rcx
    xorq    %rax, %rax
    xorq    %rbx, %rbx
    /* get s2 character */
    lodsb
    mov     %al, %bl
    btw     $7, %ax
    jnc     2f
    /* 2 bytes */
    movb    %bl, %al
    shrb    $5, %al
    cmpb    $0b110, %al
    jne     4f
    andb    $0b11111, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    jmp     2f
    /* 3 bytes */
4:  movb    %bl, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    jne     4f
    andb    $0b1111, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    jmp     2f

    /* 4 bytes */
4:  movb    %bl, %al
    shrb    $3, %al
    cmpb    $0b11110, %al
    jne      5f
    andb    $0b111, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl
    lodsb
    andb    $0b111111, %al
    shll    $6, %ebx
    addb    %al, %bl

2:  callq   tolower
    subl    %ecx, %eax
    orl     %eax, %eax
    jnz     1b
3:  ret
    /* bad UTF-8 encoding */
5:  mov     $1, %al
    ret

/* IN: ebx: char, OUT: eax: lower char, must save rcx, rdx */
tolower:
    movl    %ebx, %eax
    /* 'A' - 'Z' */
    cmpl    $0x61, %ebx
    jb      1f
    cmpl    $0x7a, %ebx
    ja      2f
    subb    $0x20, %al
    jmp     1f
    /* < 128 */
2:  cmpl    $128, %ebx
    jb      1f
    /* 0xC0 - 0xDE */
    cmpl    $0xC0, %ebx
    jb      1f
    cmpl    $0x7a, %ebx
    ja      2f
    addl    $0x20, %eax
    jmp     1f
2:  /* 0x100 - 0x137 */
    cmpl    $0x100, %ebx
    jb      1f
    cmpl    $0x137, %ebx
    ja      2f
    btw     $0, %ax
    jc      1f
    incl    %eax
    jmp     1f
2:  /* 0x139 - 0x148 */
    cmpl    $0x139, %ebx
    jb      1f
    cmpl    $0x148, %ebx
    ja      2f
    btw     $0, %ax
    jnc     1f
    incl    %eax
    jmp     1f
2:  /* 0x14A - 0x177 */
    cmpl    $0x14A, %ebx
    jb      1f
    cmpl    $0x17E, %ebx
    ja      2f
    btw     $0, %ax
    jc      1f
    incl    %eax
    jmp     1f
2:  /* 0x182 - 0x185 */
    cmpl    $0x182, %ebx
    jb      1f
    cmpl    $0x185, %ebx
    ja      2f
    btw     $0, %ax
    jc      1f
    incl    %eax
    jmp     1f
2:  /* 0x1A0 - 0x1A5 */
    cmpl    $0x1A0, %ebx
    jb      1f
    cmpl    $0x1A5, %ebx
    ja      2f
    btw     $0, %ax
    jnc     1f
    incl    %eax
    jmp     1f
2:
    /* use translate table */
    movq    translatelower@GOTPCREL(%rip), %r8
2:  cmpl    $0, (%r8)
    jz      1f
    cmpl    %eax, (%r8)
    je      2f
    addq    $8, %r8
    jmp     2b
2:  movl    4(%r8), %eax
1:  ret
    
