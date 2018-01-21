; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int B[100], int A[100]) {
;      int N = 100;
;      for (int i = 1; i < N; i++) {
;        int x = i * A[i];
;        // split
;        B[i] = x;
;      }
;    }
;
;
; CHECK:        Stmt_for_body_split
; CHECK-NEXT:      Domain :=
; CHECK-NEXT:          { Stmt_for_body_split[i0] : 0 <= i0 <= 98 };
; CHECK-NEXT:      Schedule :=
; CHECK-NEXT:          { Stmt_for_body_split[i0] -> [i0] };
; CHECK-NEXT:      MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:          { Stmt_for_body_split[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:      ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:          null;
; CHECK-NEXT:     new: { Stmt_for_body_split[i0] -> MemRef_A[1 + i0] };
; CHECK-NEXT: }
;
; IR: polly.stmt.for.body.split:
; IR:  %8 = add nsw i64 %polly.indvar, 1
; IR:  %p_tmp1 = trunc i64 %8 to i32
; IR:  %9 = add nsw i64 %polly.indvar, 1
; IR:  %p_arrayidx = getelementptr inbounds i32, i32* %A, i64 %9
; IR:  %p_tmp = load i32, i32* %p_arrayidx, align 4, !alias.scope !0, !noalias !2
; IR:  %p_mul = mul nsw i32 %p_tmp1, %p_tmp
; IR:  %scevgep3 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; IR:  store i32 %p_mul, i32* %scevgep3
;
source_filename = "../polly/test/ExpProp/scalar1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %tmp1 = trunc i64 %indvars.iv to i32
  %mul = mul nsw i32 %tmp1, %tmp
  br label %for.body.split

for.body.split:
  %arrayidx2 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx2, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
