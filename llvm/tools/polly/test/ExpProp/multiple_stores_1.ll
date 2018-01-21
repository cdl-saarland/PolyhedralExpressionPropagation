; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        A[i] = 0;
;        A[i] = A[i] + i;
;      }
;      for (int i = 0; i < 100; i++) {
;        B[i] += A[i];
;      }
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_for_body
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body[i0] : 1 = 0 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body[i0] -> [0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_for_body_split	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> [0, i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_for_body8	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body8[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body8[i0] -> [1, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body8[i0] -> MemRef_B[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body8[i0] -> MemRef_B[i0] };
; CHECK-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/multiple_stores_1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  store i32 0, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  %tmp = load i32, i32* %arrayidx2, align 4
  %tmp4 = trunc i64 %indvars.iv1 to i32
  %add = add nsw i32 %tmp, %tmp4
  %arrayidx4 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  store i32 %add, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc14, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc14 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body8, label %for.end16

for.body8:                                        ; preds = %for.cond6
  %arrayidx10 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp5 = load i32, i32* %arrayidx10, align 4
  %arrayidx12 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp6 = load i32, i32* %arrayidx12, align 4
  %add13 = add nsw i32 %tmp6, %tmp5
  store i32 %add13, i32* %arrayidx12, align 4
  br label %for.inc14

for.inc14:                                        ; preds = %for.body8
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond6

for.end16:                                        ; preds = %for.cond6
  ret void
}
