; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
;
;    void f(int *A) {
;      for (int i = 0; i < 100; i++) {
;        A[i] = A[0] + A[75] + A[99];
;        if (i > 50)
;          A[i] = 0;
;        A[i] = A[i] * 2;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body[i0] : 1 = 0 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body[i0] -> [0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_A[0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_A[75] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_A[99] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_if_then
; CHECK:            Domain :=
; CHECK:                { Stmt_if_then[i0] : 1 = 0 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_then[i0] -> [0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_then[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_if_end	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_if_end[i0] : 51 <= i0 <= 99 }; CopyDom: { Stmt_if_end[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_end[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_end[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_if_end_0	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_if_end_0[i0] : 0 <= i0 <= 50 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_end_0[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_end_0[i0] -> MemRef_A[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_if_end_0[i0] -> MemRef_A[99] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_if_end_0[i0] -> MemRef_A[75] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_if_end_0[i0] -> MemRef_A[0] };
; CHECK:    }
;
; AST:    if (
; AST:    for (int c0 = 0; c0 <= 99; c0 += 1) {
; AST:      if (c0 <= 50) {
; AST:        Stmt_if_end_0(c0);
; AST:      } else {
; AST:        Stmt_if_end(c0);
; AST:      }
; AST:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/cond_prop_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp = load i32, i32* %A, align 4
  %arrayidx1 = getelementptr inbounds i32, i32* %A, i64 75
  %tmp1 = load i32, i32* %arrayidx1, align 4
  %add = add nsw i32 %tmp, %tmp1
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 99
  %tmp2 = load i32, i32* %arrayidx2, align 4
  %add3 = add nsw i32 %add, %tmp2
  %arrayidx4 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 %add3, i32* %arrayidx4, align 4
  %cmp5 = icmp sgt i64 %indvars.iv, 50
  br i1 %cmp5, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %arrayidx7 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 0, i32* %arrayidx7, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %arrayidx9 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx9, align 4
  %mul = shl nsw i32 %tmp3, 1
  %arrayidx11 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx11, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
