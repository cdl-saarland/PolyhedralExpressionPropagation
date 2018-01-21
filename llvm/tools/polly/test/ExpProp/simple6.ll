; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *restrict B, int A[100], int C[100]) {
;      int N = 100;
;
;      for (int i = 0; i < N; i++)
;        A[i] = C[i];
;      for (int i = 0; i < N; i++)
;        B[i] = A[i < 50 ? i : 50] * 2;
;    }
;
; CHECK:        Statements {
; CHECK-NEXT:    	Stmt_for_body
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body[i0] -> [0, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_C[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_for_body4	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body4[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body4[i0] -> [1, i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body4[i0] -> MemRef_B[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body4[i0] -> MemRef_C[50] : i0 >= 50; Stmt_for_body4[i0] -> MemRef_C[i0] : i0 <= 49 };
; CHECK-NEXT:    }
;
; IR: polly.stmt.for.body4:                             ; preds = %polly.loop_header4
; IR:   %8 = icmp slt i64 50, %polly.indvar7
; IR:   %9 = select i1 %8, i64 50, i64 %polly.indvar7
; IR:   %p_arrayidxC = getelementptr inbounds i32, i32* %C, i64 %9
; IR:   %p_Cval = load i32, i32* %p_arrayidxC,
; IR:   %p_mul = shl nsw i32 %p_Cval, 1
; IR:   %scevgep10 = getelementptr i32, i32* %B, i64 %polly.indvar7
; IR:   store i32 %p_mul
;
source_filename = "../polly/test/ExpProp/simple6.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, i32* %A, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  %arrayidxC = getelementptr inbounds i32, i32* %C, i64 %indvars.iv1
  %Cval = load i32, i32* %arrayidxC, align 4
  store i32 %Cval, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc10, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc10 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body4, label %for.end12

for.body4:                                        ; preds = %for.cond2
  %cmp5 = icmp slt i64 %indvars.iv, 50
  %cond = select i1 %cmp5, i64 %indvars.iv, i64 50
  %tmp4 = trunc i64 %cond to i32
  %idxprom6 = sext i32 %tmp4 to i64
  %arrayidx7 = getelementptr inbounds i32, i32* %A, i64 %idxprom6
  %tmp5 = load i32, i32* %arrayidx7, align 4
  %mul = shl nsw i32 %tmp5, 1
  %arrayidx9 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx9, align 4
  br label %for.inc10

for.inc10:                                        ; preds = %cond.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end12:                                        ; preds = %for.cond2
  ret void
}
