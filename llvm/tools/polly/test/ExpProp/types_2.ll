; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *A, float *B) {
;      for (int i = 0; i < 100; i++) {
;        float Ai = A[i];
;        // split
;        B[i] = Ai + 3.0;
;      }
;    }
;
; CHECK:         	Stmt_for_body_split	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> MemRef_B[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body_split[i0] -> MemRef_A[i0] };
;
; IR: @f
;
; IR: polly.stmt.for.body.split:                        ; preds = %polly.loop_header
; IR:   %p_arrayidx = getelementptr inbounds i32, i32* %A, i64 %polly.indvar
; IR:   %p_tmp = load i32, i32* %p_arrayidx, align 4
; IR:   %p_conv = sitofp i32 %p_tmp to float
; IR:   %p_conv2 = fadd float %p_conv, 3.000000e+00
; IR:   %scevgep = getelementptr float, float* %B, i64 %polly.indvar
; IR:   store float %p_conv2, float* %scevgep
;
; IR: @g
;
; IR: polly.stmt.for.body.split:                        ; preds = %polly.loop_header
; IR:   %p_arrayidx = getelementptr inbounds i32, i32* %A, i64 %polly.indvar
; IR:   %p_tmp = load i32, i32* %p_arrayidx, align 4
; IR:   %p_conv = sitofp i32 %p_tmp to float
; IR:   %p_conv2 = fadd float %p_conv, 3.000000e+00
; IR:   %scevgep = getelementptr float, float* %B, i64 %polly.indvar
; IR:   store float %p_conv2, float* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/types_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, float* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  br label %for.body.split

for.body.split:                                         ; preds = %for.cond
  %conv = sitofp i32 %tmp to float
  %conv2 = fadd float %conv, 3.000000e+00
  %arrayidx4 = getelementptr inbounds float, float* %B, i64 %indvars.iv
  store float %conv2, float* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

define void @g(i32* %A, float* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %conv = sitofp i32 %tmp to float
  br label %for.body.split

for.body.split:                                         ; preds = %for.cond
  %conv2 = fadd float %conv, 3.000000e+00
  %arrayidx4 = getelementptr inbounds float, float* %B, i64 %indvars.iv
  store float %conv2, float* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
