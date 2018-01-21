; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=2 -polly-ast -analyze < %s | FileCheck %s
;
;    void f(double *C, double *D, double *E, int L) {
;      for (int i = 0; i < L; i++)
;        for (int j = 0; j < L; j++) {
;          double sum = 0;
;          for (int k = 0; k < L; k++)
; I:         sum += C[L * i + k] * D[L * k + j];
; W:       E[L * i + j] = sum;
;        }
;    }
;
; CHECK:    for (int c0 = 0; c0 < L; c0 += 1)
; CHECK:      for (int c1 = 0; c1 < L; c1 += 1) {
; CHECK:        Stmt_for_body3(c0, c1);
; CHECK:        for (int c2 = 0; c2 < L - 1; c2 += 1)
; CHECK:          Stmt_I(c0, c1, c2);
; CHECK:        Stmt_W(c0, c1);
; CHECK:      }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/matmul.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(double* %C, double* %D, double* %E, i32 %L) {
entry:
  %tmp = sext i32 %L to i64
  %tmp13 = sext i32 %L to i64
  %tmp14 = sext i32 %L to i64
  %tmp15 = sext i32 %L to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc20, %entry
  %indvars.iv9 = phi i64 [ %indvars.iv.next10, %for.inc20 ], [ 0, %entry ]
  %cmp = icmp slt i64 %indvars.iv9, %tmp13
  br i1 %cmp, label %for.body, label %for.end22

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc17, %for.body
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc17 ], [ 0, %for.body ]
  %wide.trip.count7 = zext i32 %L to i64
  %exitcond8 = icmp ne i64 %indvars.iv4, %wide.trip.count7
  br i1 %exitcond8, label %for.body3, label %for.end19

for.body3:                                        ; preds = %for.cond1
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc, %for.body3
  %indvars.iv = phi i64 [ %indvars.iv.next, %I ], [ 0, %for.body3 ]
  %sum.0 = phi double [ 0.000000e+00, %for.body3 ], [ %add12, %I ]
  %wide.trip.count = zext i32 %L to i64
  %exitcond = icmp ne i64 %indvars.iv, %wide.trip.count
  br i1 %exitcond, label %for.body6, label %W

for.body6:                                        ; preds = %for.cond4
  br label %I

I:                                          ; preds = %for.body6
  %tmp16 = mul nsw i64 %indvars.iv9, %tmp15
  %tmp17 = add nsw i64 %tmp16, %indvars.iv
  %arrayidx = getelementptr inbounds double, double* %C, i64 %tmp17
  %tmp18 = load double, double* %arrayidx, align 8
  %tmp19 = mul nsw i64 %indvars.iv, %tmp
  %tmp20 = add nsw i64 %tmp19, %indvars.iv4
  %arrayidx10 = getelementptr inbounds double, double* %D, i64 %tmp20
  %tmp21 = load double, double* %arrayidx10, align 8
  %mul11 = fmul fast double %tmp18, %tmp21
  %add12 = fadd fast double %sum.0, %mul11
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond4

W:
  %sum.0.lcssa = phi double [ %sum.0, %for.cond4 ]
  %tmp22 = mul nsw i64 %indvars.iv9, %tmp14
  %tmp23 = add nsw i64 %tmp22, %indvars.iv4
  %arrayidx16 = getelementptr inbounds double, double* %E, i64 %tmp23
  store double %sum.0.lcssa, double* %arrayidx16, align 8
  br label %for.inc17

for.inc17:                                        ; preds = %for.end
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond1

for.end19:                                        ; preds = %for.cond1
  br label %for.inc20

for.inc20:                                        ; preds = %for.end19
  %indvars.iv.next10 = add nuw nsw i64 %indvars.iv9, 1
  br label %for.cond

for.end22:                                        ; preds = %for.cond
  ret void
}
