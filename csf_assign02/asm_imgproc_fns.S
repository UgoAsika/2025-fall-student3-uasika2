	.text

/* ============================================================
   Helper functions: extract components, build a pixel, index.
   ============================================================ */

	.globl get_r
get_r:
	/* We copy the input pixel from %edi into %eax so we can modify it safely. */
	movl  %edi, %eax
	/* We shift the red component down into the low eight bits, which is what we need. */
	shrl  $24, %eax
	/* We mask so that only the low byte remains; other bits are discarded. */
	andl  $0xFF, %eax
	/* We return the red value in %eax, as expected by the calling convension. */
	ret

	.globl get_g
get_g:
	/* We copy the pixel so we can shift without disturbing %edi. */
	movl  %edi, %eax
	/* We bring the green component into the low eight bits by shifting right. */
	shrl  $16, %eax
	/* We keep only the least-significant byte to avoid stray bits. */
	andl  $0xFF, %eax
	/* We return the green byte via %eax. */
	ret

	.globl get_b
get_b:
	/* We duplicate the pixel into %eax, because we will mutate it. */
	movl  %edi, %eax
	/* We move the blue component into position by shifting right. */
	shrl  $8,  %eax
	/* We apply a mask so the rest of the value is zeroed out properly. */
	andl  $0xFF, %eax
	/* We return the blue value; this matches the tiny helper’s intent. */
	ret

	.globl get_a
get_a:
	/* We copy the pixel into %eax and do a simple mask to get alpha. */
	movl  %edi, %eax
	/* We keep only the alpha byte; nothing fancy here at all. */
	andl  $0xFF, %eax
	/* We return the alpha in %eax; very straghtforward. */
	ret

	.globl make_pixel
make_pixel:
	/* We clamp each component to eight bits, just to be safe; callers sometimes pass wider ints. */
	andl  $0xFF, %edi             /* r &= 255 */
	andl  $0xFF, %esi             /* g &= 255 */
	andl  $0xFF, %edx             /* b &= 255 */
	andl  $0xFF, %ecx             /* a &= 255 */
	/* We shift each component into its RGBA slot before we combine them. */
	shll  $24, %edi               /* r -> bits 24..31 */
	shll  $16, %esi               /* g -> bits 16..23 */
	shll  $8,  %edx               /* b -> bits 8..15  */
	/* We assemble the pixel in %eax by OR-ing each field together. */
	movl  %edi, %eax              /* start with r */
	orl   %esi, %eax              /* add g */
	orl   %edx, %eax              /* add b */
	orl   %ecx, %eax              /* add a */
	/* We return the packed 0xRRGGBBAA value; that’s the whole point. */
	ret

	.globl compute_index
compute_index:
	/* We load img->width into %eax, since we need it for the row-major indexing. */
	movl  0(%rdi), %eax
	/* We multiply the width by the row, which gives the base for this row. */
	imull %esi, %eax
	/* We add the column to get the precise linear adress inside the image buffer. */
	addl  %edx, %eax
	/* We return that index; it’s used alot across the code. */
	ret


/* ============================================================
   Image-processing API functions. These mirror the C versions,
   but are hand-written in asm. We add clear comments so it’s
   easier to debug in gdb if something acts weird.
   ============================================================ */

	.globl imgproc_complement
imgproc_complement:
	/* We set up a normal stack frame and save %rbx, which we will use as the loop counter. */
	pushq %rbp
	movq  %rsp, %rbp
	pushq %rbx

	/* We compute the total number of pixels N = width * height. */
	movl  0(%rdi), %eax           /* %eax = in->width */
	imull 4(%rdi), %eax           /* %eax = width * height */
	/* We place N in %ecx so we can compare against it inside the loop. */
	movl  %eax, %ecx

	/* We grab the input and output buffers once to avoid reloading them every time. */
	movq  8(%rdi), %r8            /* %r8  = in->data  */
	movq  8(%rsi), %r9            /* %r9  = out->data */

	/* We iterate i from 0 to N-1, processing one pixel per iteration. */
	xorl  %ebx, %ebx              /* i = 0 */
.Lc_loop:
	/* If i >= N then we are finished and can leave the loop. */
	cmpl  %ecx, %ebx
	jge   .Lc_done

	/* We load the input pixel and produce the complemented RGB while keeping alpha. */
	movl  (%r8,%rbx,4), %eax      /* %eax = in[i] */
	movl  %eax, %edx              /* %edx = copy of pixel so we can NOT it */
	notl  %edx                    /* We invert every bit for now. */
	/* We keep only the complemented RGB bits by masking out the low 8 (alpha) bits. */
	andl  $0xFFFFFF00, %edx
	/* We also keep the original alpha from the source pixel. */
	andl  $0x000000FF, %eax
	/* We combine the preserved alpha with the complemented RGB in %edx. */
	orl   %eax, %edx
	/* We store the final pixel into the output buffer. */
	movl  %edx, (%r9,%rbx,4)

	/* We advance to the next pixel and continue looping. */
	incl  %ebx
	jmp   .Lc_loop

.Lc_done:
	/* We restore %rbx, pop the frame, and return; nothing else to clean up. */
	popq  %rbx
	leave
	ret


	.globl imgproc_transpose
imgproc_transpose:
	/* We set up a standard stack frame for clarity while debugging. */
	pushq %rbp
	movq  %rsp, %rbp

	/* We load width and height and check that this image is square. If not, we fail. */
	movl  0(%rdi), %r8d           /* width */
	movl  4(%rdi), %r9d           /* height */
	cmpl  %r8d, %r9d
	je    .T_ok
	/* If width and height differ, the spec says we should return 0. */
	xorl  %eax, %eax
	leave
	ret

.T_ok:
	/* We fetch input and output data pointers early, since they do not change. */
	movq  8(%rdi), %r10           /* in->data  */
	movq  8(%rsi), %r11           /* out->data */

	/* We loop rows in %ecx and columns in %edx to cover the entire image. */
	xorl  %ecx, %ecx              /* i = 0 (row) */
.Tr_row:
	cmpl  %r9d, %ecx              /* i >= height ? */
	jge   .Tr_done

	xorl  %edx, %edx              /* j = 0 (col) */
.Tr_col:
	cmpl  %r8d, %edx              /* j >= width ? */
	jge   .Tr_next

	/* We compute src = i*width + j and load the pixel from input. */
	movl  %ecx, %eax
	imull %r8d, %eax
	addl  %edx, %eax
	movl  (%r10,%rax,4), %esi     /* %esi = in[src] */

	/* We compute dst = j*width + i and write it into the output. */
	movl  %edx, %eax
	imull %r8d, %eax
	addl  %ecx, %eax
	movl  %esi, (%r11,%rax,4)

	/* We advance the column index and keep going. */
	incl  %edx
	jmp   .Tr_col

.Tr_next:
	/* We advance the row index and continue the outer loop. */
	incl  %ecx
	jmp   .Tr_row

.Tr_done:
	/* We succeeded, so we return 1 per the API. */
	movl  $1, %eax
	leave
	ret


	.globl imgproc_ellipse
imgproc_ellipse:
	/* We make a frame and reserve space for locals: a, b, a^2, and b^2. */
	pushq %rbp
	movq  %rsp, %rbp
	subq  $32, %rsp               /* locals at -4(a), -8(b), -16(a^2), -24(b^2) */

	/* We save callee-saved registers that we will reuse across the nested loops. */
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx

	/* We load width, height, and both the input and output buffers. */
	movl  0(%rdi), %r8d           /* w */
	movl  4(%rdi), %r9d           /* h */
	movq  8(%rdi), %r10           /* in->data  */
	movq  8(%rsi), %r11           /* out->data */

	/* We compute a = floor(w/2) and b = floor(h/2), which define the centered ellipse. */
	movl  %r8d, %eax
	shrl  $1, %eax
	movl  %eax, -4(%rbp)          /* a */
	movl  %r9d, %eax
	shrl  $1, %eax
	movl  %eax, -8(%rbp)          /* b */

	/* We compute a^2 and b^2 as 64-bit values so we don’t overflow during the math. */
	movl  -4(%rbp), %eax
	imull %eax, %eax
	movslq %eax, %rax
	movq  %rax, -16(%rbp)         /* a^2 */

	movl  -8(%rbp), %eax
	imull %eax, %eax
	movslq %eax, %rax
	movq  %rax, -24(%rbp)         /* b^2 */

	/* We iterate over every pixel in row-major order. */
	xorl  %r14d, %r14d            /* row = 0 */
.Le_row:
	cmpl  %r9d, %r14d
	jge   .Le_done

	xorl  %r15d, %r15d            /* col = 0 */
.Le_col:
	cmpl  %r8d, %r15d
	jge   .Le_next_row

	/* We compute idx = row*w + col to index the image linearly. */
	movl  %r14d, %ebx
	imull %r8d,  %ebx
	addl  %r15d, %ebx

	/* We compute |x| = |col - a| without branching, because that is reliable and fast. */
	movl  %r15d, %ecx
	subl  -4(%rbp), %ecx
	movl  %ecx, %edx
	sarl  $31, %edx               /* %edx is all ones if negative, otherwise zero. */
	xorl  %edx, %ecx
	subl  %edx, %ecx               /* %ecx now holds |x|. */

	/* We compute |y| = |row - b| using the same two’s-complement trick. */
	movl  %r14d, %edi
	subl  -8(%rbp), %edi
	movl  %edi, %edx
	sarl  $31, %edx
	xorl  %edx, %edi
	subl  %edx, %edi               /* %edi now holds |y|. */

	/* We compute t1 = floor(10000*x*x / a^2) with 64-bit intermediates to avoid overflow. */
	movslq %ecx, %rax             /* rax = |x| (64-bit) */
	imulq  %rax, %rax             /* rax = x^2 */
	imulq  $10000, %rax, %rax     /* rax = 10000*x^2 */
	xorq   %rdx, %rdx             /* high half of dividend must be zero for divq. */
	movq   -16(%rbp), %rcx        /* rcx = a^2 */
	testq  %rcx, %rcx
	jz     .Le_outside            /* If a^2 is zero, we treat this as outside. */
	divq   %rcx                   /* rax = floor(10000*x^2 / a^2) */
	movq   %rax, %r12             /* r12 = t1 */

	/* We compute t2 = floor(10000*y*y / b^2) in the same way, guarding b^2. */
	movslq %edi, %rcx             /* rcx = |y| (64-bit) */
	imulq  %rcx, %rcx             /* rcx = y^2 */
	imulq  $10000, %rcx, %rcx     /* rcx = 10000*y^2 */
	xorq   %rdx, %rdx
	movq   -24(%rbp), %r13        /* r13 = b^2 */
	testq  %r13, %r13
	jz     .Le_outside
	movq   %rcx, %rax
	divq   %r13                   /* rax = floor(10000*y^2 / b^2) */

	/* If t1 + t2 is less than or equal to 10000, the pixel is considered inside. */
	addq   %r12, %rax
	cmpq   $10000, %rax
	jg     .Le_outside

	/* We copy the input pixel directly to the output when it is inside the ellipse. */
	movl  (%r10,%rbx,4), %eax
	movl  %eax, (%r11,%rbx,4)
	jmp   .Le_next_pixel

.Le_outside:
	/* We write fully-opaque black for pixels outside the ellipse (alpha = 255). */
	movl  $0x000000FF, (%r11,%rbx,4)

.Le_next_pixel:
	/* We advance to the next column and continue processing teh row. */
	incl  %r15d
	jmp   .Le_col

.Le_next_row:
	/* We advance to the next row and reset the column loop. */
	incl  %r14d
	jmp   .Le_row

.Le_done:
	/* We restore the callee-saved registers, release locals, and return cleanly. */
	popq  %rbx
	popq  %r15
	popq  %r14
	popq  %r13
	popq  %r12
	addq  $32, %rsp
	leave
	ret


	.globl imgproc_emboss
imgproc_emboss:
	/* We create a stack frame and save the registers that this routine reuses. */
	pushq %rbp
	movq  %rsp, %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	/* We load W, H, and the base pointers for the input and output images. */
	movl  0(%rdi), %r8d           /* W */
	movl  4(%rdi), %r9d           /* H */
	movq  8(%rdi), %r10           /* in->data  */
	movq  8(%rsi), %r11           /* out->data */

	/* We walk the image row by row and column by column. */
	xorl  %r14d, %r14d            /* row = 0 */
.Em_row:
	cmpl  %r9d, %r14d
	jge   .Em_done

	xorl  %r15d, %r15d            /* col = 0 */
.Em_col:
	cmpl  %r8d, %r15d
	jge   .Em_next_row

	/* We compute idx = row*W + col for easy linear addressing. */
	movl  %r14d, %ebx
	imull %r8d,  %ebx
	addl  %r15d, %ebx

	/* We load the current pixel and store its alpha, which must remain unchanged. */
	movl  (%r10,%rbx,4), %eax     /* p */
	movl  %eax, %r12d
	andl  $0xFF, %r12d            /* r12d = alpha(p) */

	/* If we are on the top row or the left column, we output gray 128 and keep alpha. */
	testl %r14d, %r14d
	je    .Em_edge
	testl %r15d, %r15d
	je    .Em_edge

	/* For interior pixels, we look at the upper-left neighbour: nidx = idx - (W + 1). */
	movl  %ebx, %ecx
	subl  %r8d, %ecx
	decl  %ecx
	movl  (%r10,%rcx,4), %esi     /* np */

	/* We compute dr = nr - r and make it the initial “best” channel difference. */
	movl  %eax, %edx
	shrl  $24, %edx               /* r(p) */
	andl  $0xFF, %edx
	movl  %esi, %ecx
	shrl  $24, %ecx               /* r(np) */
	andl  $0xFF, %ecx
	subl  %edx, %ecx               /* dr (signed) */
	movl  %ecx, %r13d             /* bestSigned = dr */
	/* We compute the absolute value of dr for tie-breaking against other channels. */
	movl  %ecx, %edi
	movl  %edi, %edx
	sarl  $31, %edx               /* sign mask */
	xorl  %edx, %edi
	subl  %edx, %edi               /* bestAbs = |dr| */

	/* We compute dg = ng - g and update “best” if its magnitude is strictly larger. */
	movl  %eax, %edx
	shrl  $16, %edx               /* g(p) */
	andl  $0xFF, %edx
	movl  %esi, %eax
	shrl  $16, %eax               /* g(np) */
	andl  $0xFF, %eax
	subl  %edx, %eax               /* dg (signed) */
	/* We form |dg| and compare to bestAbs; ties leave red winning as required by the spec. */
	movl  %eax, %edx
	movl  %edx, %ecx
	sarl  $31, %ecx
	xorl  %ecx, %edx
	subl  %ecx, %edx               /* |dg| */
	cmpl  %edi, %edx
	jle   .Em_check_b
	movl  %edx, %edi               /* bestAbs    = |dg| */
	movl  %eax, %r13d              /* bestSigned = dg   */

.Em_check_b:
	/* We compute db = nb - b and also consider that against the current best. */
	movl  (%r10,%rbx,4), %edx
	shrl  $8,  %edx               /* b(p) */
	andl  $0xFF, %edx
	movl  %esi, %ecx
	shrl  $8,  %ecx               /* b(np) */
	andl  $0xFF, %ecx
	subl  %edx, %ecx               /* db (signed) */
	/* We compute |db| and only update best if it is strictly larger (ties keep earlier). */
	movl  %ecx, %edx
	movl  %edx, %eax
	sarl  $31, %eax
	xorl  %eax, %edx
	subl  %eax, %edx               /* |db| */
	cmpl  %edi, %edx
	jle   .Em_have_best
	movl  %edx, %edi               /* bestAbs    = |db| */
	movl  %ecx, %r13d              /* bestSigned = db   */

.Em_have_best:
	/* We compute gray = clamp(128 + bestSigned, into the inclusive range 0..255). */
	leal  128(%r13d), %eax
	cmpl  $0, %eax
	jge   .Em_gray_nonneg
	/* If the sum is negative, we clamp it to zero since pixels can’t go below 0. */
	xorl  %eax, %eax
	jmp   .Em_store
.Em_gray_nonneg:
	cmpl  $255, %eax
	jle   .Em_store
	/* If the sum is too big, we clamp it down to 255 so it fits the channel. */
	movl  $255, %eax

.Em_store:
	/* We replicate gray into R, G, and B, then OR in the original alpha we saved earlier. */
	movl  %eax, %edx
	movl  %eax, %ecx
	shll  $24, %eax               /* R = gray << 24 */
	shll  $16, %edx               /* G = gray << 16 */
	shll  $8,  %ecx               /* B = gray << 8  */
	orl   %edx, %eax
	orl   %ecx, %eax
	orl   %r12d, %eax             /* add A */
	/* We write the new pixel back into the output image at the same index. */
	movl  %eax, (%r11,%rbx,4)
	jmp   .Em_advance

.Em_edge:
	/* Edge pixels always use gray 128 but keep the original alpha as-is. */
	movl  $128, %eax
	movl  %eax, %edx
	movl  %eax, %ecx
	shll  $24, %eax
	shll  $16, %edx
	shll  $8,  %ecx
	orl   %edx, %eax
	orl   %ecx, %eax
	orl   %r12d, %eax
	movl  %eax, (%r11,%rbx,4)

.Em_advance:
	/* We advance the column and continue with the next pixel in this row. */
	incl  %r15d
	jmp   .Em_col

.Em_next_row:
	/* We advance the row index and restart the inner loop from column zero. */
	incl  %r14d
	jmp   .Em_row

.Em_done:
	/* We restore all saved registers, tear the frame down, and return. */
	popq  %r15
	popq  %r14
	popq  %r13
	popq  %r12
	popq  %rbx
	leave
	ret
