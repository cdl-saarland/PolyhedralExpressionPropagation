; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double A[100], int N) {
;      double x = 0;
;      for (int i = 0; i < N; i++) {
;   S:   x = 3.0 * i;
;      }
;      int i = 0;
;      do {
;   P:   A[i] = x;
;      } while (i++ < N);
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_P	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                [N] -> { Stmt_P[i0] : N > 0 and 0 <= i0 <= N };
; CHECK:            Schedule :=
; CHECK:                [N] -> { Stmt_P[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_P[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_P_0	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                [N] -> { Stmt_P_0[0] : N <= 0 };
; CHECK:            Schedule :=
; CHECK:                [N] -> { Stmt_P_0[i0] -> [0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_P_0[i0] -> MemRef_A[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_P_0[i0] -> MemRef_x_0_reg2mem[0]
; CHECK:    }
;
; IR: polly.stmt.P:
; IR:   %3 = sext i32 %N to i64
; IR:   %4 = sub nsw i64 %3, 1
; IR:   %5 = trunc i64 %4 to i32
; IR:   %p_conv = sitofp i32 %5 to double
; IR:   %p_mul = fmul double %p_conv, 3.000000e+00
; IR:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar
; IR:   store double %p_mul, double* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar11.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A, i32 %N) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi double [ 0.000000e+00, %entry ], [ %mul, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, %N
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S

S:                                                ; preds = %for.body
  br label %for.inc

for.inc:                                          ; preds = %S
  %conv = sitofp i32 %i.0 to double
  %mul = fmul double %conv, 3.000000e+00
  %inc = add nuw nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi double [ %x.0, %for.cond ]
  %tmp = sext i32 %N to i64
  br label %do.body

do.body:                                          ; preds = %do.cond, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %do.cond ], [ 0, %for.end ]
  br label %P

P:                                                ; preds = %do.body
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %x.0.lcssa, double* %arrayidx, align 8
  br label %do.cond

do.cond:                                          ; preds = %P
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp3 = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp3, label %do.body, label %do.end

do.end:                                           ; preds = %do.cond
  ret void
}
