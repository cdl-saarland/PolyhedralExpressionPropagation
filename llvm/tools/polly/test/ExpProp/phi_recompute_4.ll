; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *restrict B, int A[100]) {
;      int N = 100;
;
;      for (int i = 0; i < N; i++)
;        A[i] = 5 * i;
;      for (int i = 3; i < N; i++)
;        B[i] = A[i - 3] * 2;
;    }
;
; IR: polly.stmt.for.body4:                             ; preds = %polly.loop_header1
; IR:   %p_tmp = mul nuw nsw i64 %polly.indvar4, 5
; IR:   %p_tmp6 = trunc i64 %p_tmp to i32
; IR:   %p_mul7 = shl nsw i32 %p_tmp6, 1
; IR:   %scevgep8 = getelementptr i32, i32* %scevgep7, i64 %polly.indvar4
; IR:   store i32 %p_mul7, i32* %scevgep8
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/phi_recompute_4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv2 = phi i64 [ %indvars.iv.next3, %for.inc ], [ 0, %entry ]
  %exitcond5 = icmp ne i64 %indvars.iv2, 100
  br i1 %exitcond5, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp = mul nuw nsw i64 %indvars.iv2, 5
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv2
  %tmp6 = trunc i64 %tmp to i32
  store i32 %tmp6, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next3 = add nuw nsw i64 %indvars.iv2, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc10, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc10 ], [ 3, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body4, label %for.end12

for.body4:                                        ; preds = %for.cond2
  %tmp7 = add nsw i64 %indvars.iv, -3
  %arrayidx6 = getelementptr inbounds i32, i32* %A, i64 %tmp7
  %tmp8 = load i32, i32* %arrayidx6, align 4
  %mul7 = shl nsw i32 %tmp8, 1
  %arrayidx9 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %mul7, i32* %arrayidx9, align 4
  br label %for.inc10

for.inc10:                                        ; preds = %for.body4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end12:                                        ; preds = %for.cond2
  ret void
}
