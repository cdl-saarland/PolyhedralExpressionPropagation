; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int B[100], int A[100], int N) {
;      int x = 0;
;      for (int i = 0; i < N; i++) {
;    S:  x = i;
;      }
;      for (int i = 0; i < N; i++) {
;    P:  A[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_P	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [N] -> { Stmt_P[i0] : 0 <= i0 < N };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [N] -> { Stmt_P[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [N] -> { Stmt_P[i0] -> MemRef_A[i0] };
;
; IR: polly.stmt.P:
; IR:  %1 = sext i32 %N to i64
; IR:  %2 = sub nsw i64 %1, 1
; IR:  %3 = trunc i64 %2 to i32
; IR:  %scevgep = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:  store i32 %3, i32* %scevgep, align 4, !alias.scope !0, !noalias !2
;
source_filename = "../polly/test/ExpProp/scalar4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %B, i32* %A, i32 %N) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi i32 [ 0, %entry ], [ %i.0, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, %N
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S

S:                                                ; preds = %for.body
  br label %for.inc

for.inc:                                          ; preds = %S
  %inc = add nuw nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi i32 [ %x.0, %for.cond ]
  %tmp = sext i32 %N to i64
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc5, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc5 ], [ 0, %for.end ]
  %cmp3 = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp3, label %for.body4, label %for.end7

for.body4:                                        ; preds = %for.cond2
  br label %P

P:                                                ; preds = %for.body4
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 %x.0.lcssa, i32* %arrayidx, align 4
  br label %for.inc5

for.inc5:                                         ; preds = %P
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end7:                                         ; preds = %for.cond2
  ret void
}
