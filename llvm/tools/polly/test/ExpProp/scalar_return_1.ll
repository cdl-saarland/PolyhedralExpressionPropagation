; RUN: opt %loadPolly -polly-prepare -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-prepare -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
;
;    int f(int *A, int *B) {
;      int x = 21;
;      for (int i = 0; i < 100; i++) {
;        x = A[i];
;        B[i] = A[i];
;      }
;      return x;
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_cond
; CHECK:            Domain :=
; CHECK:                { Stmt_for_cond[100] };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_cond[i0] -> [100, 0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_cond[i0] -> MemRef_x_0_reload_reg2mem[0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_for_cond[i0] -> MemRef_A[99] };
; CHECK:    	Stmt_for_body_split
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split[i0] -> [i0, 1] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split[i0] -> MemRef_A[i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split[i0] -> MemRef_B[i0] };
; CHECK:    	Stmt_for_inc	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_inc[i0] : 1 = 0 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_inc[i0] -> [0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_inc[i0] -> MemRef_x_0_reg2mem[0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_for_inc[i0] -> MemRef_A[i0] };
; CHECK:    }
;
; AST:         if (1
; AST:         {
; AST-NEXT:      for (int c0 = 0; c0 <= 99; c0 += 1)
; AST-NEXT:        Stmt_for_body_split(c0);
; AST-NEXT:      Stmt_for_cond(100);
; AST-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_return.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define i32 @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %x.0 = phi i32 [ 21, %entry ], [ %tmp, %for.inc ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx2, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp1, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi i32 [ %x.0, %for.cond ]
  ret i32 %x.0.lcssa
}
