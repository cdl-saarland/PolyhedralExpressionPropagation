; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s --check-prefix=SCOP
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        int x = A[i];
;        // split
;        A[98] = 0;
;        // split
;        B[i] = x;
;      }
;    }
;
; SCOP:         Statements {
; SCOP-NEXT:    	Stmt_for_body
; SCOP-NEXT:            Domain :=
; SCOP-NEXT:                { Stmt_for_body[i0] : 1 = 0 };
; SCOP-NEXT:            Schedule :=
; SCOP-NEXT:                { Stmt_for_body[i0] -> [0] };
; SCOP-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                { Stmt_for_body[i0] -> MemRef_A[i0] };
; SCOP-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                { Stmt_for_body[i0] -> MemRef_tmp_reg2mem[0] };
; SCOP-NEXT:    	Stmt_for_body_split
; SCOP-NEXT:            Domain :=
; SCOP-NEXT:                { Stmt_for_body_split[99] };
; SCOP-NEXT:            Schedule :=
; SCOP-NEXT:                { Stmt_for_body_split[i0] -> [99, 0] };
; SCOP-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                { Stmt_for_body_split[i0] -> MemRef_A[98] };
; SCOP-NEXT:    	Stmt_for_body_split_split	 [#Recompute Values: 1]
; SCOP-NEXT:            Domain :=
; SCOP-NEXT:                { Stmt_for_body_split_split[98] }; CopyDom: { Stmt_for_body_split_split[i0] : 0 <= i0 <= 99 };
; SCOP-NEXT:            Schedule :=
; SCOP-NEXT:                { Stmt_for_body_split_split[i0] -> [98, 1] };
; SCOP-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                { Stmt_for_body_split_split[i0] -> MemRef_B[i0] };
; SCOP-NEXT:    	Stmt_for_body_split_split_0	 [#Recompute Values: 1]
; SCOP-NEXT:            Domain :=
; SCOP-NEXT:                { Stmt_for_body_split_split_0[i0] : 0 <= i0 <= 97; Stmt_for_body_split_split_0[99] };
; SCOP-NEXT:            Schedule :=
; SCOP-NEXT:                { Stmt_for_body_split_split_0[i0] -> [i0, 1] : i0 <= 97; Stmt_for_body_split_split_0[99] -> [99, 1] };
; SCOP-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                { Stmt_for_body_split_split_0[i0] -> MemRef_B[i0] };
; SCOP-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; SCOP-NEXT:                null;
; SCOP-NEXT:           new: { Stmt_for_body_split_split_0[i0] -> MemRef_A[i0] };
; SCOP-NEXT:    }
;
; CHECK: if (1
;
; CHECK:         for (int c0 = 0; c0 <= 99; c0 += 1) {
; CHECK-NEXT:      if (c0 == 99)
; CHECK-NEXT:        Stmt_for_body_split(99);
; CHECK-NEXT:      if (c0 <= 97 || c0 == 99)
; CHECK-NEXT:        Stmt_for_body_split_split_0(c0);
; CHECK-NEXT:      } else {
; CHECK-NEXT:        Stmt_for_body_split_split(98);
; CHECK-NEXT:      }
; CHECK-NEXT:    }

source_filename = "test/ExpProp/dep_direction2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  br label %for.body.split

for.body.split:
  %arrayidx3 = getelementptr inbounds i32, i32* %A, i64 98
  store i32 0, i32* %arrayidx3, align 4
  br label %for.body.split_split

for.body.split_split:
  %arrayidx2 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp, i32* %arrayidx2, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
