; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(double *A) {
;      for (int i = 0; i < 100; i++) {
;        double x = 3.3 * A[i];
;        // split
;        A[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_for_body_split	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body_split[i0] -> MemRef_A[i0] };
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_prop_4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  %tmp = load double, double* %arrayidx, align 8
  %mul = fmul fast double %tmp, 3.300000e+00
  br label %for.body.split

for.body.split:                                         ; preds = %for.cond
  %arrayidx2 = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %mul, double* %arrayidx2, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
