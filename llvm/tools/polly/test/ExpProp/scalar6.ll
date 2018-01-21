; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int B[100], int A[100]) {
;      int N = 100;
;      for (int i = 1; i < N; i++) {
;        int x = i * A[i];
;        if (i > 20)
;          x += 3;
;        if (i > 40)
;          x += 5;
;        B[i] = x;
;      }
;    }
;
; CHECK:         	Stmt_if_end5	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_if_end5[i0] : 0 <= i0 <= 19 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_if_end5[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_if_end5[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_if_end5[i0] -> MemRef_A[1 + i0] };
; CHECK:         	Stmt_if_end5_0	 [#Recompute Values: 2]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_if_end5_0[i0] : 40 <= i0 <= 98 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_if_end5_0[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_if_end5_0[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_if_end5_0[i0] -> MemRef_A[1 + i0] };
; CHECK:         	Stmt_if_end5_0_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_if_end5_0_0[i0] : 20 <= i0 <= 39 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_if_end5_0_0[i0] -> [i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_if_end5_0_0[i0] -> MemRef_B[1 + i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_if_end5_0_0[i0] -> MemRef_A[1 + i0] };
;
; IR: polly.stmt.if.end5:
; IR:   %9 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A5 = getelementptr i32, i32* %A, i64 %9
; IR:   %tmp_p_scalar_ = load i32, i32* %polly.access.A5, align 4, !alias.scope !0, !noalias !2
; IR:   %10 = add nsw i64 %polly.indvar, 1
; IR:   %p_tmp1 = trunc i64 %10 to i32
; IR:   %p_mul = mul nsw i32 %p_tmp1, %tmp_p_scalar_
; IR:   %scevgep6 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; IR:   store i32 %p_mul, i32* %scevgep6
;
; IR: polly.stmt.if.end511:
; IR:   %12 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A12 = getelementptr i32, i32* %A, i64 %12
; IR:   %tmp_p_scalar_13 = load i32, i32* %polly.access.A12, align 4, !alias.scope !0, !noalias !2
; IR:   %13 = add nsw i64 %polly.indvar, 1
; IR:   %p_tmp114 = trunc i64 %13 to i32
; IR:   %p_mul15 = mul nsw i32 %p_tmp114, %tmp_p_scalar_13
; IR:   %p_add = add nsw i32 %p_mul15, 3
; IR:   %scevgep17 = getelementptr i32, i32* %scevgep16, i64 %polly.indvar
; IR:   store i32 %p_add, i32* %scevgep17
;
; IR: polly.stmt.if.end518:
; IR:   %14 = add nsw i64 %polly.indvar, 1
; IR:   %polly.access.A19 = getelementptr i32, i32* %A, i64 %14
; IR:   %tmp_p_scalar_20 = load i32, i32* %polly.access.A19, align 4, !alias.scope !0, !noalias !2
; IR:   %15 = add nsw i64 %polly.indvar, 1
; IR:   %p_tmp121 = trunc i64 %15 to i32
; IR:   %p_mul22 = mul nsw i32 %p_tmp121, %tmp_p_scalar_20
; IR:   %p_add23 = add nsw i32 %p_mul22, 3
; IR:   %p_add4 = add nsw i32 %p_add23, 5
; IR:   %scevgep25 = getelementptr i32, i32* %scevgep24, i64 %polly.indvar
; IR:   store i32 %p_add4, i32* %scevgep25
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar6.c"
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
  %cmp1 = icmp sgt i64 %indvars.iv, 20
  br i1 %cmp1, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %add = add nsw i32 %mul, 3
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %x.0 = phi i32 [ %add, %if.then ], [ %mul, %for.body ]
  %cmp2 = icmp sgt i64 %indvars.iv, 40
  br i1 %cmp2, label %if.then3, label %if.end5

if.then3:                                         ; preds = %if.end
  %add4 = add nsw i32 %x.0, 5
  br label %if.end5

if.end5:                                          ; preds = %if.then3, %if.end
  %x.1 = phi i32 [ %add4, %if.then3 ], [ %x.0, %if.end ]
  %arrayidx7 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %x.1, i32* %arrayidx7, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end5
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
