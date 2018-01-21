; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double *A, double b, double c) {
;      for (int i = 0; i < 100; i++) {
;        double a = 213.23;
;        a += b;
;        // split
;        a += c;
;        // split
;        A[i] = a;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_body_split1	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_body_split1[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_body_split1[i0] -> [i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_body_split1[i0] -> MemRef_A[i0] };
; CHECK-DAG:           new: { Stmt_for_body_split1[i0] -> MemRef_c[] };
; CHECK-DAG:           new: { Stmt_for_body_split1[i0] -> MemRef_b[] };
; CHECK:    }
;
; IR: polly.stmt.for.body.split1:                       ; preds = %polly.loop_header
; IR:   %p_add = fadd double %b, 2.132300e+02
; IR:   %p_add1 = fadd double %p_add, %c
; IR:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar
; IR:   store double %p_add1, double* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_chain.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A, double %b, double %c) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = fadd double %b, 2.132300e+02
  br label %for.body.split0

for.body.split0:
  %add1 = fadd double %add, %c
  br label %for.body.split1

for.body.split1:
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %add1, double* %arrayidx, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
