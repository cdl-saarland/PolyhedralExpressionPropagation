; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(double *A, double *B) {
;      for (int i = 0; i < 100; i++) {
;        double x = 3.3 * A[i];
;        // split
;        A[i] = 0;
;        A[i] = x;
;      }
;
;      for (int i = 0; i < 100; i++) {
;        B[i] = A[i];
;      }
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_for_body
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body[i0] : 1 = 0 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body[i0] -> [0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body[i0] -> MemRef_mul_reg2mem[0] };
; CHECK-NEXT:    	Stmt_for_body_split
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] : 1 = 0 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> [0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_for_body_split_split	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body_split_split[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body_split_split[i0] -> [0, i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body_split_split[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body_split_split[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_for_body6
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body6[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body6[i0] -> [1, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body6[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body6[i0] -> MemRef_B[i0] };
; CHECK-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_no_prop.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A, double* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv1
  %tmp = load double, double* %arrayidx, align 8
  %mul = fmul fast double %tmp, 3.300000e+00
  br label %for.body.split

for.body.split:
  %arrayidx2 = getelementptr inbounds double, double* %A, i64 %indvars.iv1
  store double 0.0e+00, double* %arrayidx2, align 8
  store double %mul, double* %arrayidx2, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc11, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc11 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body6, label %for.end13

for.body6:                                        ; preds = %for.cond4
  %arrayidx8 = getelementptr inbounds double, double* %A, i64 %indvars.iv
  %tmp4 = bitcast double* %arrayidx8 to i64*
  %tmp5 = load i64, i64* %tmp4, align 8
  %arrayidx10 = getelementptr inbounds double, double* %B, i64 %indvars.iv
  %tmp6 = bitcast double* %arrayidx10 to i64*
  store i64 %tmp5, i64* %tmp6, align 8
  br label %for.inc11

for.inc11:                                        ; preds = %for.body6
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond4

for.end13:                                        ; preds = %for.cond4
  ret void
}
