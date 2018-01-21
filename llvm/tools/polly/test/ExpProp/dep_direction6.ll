; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        int x = A[i - 1];
;        // split
;        B[i] = x;
;        A[50] = 0;
;      }
;    }
;
; CHECK:        Statements {
; CHECK-NEXT:   	Stmt_for_body
; CHECK-NEXT:           Domain :=
; CHECK-NEXT:               { Stmt_for_body[i0] : 1 = 0 };
; CHECK-NEXT:           Schedule :=
; CHECK-NEXT:               { Stmt_for_body[i0] -> [0] };
; CHECK-NEXT:           ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               { Stmt_for_body[i0] -> MemRef_A[-1 + i0] };
; CHECK-NEXT:           MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               { Stmt_for_body[i0] -> MemRef_tmp2_reg2mem[0] };
; CHECK-NEXT:   	Stmt_for_body_split	 [#Recompute Values: 1]
; CHECK-NEXT:           Domain :=
; CHECK-NEXT:               { Stmt_for_body_split[51] }; CopyDom: { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:           Schedule :=
; CHECK-NEXT:               { Stmt_for_body_split[i0] -> [51, 0] };
; CHECK-NEXT:           MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               { Stmt_for_body_split[i0] -> MemRef_B[i0] };
; CHECK-NEXT:   	Stmt_for_body_split_split
; CHECK-NEXT:           Domain :=
; CHECK-NEXT:               { Stmt_for_body_split_split[99] };
; CHECK-NEXT:           Schedule :=
; CHECK-NEXT:               { Stmt_for_body_split_split[i0] -> [99, 1] };
; CHECK-NEXT:           MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               { Stmt_for_body_split_split[i0] -> MemRef_A[50] };
; CHECK-NEXT:   	Stmt_for_body_split_0	 [#Recompute Values: 1]
; CHECK-NEXT:           Domain :=
; CHECK-NEXT:               { Stmt_for_body_split_0[i0] : 0 <= i0 <= 99 and (i0 >= 52 or i0 <= 50) };
; CHECK-NEXT:           Schedule :=
; CHECK-NEXT:               { Stmt_for_body_split_0[i0] -> [i0, 0] : i0 >= 52 or i0 <= 50 };
; CHECK-NEXT:           MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               { Stmt_for_body_split_0[i0] -> MemRef_B[i0] };
; CHECK-NEXT:           ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:               null
; CHECK-NEXT:          new: { Stmt_for_body_split_0[i0] -> MemRef_A[-1 + i0] };
; CHECK-NEXT:    }
;
source_filename = "test/ExpProp/dep_direction4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp = add nsw i64 %indvars.iv, -1
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %tmp
  %tmp2 = load i32, i32* %arrayidx, align 4
  br label %for.body.split

for.body.split:
  %arrayidx2 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp2, i32* %arrayidx2, align 4
  %arrayidx3 = getelementptr inbounds i32, i32* %A, i64 50
  store i32 0, i32* %arrayidx3, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
