; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double *A, double b, double c) {
;      for (int i = 0; i < 100; i++) {
;        double a = 213.23;
;        // split
;        a += b;
;        a *= 2;
;        // split
;        a *= 3;
;        a += c;
;        a *= 5;
;        // split
;        a *= 7;
;        A[i] = a;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_BB2	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_BB2[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_BB2[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_BB2[i0] -> MemRef_A[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 1]
; CHECK:                null;
; CHECK:           new: { Stmt_BB2[i0] -> MemRef_c[] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 1]
; CHECK:                null;
; CHECK:           new: { Stmt_BB2[i0] -> MemRef_b[] };
; CHECK:    }
;
; IR: polly.stmt.BB2:                                   ; preds = %polly.loop_header
; IR:   %p_add = fadd double %b, 2.132300e+02
; IR:   %p_mul = fmul double %p_add, 2.000000e+00
; IR:   %p_mul1 = fmul double %p_mul, 3.000000e+00
; IR:   %p_add2 = fadd double %p_mul1, %c
; IR:   %p_mul3 = fmul double %p_add2, 5.000000e+00
; IR:   %p_mul4 = fmul double %p_mul3, 7.000000e+00
; IR:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar
; IR:   store double %p_mul4
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_chain_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A, double %b, double %c) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %BB0, label %for.end

BB0:                                         ; preds = %for.cond
  %add = fadd double %b, 2.132300e+02
  %mul = fmul double %add, 2.000000e+00
  br label %BB1
BB1:
  %mul1 = fmul double %mul, 3.000000e+00
  %add2 = fadd double %mul1, %c
  %mul3 = fmul double %add2, 5.000000e+00
  br label %BB2
BB2:
  %mul4 = fmul double %mul3, 7.000000e+00
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %mul4, double* %arrayidx, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
