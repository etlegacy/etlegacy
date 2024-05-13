include \masm32\include64\masm64rt.inc
.code
PUBLIC SinCos
; define SinCos(float rad, float *S, float *C);

angle_on_stack$ = 8

SinCos proc
    movsd QWORD PTR angle_on_stack$[rsp], xmm0      ; argument angle is in xmm0, move it to the stack
    fld QWORD PTR angle_on_stack$[rsp]              ; push angle onto the FPU stack where we can do FLOPs
    fsincos
    fstp QWORD PTR [xmm1]                             ; store/pop cosine output argument 
    fstp QWORD PTR [xmm2]                            ; store/pop sine output argument
    ret 0
SinCos endp

end