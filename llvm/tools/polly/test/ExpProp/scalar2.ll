; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int B[100], int A[100]) {
;      int N = 100;
;      int x = 0;
;      for (int i = 1; i < N; i++) {
;        B[i] = x;
;        x = 1;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body[i0] : 0 < i0 <= 98 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body[i0] -> MemRef_B[1 + i0] };
; CHECK:    	Stmt_for_body_0	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_0[0] };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_0[i0] -> [0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_0[i0] -> MemRef_B[1 + i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_0[i0] -> MemRef_x_0_reg2mem[0] };
; CHECK:    }
;
; IR: polly.stmt.for.body:
; IR:   %scevgep1 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; IR:   store i32 1, i32* %scevgep1
;
source_filename = "../polly/test/ExpProp/scalar2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %x.0 = phi i32 [ 0, %entry ], [ 1, %for.inc ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %x.0, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
