/*
 * asm_imgproc_fns.S
 * x86-64 (System V) assembly implementations for Assignment 2.
 *
 * Implemented:
 *   - Helpers: get_r, get_g, get_b, get_a, make_pixel, compute_index
 *   - Transforms (ASM): imgproc_complement, imgproc_transpose
 *   - MS3 (ASM): imgproc_ellipse, imgproc_emboss
 */

    .section .text

/* struct Image layout */
#define IMAGE_WIDTH_OFFSET    0
#define IMAGE_HEIGHT_OFFSET   4
#define IMAGE_DATA_OFFSET     8

/* ===================== Helpers ===================== */

/* uint32_t get_r(uint32_t pixel) */
    .globl get_r
get_r:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $24, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

/* uint32_t get_g(uint32_t pixel) */
    .globl get_g
get_g:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $16, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

/* uint32_t get_b(uint32_t pixel) */
    .globl get_b
get_b:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    shrl    $8, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

/* uint32_t get_a(uint32_t pixel) */
    .globl get_a
get_a:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    %edi, %eax
    andl    $0xFF, %eax
    popq    %rbp
    ret

/* uint32_t make_pixel(uint32_t r, uint32_t g, uint32_t b, uint32_t a) */
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

/* int32_t compute_index(struct Image *img, int32_t row, int32_t col) */
    .globl compute_index
compute_index:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    IMAGE_WIDTH_OFFSET(%rdi), %eax
    imull   %esi, %eax
    addl    %edx, %eax
    popq    %rbp
    ret

/* ===================== imgproc_complement ===================== */
/* void imgproc_complement(struct Image *in, struct Image *out) */
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

/* ===================== imgproc_transpose ===================== */
/* int imgproc_transpose(struct Image *in, struct Image *out) */
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

/* ===================== imgproc_ellipse (ASM, MS3) ===================== */
/* void imgproc_ellipse(struct Image *in, struct Image *out) */
    .globl imgproc_ellipse
imgproc_ellipse:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    movl    IMAGE_WIDTH_OFFSET(%rdi),  %r8d      /* width  */
    movl    IMAGE_HEIGHT_OFFSET(%rdi), %r9d      /* height */
    movq    IMAGE_DATA_OFFSET(%rdi), %r12        /* in->data  */
    movq    IMAGE_DATA_OFFSET(%rsi), %r13        /* out->data */

    movl    %r8d, %r10d                          /* a = w/2 */
    shrl    $1, %r10d
    movl    %r9d, %r11d                          /* b = h/2 */
    shrl    $1, %r11d

    /* a^2 -> rsi, b^2 -> rdi (as 64-bit values with zeroed high) */
    movl    %r10d, %eax
    imull   %r10d, %eax
    movl    %eax, %esi

    movl    %r11d, %eax
    imull   %r11d, %eax
    movl    %eax, %edi

    xorl    %r14d, %r14d                         /* row = 0 */
.Le_outer:
    cmpl    %r9d, %r14d
    jge     .Le_done

    xorl    %r15d, %r15d                         /* col = 0 */
.Le_inner:
    cmpl    %r8d, %r15d
    jge     .Le_next_row

    /* idx = row*width + col -> edx */
    movl    %r14d, %edx
    imull   %r8d, %edx
    addl    %r15d, %edx

    /* x = col - a -> ecx; y = row - b -> ebx */
    movl    %r15d, %ecx
    subl    %r10d, %ecx
    movl    %r14d, %ebx
    subl    %r11d, %ebx

    /* degenerate cases */
    cmpl    $0, %r10d
    je      .Le_a_zero
    cmpl    $0, %r11d
    je      .Le_b_zero

    /* termX = (10000*x*x) / (a^2) */
    movslq  %ecx, %rax
    imulq   %rax, %rax
    movq    $10000, %rcx
    imulq   %rcx, %rax
    xorq    %rdx, %rdx
    movq    %rsi, %rcx
    divq    %rcx
    movq    %rax, %rbx     /* rbx = termX */

    /* termY = (10000*y*y) / (b^2) */
    movslq  %ebx, %rax
    imulq   %rax, %rax
    movq    $10000, %rcx
    imulq   %rcx, %rax
    xorq    %rdx, %rdx
    movq    %rdi, %rcx
    divq    %rcx

    addq    %rbx, %rax
    cmpq    $10000, %rax
    jg      .Le_outside

.Le_inside:
    movl    (%r12, %rdx, 4), %eax
    movl    %eax, (%r13, %rdx, 4)
    jmp     .Le_advance

.Le_outside:
    movl    $0x000000FF, (%r13, %rdx, 4)
    jmp     .Le_advance

/* a==0: require x==0 and termY<=10000 */
.Le_a_zero:
    cmpl    $0, %ecx
    jne     .Le_outside
    movslq  %ebx, %rax
    imulq   %rax, %rax
    movq    $10000, %rcx
    imulq   %rcx, %rax
    xorq    %rdx, %rdx
    movq    %rdi, %rcx
    divq    %rcx
    cmpq    $10000, %rax
    jg      .Le_outside
    movl    (%r12, %rdx, 4), %eax
    movl    %eax, (%r13, %rdx, 4)
    jmp     .Le_advance

/* b==0: require y==0 and termX<=10000 */
.Le_b_zero:
    cmpl    $0, %ebx
    jne     .Le_outside
    movslq  %ecx, %rax
    imulq   %rax, %rax
    movq    $10000, %rcx
    imulq   %rcx, %rax
    xorq    %rdx, %rdx
    movq    %rsi, %rcx
    divq    %rcx
    cmpq    $10000, %rax
    jg      .Le_outside
    movl    (%r12, %rdx, 4), %eax
    movl    %eax, (%r13, %rdx, 4)
.Le_advance:
    incl    %r15d
    jmp     .Le_inner

.Le_next_row:
    incl    %r14d
    jmp     .Le_outer

.Le_done:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

/* ===================== imgproc_emboss (ASM, MS3) ===================== */
/* void imgproc_emboss(struct Image *in, struct Image *out) */
    .globl imgproc_emboss
imgproc_emboss:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    movl    IMAGE_WIDTH_OFFSET(%rdi),  %r8d      /* width  */
    movl    IMAGE_HEIGHT_OFFSET(%rdi), %r9d      /* height */
    movq    IMAGE_DATA_OFFSET(%rdi), %r12        /* in->data  */
    movq    IMAGE_DATA_OFFSET(%rsi), %r13        /* out->data */

    xorl    %r14d, %r14d                          /* row = 0 */
.Lem_outer:
    cmpl    %r9d, %r14d
    jge     .Lem_done

    xorl    %r15d, %r15d                          /* col = 0 */
.Lem_inner:
    cmpl    %r8d, %r15d
    jge     .Lem_next_row

    /* idx = row*width + col -> we'll recompute at store, so don't keep it live */
    movl    (%r12, %r14, 4) , %eax    /* dummy to keep assembler calm; replaced below */
    /* Proper index for load: */
    movl    %r14d, %edx
    imull   %r8d, %edx
    addl    %r15d, %edx

    /* p and alpha */
    movl    (%r12, %rdx, 4), %ebx                 /* ebx = p */
    movl    %ebx, %r11d
    andl    $0xFF, %r11d                          /* r11d = alpha */

    /* Edge: top row or left col */
    testl   %r14d, %r14d
    je      .Lem_edge
    testl   %r15d, %r15d
    je      .Lem_edge

    /* neighbor nidx = (row-1)*width + (col-1) -> ecx */
    movl    %r14d, %ecx
    decl    %ecx
    imull   %r8d, %ecx
    addl    %r15d, %ecx
    decl    %ecx

    movl    (%r12, %rcx, 4), %esi                 /* esi = pn */

    /* Extract r,g,b from p into r10d (r), r9d (g), r8d (b) */
    movl    %ebx, %r10d
    shrl    $24, %r10d
    andl    $0xFF, %r10d
    movl    %ebx, %r9d
    shrl    $16, %r9d
    andl    $0xFF, %r9d
    movl    %ebx, %r8d
    shrl    $8,  %r8d
    andl    $0xFF, %r8d

    /* Extract rn,gn,bn from pn into edi (rn), ecx (gn), ebx (bn) */
    movl    %esi, %edi
    shrl    $24, %edi
    andl    $0xFF, %edi
    movl    %esi, %ecx
    shrl    $16, %ecx
    andl    $0xFF, %ecx
    movl    %esi, %ebx
    shrl    $8,  %ebx
    andl    $0xFF, %ebx

    /* diffs: dr -> edx, dg -> esi, db -> eax */
    movl    %edi, %edx
    subl    %r10d, %edx
    movl    %ecx, %esi
    subl    %r9d,  %esi
    movl    %ebx, %eax
    subl    %r8d,  %eax

    /* abs: adr -> r10d, adg -> r9d, adb -> r8d (reuse rgb regs—they're no longer needed) */
    movl    %edx, %r10d
    cmpl    $0, %r10d
    jge     .Ladr_ok
    negl    %r10d
.Ladr_ok:
    movl    %esi, %r9d
    cmpl    $0, %r9d
    jge     .Ladg_ok
    negl    %r9d
.Ladg_ok:
    movl    %eax, %r8d
    cmpl    $0, %r8d
    jge     .Ladb_ok
    negl    %r8d
.Ladb_ok:

    /* choose diff with largest abs (ties R > G > B) -> diff in edi */
    movl    %edx, %edi                 /* assume dr */
    cmpl    %r9d, %r10d                /* adr ? adg */
    jl      .Ltry_green
    cmpl    %r8d, %r10d                /* adr ? adb */
    jl      .Luse_blue
    jmp     .Lgot_diff
.Ltry_green:
    cmpl    %r8d, %r9d
    jge     .Luse_green
.Luse_blue:
    movl    %eax, %edi                 /* db */
    jmp     .Lgot_diff
.Luse_green:
    movl    %esi, %edi                 /* dg */
.Lgot_diff:

    /* gray = clamp(128 + diff) -> ecx */
    movl    %edi, %ecx
    addl    $128, %ecx
    cmpl    $0, %ecx
    jge     .Lgray_hi
    xorl    %ecx, %ecx
    jmp     .Lgray_ready
.Lgray_hi:
    cmpl    $255, %ecx
    jle     .Lgray_ready
    movl    $255, %ecx
.Lgray_ready:

    /* build pixel (gray,gray,gray,alpha=r11d) -> ebx */
    andl    $0xFF, %ecx
    movl    %ecx, %ebx
    shll    $24, %ebx
    movl    %ecx, %edi
    shll    $16, %edi
    orl     %edi, %ebx
    movl    %ecx, %edi
    shll    $8,  %edi
    orl     %edi, %ebx
    andl    $0xFF, %r11d
    orl     %r11d, %ebx

    /* recompute idx = row*width + col -> eax for store */
    movl    %r14d, %eax
    imull   %r8d, %eax
    addl    %r15d, %eax
    movl    %ebx, (%r13, %rax, 4)
    jmp     .Lem_advance

.Lem_edge:
    movl    $0x80808000, %ebx
    andl    $0xFF, %r11d
    orl     %r11d, %ebx
    /* idx = row*width + col -> eax */
    movl    %r14d, %eax
    imull   %r8d, %eax
    addl    %r15d, %eax
    movl    %ebx, (%r13, %rax, 4)

.Lem_advance:
    incl    %r15d
    jmp     .Lem_inner

.Lem_next_row:
    incl    %r14d
    jmp     .Lem_outer

.Lem_done:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret
