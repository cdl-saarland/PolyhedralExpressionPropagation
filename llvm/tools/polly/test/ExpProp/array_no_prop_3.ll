; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(int *tmp, int *y, int NX, int NY) {
;      for (int i = 0; i < NX; i++) {
;    A:    tmp[i] = 0;
;        for (int j = 0; j < NY; j++)
;    B:    tmp[i] = tmp[i] + 3;
;        for (int j = 0; j < NY; j++)
;    C:    y[j] = y[j] * tmp[i];
;      }
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_A
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_A[i0] : NY <= 0 and 0 <= i0 < NX };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_A[i0] -> [i0, 0, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_A[i0] -> MemRef_tmp[i0] };
; CHECK-NEXT:    	Stmt_B	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_B[i0, 0] : NY > 0 and 0 <= i0 < NX }; CopyDom: [NX, NY] -> { Stmt_B[i0, i1] : 0 <= i0 < NX and 0 <= i1 < NY };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_B[i0, i1] -> [i0, 1, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_B[i0, i1] -> MemRef_tmp[i0] };
; CHECK-NEXT:    	Stmt_C	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_C[i0, 0] : NY = 1 and 0 <= i0 < NX }; CopyDom: [NX, NY] -> { Stmt_C[i0, i1] : NY > 0 and 0 <= i0 < NX and 0 <= i1 < NY };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_C[i0, i1] -> [i0, 2, 0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: *] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_C[i0, i1] -> MemRef_y[i1] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: *] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_C[i0, i1] -> MemRef_y[i1] };
; CHECK-NEXT:    	Stmt_B_0
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_B_0[i0, i1] : 0 <= i0 < NX and 0 < i1 < NY };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_B_0[i0, i1] -> [i0, 1, i1] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_B_0[i0, i1] -> MemRef_tmp[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_B_0[i0, i1] -> MemRef_tmp[i0] };
; CHECK-NEXT:    	Stmt_C_0
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_C_0[i0, i1] : NY >= 2 and 0 <= i0 < NX and 0 <= i1 < NY };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                [NX, NY] -> { Stmt_C_0[i0, i1] -> [i0, 2, i1] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_C_0[i0, i1] -> MemRef_y[i1] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_C_0[i0, i1] -> MemRef_tmp[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                [NX, NY] -> { Stmt_C_0[i0, i1] -> MemRef_y[i1] };
; CHECK-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_no_prop_3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %tmp, i32* %y, i32 %NX, i32 %NY) {
entry:
  %tmp3 = sext i32 %NY to i64
  %tmp4 = sext i32 %NX to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc24, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc24 ], [ 0, %entry ]
  %cmp = icmp slt i64 %indvars.iv1, %tmp4
  br i1 %cmp, label %for.body, label %for.end26

for.body:                                         ; preds = %for.cond
  br label %A

A:                                                ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv1
  store i32 0, i32* %arrayidx, align 4
  br label %for.cond3

for.cond3:                                        ; preds = %for.inc, %A
  %j.0 = phi i32 [ 0, %A ], [ %inc, %for.inc ]
  %cmp4 = icmp slt i32 %j.0, %NY
  br i1 %cmp4, label %B, label %for.end

B:                                        ; preds = %for.cond3
  %arrayidx7 = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv1
  %tmp5 = load i32, i32* %arrayidx7, align 4
  %add = add nsw i32 %tmp5, 3
  %arrayidx9 = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv1
  store i32 %add, i32* %arrayidx9, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body5
  %inc = add nuw nsw i32 %j.0, 1
  br label %for.cond3

for.end:                                          ; preds = %for.cond3
  br label %for.cond12

for.cond12:                                       ; preds = %for.inc21, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc21 ], [ 0, %for.end ]
  %cmp13 = icmp slt i64 %indvars.iv, %tmp3
  br i1 %cmp13, label %for.body14, label %for.end23

for.body14:                                       ; preds = %for.cond12
  br label %C

C:                                                ; preds = %for.body14
  %arrayidx16 = getelementptr inbounds i32, i32* %y, i64 %indvars.iv
  %tmp6 = load i32, i32* %arrayidx16, align 4
  %arrayidx18 = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv1
  %tmp7 = load i32, i32* %arrayidx18, align 4
  %mul = mul nsw i32 %tmp6, %tmp7
  %arrayidx20 = getelementptr inbounds i32, i32* %y, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx20, align 4
  br label %for.inc21

for.inc21:                                        ; preds = %C
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond12

for.end23:                                        ; preds = %for.cond12
  br label %for.inc24

for.inc24:                                        ; preds = %for.end23
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end26:                                        ; preds = %for.cond
  ret void
}
