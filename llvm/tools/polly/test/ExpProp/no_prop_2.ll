; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        int x = A[i];
;        // split
;        int y = x + 3;
;        // split
;        A[i] = 0;
;        // split
;        B[i] = y;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body_split0
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split0[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split0[i0] -> [i0, 0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split0[i0] -> MemRef_A[i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split0[i0] -> MemRef_tmp_reg2mem[0] };
; CHECK:    	Stmt_for_body_split1
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split1[i0] : 1 = 0 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split1[i0] -> [0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split1[i0] -> MemRef_tmp_reg2mem[0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split1[i0] -> MemRef_add_reg2mem[0] };
; CHECK:    	Stmt_for_body_split2
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split2[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split2[i0] -> [i0, 1] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split2[i0] -> MemRef_A[i0] };
; CHECK:    	Stmt_for_body_split3	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split3[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split3[i0] -> [i0, 2] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split3[i0] -> MemRef_B[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_for_body_split3[i0] -> MemRef_tmp_reg2mem[0] };
; CHECK:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/no_prop_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body.split0, label %for.end

for.body.split0:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  br label %for.body.split1

for.body.split1:
  %add = add nsw i32 %tmp, 3
  br label %for.body.split2

for.body.split2:
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 0, i32* %arrayidx2, align 4
  br label %for.body.split3

for.body.split3:
  %arrayidx4 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
