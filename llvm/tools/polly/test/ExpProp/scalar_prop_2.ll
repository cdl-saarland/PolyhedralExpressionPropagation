; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double *A, double *B) {
;      double x;
;      for (int i = 0; i < 100; i++) {
;        x = 3.3 * B[i];
;        // split
;        A[i] = x;
;      }
;
;      for (int i = 0; i < 100; i++) {
;        A[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_for_body6	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body6[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body6[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body6[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body6[i0] -> MemRef_B[99] };
;
; IR:      polly.stmt.for.body6:
; IR-NEXT:   %polly.access.B4 = getelementptr double, double* %B, i64 99
; IR-NEXT:   %tmp_p_scalar_ = load double, double* %polly.access.B4, align 8
; IR-NEXT:   %p_mul = fmul fast double %tmp_p_scalar_, 3.300000e+00
; IR-NEXT:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar
; IR-NEXT:   store double %p_mul, double* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_prop.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A, double* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %x.0 = phi double [ undef, %entry ], [ %mul, %for.inc ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds double, double* %B, i64 %indvars.iv1
  %tmp = load double, double* %arrayidx, align 8
  %mul = fmul fast double %tmp, 3.300000e+00
  br label %for.body.split

for.body.split:
  %arrayidx2 = getelementptr inbounds double, double* %A, i64 %indvars.iv1
  store double %mul, double* %arrayidx2, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi double [ %x.0, %for.cond ]
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc9, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc9 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body6, label %for.end11

for.body6:                                        ; preds = %for.cond4
  %arrayidx8 = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %x.0.lcssa, double* %arrayidx8, align 8
  br label %for.inc9

for.inc9:                                         ; preds = %for.body6
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond4

for.end11:                                        ; preds = %for.cond4
  ret void
}
