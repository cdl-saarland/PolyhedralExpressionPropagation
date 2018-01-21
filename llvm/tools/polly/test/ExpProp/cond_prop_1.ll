; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=DCE
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        int x = A[i];
;        if (i > 50)
;          A[i] = 0;
;        B[i] = x;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body[i0] : 51 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body[i0] -> [i0, 0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_tmp_reg2mem[0] };
; CHECK:    	Stmt_if_then
; CHECK:            Domain :=
; CHECK:                { Stmt_if_then[i0] : 51 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_then[i0] -> [i0, 1] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_then[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_if_end	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_if_end[i0] : 0 <= i0 <= 50 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_end[i0] -> [i0, 2] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_end[i0] -> MemRef_B[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_if_end[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_if_end_0
; CHECK:            Domain :=
; CHECK:                { Stmt_if_end_0[i0] : 51 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_if_end_0[i0] -> [i0, 2] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_end_0[i0] -> MemRef_tmp_reg2mem[0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_if_end_0[i0] -> MemRef_B[i0] };
; CHECK:    }
;
; DCE:    for (int c0 = 0; c0 <= 99; c0 += 1) {
; DCE:      if (c0 >= 51) {
; DCE:        Stmt_for_body(c0);
; DCE:        Stmt_if_then(c0);
; DCE:        Stmt_if_end_0(c0);
; DCE:      } else {
; DCE:        Stmt_if_end(c0);
; DCE:      }
; DCE:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_prop_cond.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %cmp1 = icmp sgt i64 %indvars.iv, 50
  br i1 %cmp1, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %arrayidx3 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 0, i32* %arrayidx3, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %arrayidx5 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp, i32* %arrayidx5, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
