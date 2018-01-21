; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-prop-times=0 -analyze < %s | FileCheck %s
;
;    void f(int *tmp, int *y, int NX, int NY) {
;   A: tmp[0] = 0;
;      for (int j = 0; j < NY; j++)
;   B:   tmp[0] = tmp[0] + 3;
;      for (int j = 0; j < NY; j++)
;   C:   y[j] = y[j] * tmp[0];
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:     	Stmt_A
; CHECK-NEXT:             Domain :=
; CHECK-NEXT:                 [NY] -> { Stmt_A[] : NY <= 0 };
; CHECK-NEXT:             Schedule :=
; CHECK-NEXT:                 [NY] -> { Stmt_A[] -> [0, 0] };
; CHECK-NEXT:             MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                 [NY] -> { Stmt_A[] -> MemRef_tmp[0] };
; CHECK-NEXT:      Stmt_B	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NY] -> { Stmt_B[0] : NY = 1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NY] -> { Stmt_B[i0] -> [1, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_B[i0] -> MemRef_tmp[0] };
; CHECK-NEXT:    	Stmt_C	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NY] -> { Stmt_C[i0] : NY = 2 and 0 <= i0 <= 1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NY] -> { Stmt_C[i0] -> [2, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: *] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_C[i0] -> MemRef_y[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: *] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_C[i0] -> MemRef_y[i0] };
; CHECK-NEXT:    	Stmt_B_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NY] -> { Stmt_B_0[1] : NY >= 2 }
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NY] -> { Stmt_B_0[i0] -> [1, 1] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_B_0[i0] -> MemRef_tmp[0] };
; CHECK-NEXT:    	Stmt_B_0_0
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NY] -> { Stmt_B_0_0[i0] : 2 <= i0 < NY };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NY] -> { Stmt_B_0_0[i0] -> [1, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_B_0_0[i0] -> MemRef_tmp[0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_B_0_0[i0] -> MemRef_tmp[0] };
; CHECK-NEXT:    	Stmt_C_0
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NY] -> { Stmt_C_0[i0] : NY >= 3 and 0 <= i0 < NY; Stmt_C_0[0] : NY = 1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NY] -> { Stmt_C_0[i0] -> [2, i0] : NY >= 3; Stmt_C_0[0] -> [2, 0] : NY = 1 };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_C_0[i0] -> MemRef_y[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_C_0[i0] -> MemRef_tmp[0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NY] -> { Stmt_C_0[i0] -> MemRef_y[i0] };
; CHECK-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_no_prop_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %tmp, i32* %y, i32 %NX, i32 %NY) {
entry:
  br label %A

A:                                                ; preds = %entry
  store i32 0, i32* %tmp, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %A
  %j.0 = phi i32 [ 0, %A ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %j.0, %NY
  br i1 %cmp, label %B, label %for.end

B:                                         ; preds = %for.cond
  %tmp1 = load i32, i32* %tmp, align 4
  %add = add nsw i32 %tmp1, 3
  store i32 %add, i32* %tmp, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nuw nsw i32 %j.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp2 = sext i32 %NY to i64
  br label %for.cond9

for.cond9:                                        ; preds = %for.inc18, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc18 ], [ 0, %for.end ]
  %cmp10 = icmp slt i64 %indvars.iv, %tmp2
  br i1 %cmp10, label %for.body11, label %for.end20

for.body11:                                       ; preds = %for.cond9
  br label %C

C:                                                ; preds = %for.body11
  %arrayidx13 = getelementptr inbounds i32, i32* %y, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx13, align 4
  %tmp4 = load i32, i32* %tmp, align 4
  %mul = mul nsw i32 %tmp3, %tmp4
  %arrayidx17 = getelementptr inbounds i32, i32* %y, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx17, align 4
  br label %for.inc18

for.inc18:                                        ; preds = %C
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond9

for.end20:                                        ; preds = %for.cond9
  ret void
}
