; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    #include <math.h>
;    void f(double B[100], int A[100]) {
;      int N = 100;
;      for (int i = 1; i < N; i++) {
;        int x = sqrt(i);
;        // split
;        B[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_for_body_cont	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_cont[i0] : 0 <= i0 <= 98 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_cont[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_cont[i0] -> MemRef_B[1 + i0] };
;
; IR: polly.stmt.for.body.cont:
; IR:   %0 = add nsw i64 %polly.indvar, 1
; IR:   %p_tmp = trunc i64 %0 to i32
; IR:   %p_conv = sitofp i32 %p_tmp to double
; IR:   %p_tmp1 = call fast double @llvm.sqrt.f64(double %p_conv)
; IR:   %p_conv1 = fptosi double %p_tmp1 to i32
; IR:   %p_conv2 = sitofp i32 %p_conv1 to double
; IR:   %scevgep1 = getelementptr double, double* %scevgep, i64 %polly.indvar
; IR:   store double %p_conv2, double* %scevgep1
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp = trunc i64 %indvars.iv to i32
  %conv = sitofp i32 %tmp to double
  %tmp1 = call fast double @llvm.sqrt.f64(double %conv)
  %conv1 = fptosi double %tmp1 to i32
  %conv2 = sitofp i32 %conv1 to double
  br label %for.body.cont

for.body.cont:
  %arrayidx = getelementptr inbounds double, double* %B, i64 %indvars.iv
  store double %conv2, double* %arrayidx, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

declare double @llvm.sqrt.f64(double) #1

attributes #1 = { nounwind readnone }
