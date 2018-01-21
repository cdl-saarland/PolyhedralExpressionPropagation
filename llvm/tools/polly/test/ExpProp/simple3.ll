; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=4 -analyze < %s | FileCheck %s
; opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *restrict B, int N, int A[N]) {
;      for (int i = 1; i < N; i++)
;        A[i] = A[i - 1] * A[i - 1];
;      for (int i = 0; i < N; i++)
;        B[i] = A[i] + 2;
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body
; CHECK:            Domain :=
; CHECK:                [N] -> { Stmt_for_body[i0] : 0 <= i0 <= -2 + N };
; CHECK:            Schedule :=
; CHECK:                [N] -> { Stmt_for_body[i0] -> [0, i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                [N] -> { Stmt_for_body[i0] -> MemRef_A[1 + i0] };
; CHECK:     	Stmt_for_body9
; CHECK:             Domain :=
; CHECK:                 [N] -> { Stmt_for_body9[i0] : 0 <= i0 < N };
; CHECK:             Schedule :=
; CHECK:                 [N] -> { Stmt_for_body9[i0] -> [1, i0] };
; CHECK:             ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                 [N] -> { Stmt_for_body9[i0] -> MemRef_A[i0] };
; CHECK:             MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                 [N] -> { Stmt_for_body9[i0] -> MemRef_B[i0] };
; CHECK:    }
;
; IR: polly.stmt.for.body9:                             ; preds = %polly.then
; IR:   %4 = sub nsw i64 %polly.indvar9, 1
; IR:   %polly.access.A = getelementptr i32, i32* %A, i64 %4
; IR:   %tmp6_p_scalar_13 = load i32, i32* %polly.access.A
; IR:   %5 = sub nsw i64 %polly.indvar9, 1
; IR:   %polly.access.A14 = getelementptr i32, i32* %A, i64 %5
; IR:   %tmp8_p_scalar_15 = load i32, i32* %polly.access.A14
; IR:   %p_mul16 = mul nsw i32 %tmp6_p_scalar_13, %tmp8_p_scalar_15
; IR:   %p_add = add nsw i32 %p_mul16, 2
; IR:   %scevgep17 = getelementptr i32, i32* %B, i64 %polly.indvar9
; IR:   store i32 %p_add, i32* %scevgep17
; IR:   br label %polly.merge
;
; IR: polly.stmt.for.body918:                           ; preds = %polly.else
; IR:   %tmp10_p_scalar_ = load i32, i32* %scevgep19
; IR:   %p_add20 = add nsw i32 %tmp10_p_scalar_, 2
; IR:   store i32 %p_add20, i32* %scevgep21
; IR:   br label %polly.merge
;
source_filename = "../polly/test/ExpProp/simple3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, i32 %N, i32* %A) {
entry:
  %tmp = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 1, %entry ]
  %cmp = icmp slt i64 %indvars.iv1, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp5 = add nsw i64 %indvars.iv1, -1
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %tmp5
  %tmp6 = load i32, i32* %arrayidx, align 4
  %tmp7 = add nsw i64 %indvars.iv1, -1
  %arrayidx3 = getelementptr inbounds i32, i32* %A, i64 %tmp7
  %tmp8 = load i32, i32* %arrayidx3, align 4
  %mul = mul nsw i32 %tmp6, %tmp8
  %arrayidx5 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  store i32 %mul, i32* %arrayidx5, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp9 = sext i32 %N to i64
  br label %for.cond7

for.cond7:                                        ; preds = %for.inc14, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc14 ], [ 0, %for.end ]
  %cmp8 = icmp slt i64 %indvars.iv, %tmp9
  br i1 %cmp8, label %for.body9, label %for.end16

for.body9:                                        ; preds = %for.cond7
  %arrayidx11 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp10 = load i32, i32* %arrayidx11, align 4
  %add = add nsw i32 %tmp10, 2
  %arrayidx13 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add, i32* %arrayidx13, align 4
  br label %for.inc14

for.inc14:                                        ; preds = %for.body9
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond7

for.end16:                                        ; preds = %for.cond7
  ret void
}
