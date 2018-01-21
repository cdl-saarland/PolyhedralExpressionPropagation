; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0  -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        A[75] = 0;
;        int x = A[i - 1];
;        // split
;        B[i] = x;
;        A[50] = 0;
;      }
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_for_body
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body[99] };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body[i0] -> [99, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_A[75] };
; CHECK:          Stmt_for_body_split	 [#Recompute Values: 1]
; CHECK-NEXT:             Domain :=
; CHECK-NEXT:                 { Stmt_for_body_split[51] }; CopyDom: { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:             Schedule :=
; CHECK-NEXT:                 { Stmt_for_body_split[i0] -> [51, 1] };
; CHECK-NEXT:             MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                 { Stmt_for_body_split[i0] -> MemRef_B[i0] };
; CHECK-NEXT:     Stmt_for_body_split_split
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split_split[99] };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split_split[i0] -> [99, 2] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split_split[i0] -> MemRef_A[50] };
; CHECK:          Stmt_for_body_split_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split_0[76] }; CopyDom: { Stmt_for_body_split_0[i0] : 0 <= i0 <= 99 and (i0 >= 52 or i0 <= 50) };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split_0[i0] -> [76, 1] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split_0[i0] -> MemRef_B[i0] };
; CHECK:          Stmt_for_body_split_0_0	 [#Recompute Values: 1]
; CHECK-NEXT:             Domain :=
; CHECK-NEXT:                 { Stmt_for_body_split_0_0[i0] : 0 <= i0 <= 99 and (i0 >= 77 or (52 <= i0 <= 75) or i0 <= 50) };
; CHECK-NEXT:             Schedule :=
; CHECK-NEXT:                 { Stmt_for_body_split_0_0[i0] -> [i0, 1] : i0 >= 77 or i0 <= 50 or (52 <= i0 <= 75) };
; CHECK-NEXT:             MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                 { Stmt_for_body_split_0_0[i0] -> MemRef_B[i0] };
; CHECK-NEXT:             ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                 null;
; CHECK-NEXT:            new: { Stmt_for_body_split_0_0[i0] -> MemRef_A[-1 + i0] }
;
; IR:  %scevgep = getelementptr i32, i32* %A, i64 75
;
; IR:      polly.stmt.for.body:                              ; preds = %polly.then
; IR-NEXT:  store i32 0, i32* %scevgep, align 4, !alias.scope !0, !noalias !2
; IR-NEXT:  br label %polly.merge
;
; IR:      polly.stmt.for.body3:                             ; preds = %polly.else
; IR-NEXT:  store i32 0, i32* %scevgep, align 4, !alias.scope !0, !noalias !2
; IR-NEXT:  br label %polly.merge
;
source_filename = "test/ExpProp/dep_direction5.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 75
  store i32 0, i32* %arrayidx, align 4
  %tmp = add nsw i64 %indvars.iv, -1
  %arrayidx1 = getelementptr inbounds i32, i32* %A, i64 %tmp
  %tmp2 = load i32, i32* %arrayidx1, align 4
  br label %for.body.split

for.body.split:
  %arrayidx3 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp2, i32* %arrayidx3, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %A, i64 50
  store i32 0, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
