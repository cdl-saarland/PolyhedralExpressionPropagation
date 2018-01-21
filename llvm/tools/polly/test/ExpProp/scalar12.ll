; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(double A[100]) {
;      double x = 0;
;      for (int i = 0; i < 100; i++) {
;   S:   x += 3.0 * i;
;      }
;      for (int i = 0; i < 100; i++) {
;   P:   A[i] = x;
;      }
;    }
;
; CHECK:    Statements {
; CHECK:    	Stmt_for_inc	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_for_inc[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_for_inc[i0] -> [0, i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_for_inc[i0] -> MemRef_x_0_reg2mem[0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_for_inc[i0] -> MemRef_x_0_reg2mem[0] };
; CHECK:    	Stmt_P	 [#Recompute Values: 1]
; CHECK:            Domain :=
; CHECK:                { Stmt_P[i0] : 0 <= i0 <= 99 };
; CHECK:            Schedule :=
; CHECK:                { Stmt_P[i0] -> [1, i0] };
; CHECK:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                { Stmt_P[i0] -> MemRef_A[i0] };
; CHECK:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK:                null;
; CHECK:           new: { Stmt_P[i0] -> MemRef_x_0_reg2mem[0] };
; CHECK:    }
;

; IR:        %x.0.reg2mem = alloca double
; IR-NEXT:   store double 0.000000e+00, double* %x.0.reg2mem
;
; IR:      polly.stmt.for.inc:
; IR-NEXT:   %polly.access.x.0.reg2mem = getelementptr double, double* %x.0.reg2mem, i64 0
; IR-NEXT:   %x.0.reload_p_scalar_ = load double, double* %polly.access.x.0.reg2mem
; IR-NEXT:   %0 = trunc i64 %polly.indvar to i32
; IR-NEXT:   %p_conv = sitofp i32 %0 to double
; IR-NEXT:   %p_mul = fmul double %p_conv, 3.000000e+00
; IR-NEXT:   %p_add = fadd double %x.0.reload_p_scalar_, %p_mul
; IR-NEXT:   store double %p_add, double* %x.0.reg2mem
;
; IR:      polly.stmt.P:
; IR-NEXT:   %polly.access.x.0.reg2mem8 = getelementptr double, double* %x.0.reg2mem, i64 0
; IR-NEXT:   %x.0.reload_p_scalar_9 = load double, double* %polly.access.x.0.reg2mem8
; IR-NEXT:   %scevgep = getelementptr double, double* %A, i64 %polly.indvar5
; IR-NEXT:   store double %x.0.reload_p_scalar_9, double* %scevgep
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar12.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi double [ 0.000000e+00, %entry ], [ %add, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, 100
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S

S:                                                ; preds = %for.body
  br label %for.inc

for.inc:                                          ; preds = %S
  %conv = sitofp i32 %i.0 to double
  %mul = fmul double %conv, 3.000000e+00
  %add = fadd double %x.0, %mul
  %inc = add nuw nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi double [ %x.0, %for.cond ]
  %tmp = sext i32 100 to i64
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc6, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc6 ], [ 0, %for.end ]
  %cmp3 = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp3, label %for.body5, label %for.end8

for.body5:                                        ; preds = %for.cond2
  br label %P

P:                                                ; preds = %for.body5
  %arrayidx = getelementptr inbounds double, double* %A, i64 %indvars.iv
  store double %x.0.lcssa, double* %arrayidx, align 8
  br label %for.inc6

for.inc6:                                         ; preds = %P
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end8:                                         ; preds = %for.cond2
  ret void
}
