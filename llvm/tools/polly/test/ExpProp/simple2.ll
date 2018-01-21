; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=2 -analyze < %s | FileCheck %s --check-prefix=SCOP
;
;    void f(int *restrict B, int N, int A[N]) {
;      for (int i = 1; i < N; i++)
;        A[i] = A[i - 1] * A[i - 1];
;      for (int i = 1; i < N; i++)
;        B[i] = A[i] + 2;
;    }
;
; SCOP:    Statements {
; SCOP:    	Stmt_for_body
; SCOP:            Domain :=
; SCOP:                [N] -> { Stmt_for_body[i0] : 0 <= i0 <= -2 + N };
; SCOP:            Schedule :=
; SCOP:                [N] -> { Stmt_for_body[i0] -> [0, i0] };
; SCOP:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP:                [N] -> { Stmt_for_body[i0] -> MemRef_A[i0] };
; SCOP:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP:                [N] -> { Stmt_for_body[i0] -> MemRef_A[i0] };
; SCOP:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP:                [N] -> { Stmt_for_body[i0] -> MemRef_A[1 + i0] };
; SCOP:    	Stmt_for_body9
; SCOP:            Domain :=
; SCOP:                [N] -> { Stmt_for_body9[i0] : 0 <= i0 <= -2 + N };
; SCOP:            Schedule :=
; SCOP:                [N] -> { Stmt_for_body9[i0] -> [1, i0] };
; SCOP:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP:                [N] -> { Stmt_for_body9[i0] -> MemRef_B[1 + i0] };
; SCOP:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP:                null;
; SCOP:           new: [N] -> { Stmt_for_body9[i0] -> MemRef_A[i0] };
; SCOP:    }
;
source_filename = "../polly/test/ExpProp/simple2.c"
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
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc14 ], [ 1, %for.end ]
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
