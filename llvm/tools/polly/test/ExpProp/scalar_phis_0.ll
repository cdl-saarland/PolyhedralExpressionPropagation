; RUN: opt -polly-process-unprofitable -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt -polly-process-unprofitable -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
; XFAIL: *
;
;    double f(double *A) {
;      double r, d = 5;
;      for (long i = 0; i < 1000; i++) {
;        for (long j = 0; j < 1000; j++) {
;          d = d + d * 3;
;        }
;        d += 2;
;        r = d * 3;
;      }
;      return r;
;    }
;
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define double @f(double* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc6, %entry
  %i.0 = phi i64 [ 0, %entry ], [ %inc7, %for.inc6 ]
  %d.0 = phi double [ 5.000000e+00, %entry ], [ %add4, %for.inc6 ]
  %r.0 = phi double [ undef, %entry ], [ %mul5, %for.inc6 ]
  %exitcond1 = icmp ne i64 %i.0, 1000
  br i1 %exitcond1, label %for.body, label %for.end8

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %d.1 = phi double [ %d.0, %for.body ], [ %tmp, %for.inc ]
  %j.0 = phi i64 [ 0, %for.body ], [ %inc, %for.inc ]
  %exitcond = icmp ne i64 %j.0, 1000
  br i1 %exitcond, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %tmp = fmul fast double %d.1, 4.000000e+00
  %inc = add nuw nsw i64 %j.0, 1
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  %d.1.lcssa = phi double [ %d.1, %for.cond1 ]
  %add4 = fadd fast double %d.1.lcssa, 2.000000e+00
  br label %for.inc6

for.inc6:                                         ; preds = %for.end
  %mul5 = fmul fast double %add4, 3.000000e+00
  %inc7 = add nuw nsw i64 %i.0, 1
  br label %for.cond

for.end8:                                         ; preds = %for.cond
  %r.0.lcssa = phi double [ %r.0, %for.cond ]
  ret double %r.0.lcssa
}
