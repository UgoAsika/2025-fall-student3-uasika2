/*
 * asm_imgproc_fns.S
 * x86-64 (System V) assembly implementations for Assignment 2.
 *
 * Implemented here (MS2):
 *   - imgproc_complement
 *   - imgproc_transpose
 *   - get_r, get_g, get_b, get_a
 *   - make_pixel
 *   - compute_index
 *
 * Left for MS3:
 *   - imgproc_ellipse
 *   - imgproc_emboss
 */

    .section .text

#define IMAGE_WIDTH_OFFSET    0
#define IMAGE_HEIGHT_OFFSET   4
#define IMAGE_DATA_OFFSET     8

    .globl get_r
get_r:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $24, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

    .globl get_g
get_g:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $16, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

    .globl get_b
get_b:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $8, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

    .globl get_a
get_a:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

    .globl make_pixel
make_pixel:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    movl    %edi, %eax
    andl    $0xFF, %eax
    shll    $24, %eax
    movl    %esi, %ebx
    andl    $0xFF, %ebx
    shll    $16, %ebx
    orl     %ebx, %eax
    movl    %edx, %ebx
    andl    $0xFF, %ebx
    shll    $8, %ebx
    orl     %ebx, %eax
    andl    $0xFF, %ecx
    orl     %ecx, %eax
    popq    %rbx
    popq    %rbp
    ret

    .globl compute_index
compute_index:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    IMAGE_WIDTH_OFFSET(%rdi), %eax
    imull   %esi, %eax
    addl    %edx, %eax
    popq    %rbp
    ret

    .globl imgproc_complement
imgproc_complement:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    movq    IMAGE_DATA_OFFSET(%rdi), %r12
    movq    IMAGE_DATA_OFFSET(%rsi), %r13
    movl    IMAGE_WIDTH_OFFSET(%rdi),  %r14d
    imull   IMAGE_HEIGHT_OFFSET(%rdi), %r14d
    xorl    %r15d, %r15d
.Lcompl_loop:
    cmpl    %r14d, %r15d
    jge     .Lcompl_done
    movl    (%r12, %r15, 4), %ebx
    movl    %ebx, %eax
    andl    $0xFF, %eax
    notl    %ebx
    andl    $0xFFFFFF00, %ebx
    orl     %eax, %ebx
    movl    %ebx, (%r13, %r15, 4)
    incl    %r15d
    jmp     .Lcompl_loop
.Lcompl_done:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

    .globl imgproc_transpose
imgproc_transpose:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    movl    IMAGE_WIDTH_OFFSET(%rdi),  %r14d
    movl    IMAGE_HEIGHT_OFFSET(%rdi), %ecx
    cmpl    %ecx, %r14d
    jne     .Lti_not_square
    movq    IMAGE_DATA_OFFSET(%rdi), %r12
    movq    IMAGE_DATA_OFFSET(%rsi), %r13
    xorl    %r15d, %r15d
.Lti_outer:
    cmpl    %r14d, %r15d
    jge     .Lti_done
    xorl    %ebx, %ebx
.Lti_inner:
    cmpl    %r14d, %ebx
    jge     .Lti_next_row
    movl    %r15d, %edx
    imull   %r14d, %edx
    addl    %ebx, %edx
    movl    %ebx, %eax
    imull   %r14d, %eax
    addl    %r15d, %eax
    movl    (%r12, %rdx, 4), %esi
    movl    %esi, (%r13, %rax, 4)
    incl    %ebx
    jmp     .Lti_inner
.Lti_next_row:
    incl    %r15d
    jmp     .Lti_outer
.Lti_done:
    movl    $1, %eax
    jmp     .Lti_epilogue
.Lti_not_square:
    xorl    %eax, %eax
.Lti_epilogue:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

    .globl imgproc_ellipse
imgproc_ellipse:
    ret

    .globl imgproc_emboss
imgproc_emboss:
    ret
