; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double A[100], int N) {
;      double x = 0;
;      for (int i = 0; i < N; i++) {
; S:     x = 3.0 * i;
;      }
;      for (int i = 0; i < N; i++) {
; P:     A[i] = x;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_P	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                [N] -> { Stmt_P[i0] : 0 <= i0 < N };
; CHECK:            Schedule :=
; CHECK:                [N] -> { Stmt_P[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_P[i0] -> MemRef_A[i0] };
; CHECK:    }
;
; IR: polly.stmt.P:
; IR:   %1 = sext i32 %N to i64
; IR:   %2 = sub nsw i64 %1, 1
; IR:   %3 = trunc i64 %2 to i32
; IR:   %p_conv = sitofp i32 %3 to double
; IR:   %p_mul = fmul double %p_conv, 3.000000e+00
; IR:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar
; IR:   store double %p_mul, double* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar10.c"
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
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc6, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc6 ], [ 0, %for.end ]
  %cmp3 = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp3, label %for.body5, label %for.end8

for.body5:                                        ; preds = %for.cond2
  br label %P

P:                                                ; preds = %for.body5
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %x.0.lcssa, double* %arrayidx, align 8
  br label %for.inc6

for.inc6:                                         ; preds = %P
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end8:                                         ; preds = %for.cond2
  ret void
}
