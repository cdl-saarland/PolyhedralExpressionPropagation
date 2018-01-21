; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
; Verify we do propagate tmp = A[i] to for_end.
;
;    void f(int *A, int *B, int *C) {
;      for (int i = 0; i < 100; i++) {
;        for (int j = 0; j < 100; j++) {
;          A[i] = A[i] + B[j];
;          int tmp = A[i];
;        }
;        C[i] = tmp;
;      }
;    }
;
; CHECK: { Stmt_for_body[i0] : 1 = 0 };
; CHECK: { Stmt_for_cond1[i0, i1] : 1 = 0 };
;
; CHECK:         	Stmt_for_body3
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body3[i0, i1] : 0 <= i0 <= 99 and 0 <= i1 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body3[i0, i1] -> [i0, 0, i1] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body3[i0, i1] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body3[i0, i1] -> MemRef_B[i1] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body3[i0, i1] -> MemRef_A[i0] };
;
; CHECK: { Stmt_for_body3_split[i0, i1] : 1 = 0 };
; CHECK: { Stmt_for_inc[i0, i1] : 1 = 0 };
;
; CHECK:         	Stmt_for_end	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_end[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_end[i0] -> [i0, 1, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_end[i0] -> MemRef_C[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_end[i0] -> MemRef_A[i0] };
;
source_filename = "../polly/test/ExpProp/overwrite_1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc12, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc12 ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end14

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %for.body ]
  %aval = phi i32 [ %reload, %for.inc ], [ undef, %for.body ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx5 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp4 = load i32, i32* %arrayidx5, align 4
  %add = add nsw i32 %tmp, %tmp4
  %arrayidx7 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  store i32 %add, i32* %arrayidx7, align 4
  %reload = load i32, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  %arrayidx11 = getelementptr inbounds i32, i32* %C, i64 %indvars.iv1
  store i32 %aval, i32* %arrayidx11, align 4
  br label %for.inc12

for.inc12:                                        ; preds = %for.end
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end14:                                        ; preds = %for.cond
  ret void
}
