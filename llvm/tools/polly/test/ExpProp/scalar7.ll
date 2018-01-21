; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int p0, int p1, int B[100], int A[100]) {
;      int N = 100;
;      for (int i = 1; i < N; i++) {
;        int x = A[i];
;        if (p0)
;          x += 3;
;        if (p1)
;          x += 5;
;        B[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_if_end4	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4[i0] : p0 = 0 and p1 = 0 and 0 <= i0 <= 98 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: [p0, p1] -> { Stmt_if_end4[i0] -> MemRef_A[1 + i0] };
; CHECK:         	Stmt_if_end4_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0[i0] : p0 = 0 and 0 <= i0 <= 98 and (p1 < 0 or p1 > 0) };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0[i0] -> [i0] : p1 < 0 or p1 > 0 };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: [p0, p1] -> { Stmt_if_end4_0[i0] -> MemRef_A[1 + i0] : p1 < 0 or p1 > 0 };
; CHECK-NEXT:    	Stmt_if_end4_0_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0[i0] : p1 = 0 and 0 <= i0 <= 98 and (p0 < 0 or p0 > 0) };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0[i0] -> [i0] : p0 < 0 or p0 > 0 };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: [p0, p1] -> { Stmt_if_end4_0_0[i0] -> MemRef_A[1 + i0] : p0 < 0 or p0 > 0 };
; CHECK-NEXT:    	Stmt_if_end4_0_0_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0_0[i0] : 0 <= i0 <= 98 and ((p0 < 0 and p1 < 0) or (p0 < 0 and p1 > 0) or (p0 > 0 and p1 < 0) or (p0 > 0 and p1 > 0)) };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0_0[i0] -> [i0] : (p0 < 0 and p1 < 0) or (p0 < 0 and p1 > 0) or (p0 > 0 and p1 < 0) or (p0 > 0 and p1 > 0) };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [p0, p1] -> { Stmt_if_end4_0_0_0[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: [p0, p1] -> { Stmt_if_end4_0_0_0[i0] -> MemRef_A[1 + i0] : (p0 < 0 and p1 < 0) or (p0 < 0 and p1 > 0) or (p0 > 0 and p1 < 0) or (p0 > 0 and p1 > 0) };
;
;
; IR: polly.stmt.if.end4:
; IR:   %31 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A5 = getelementptr i32, i32* %A, i64 %31
; IR:   %tmp_p_scalar_ = load i32, i32* %polly.access.A5
; IR:   %p_add = add nsw i32 %tmp_p_scalar_, 3
; IR:   %p_add3 = add nsw i32 %p_add, 5
; IR:   %scevgep6 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; IR:   store i32 %p_add3, i32* %scevgep6
;
; IR: polly.stmt.if.end411:
; IR:   %37 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A12 = getelementptr i32, i32* %A, i64 %37
; IR:   %tmp_p_scalar_13 = load i32, i32* %polly.access.A12
; IR:   %scevgep15 = getelementptr i32, i32* %scevgep14, i64 %polly.indvar
; IR:   store i32 %tmp_p_scalar_13, i32* %scevgep15

; IR: polly.stmt.if.end420:
; IR:   %49 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A21 = getelementptr i32, i32* %A, i64 %49
; IR:   %tmp_p_scalar_22 = load i32, i32* %polly.access.A21
; IR:   %p_add323 = add nsw i32 %tmp_p_scalar_22, 5
; IR:   %scevgep25 = getelementptr i32, i32* %scevgep24, i64 %polly.indvar
; IR:   store i32 %p_add323, i32* %scevgep25
; IR:   br label %polly.merge17
;
; IR: polly.stmt.if.end426:
; IR:   %50 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A27 = getelementptr i32, i32* %A, i64 %50
; IR:   %tmp_p_scalar_28 = load i32, i32* %polly.access.A27, align 4, !alias.scope !0, !noalias !2
; IR:   %p_add29 = add nsw i32 %tmp_p_scalar_28, 3
; IR:   %scevgep31 = getelementptr i32, i32* %scevgep30, i64 %polly.indvar
; IR:   store i32 %p_add29, i32* %scevgep31
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar7.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32 %p0, i32 %p1, i32* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %tobool = icmp eq i32 %p0, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %for.body
  %add = add nsw i32 %tmp, 3
  br label %if.end

if.end:                                           ; preds = %for.body, %if.then
  %x.0 = phi i32 [ %add, %if.then ], [ %tmp, %for.body ]
  %tobool1 = icmp eq i32 %p1, 0
  br i1 %tobool1, label %if.end4, label %if.then2

if.then2:                                         ; preds = %if.end
  %add3 = add nsw i32 %x.0, 5
  br label %if.end4

if.end4:                                          ; preds = %if.end, %if.then2
  %x.1 = phi i32 [ %add3, %if.then2 ], [ %x.0, %if.end ]
  %arrayidx6 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %x.1, i32* %arrayidx6, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
