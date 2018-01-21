; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *restrict B, int A[100]) {
;      int N = 100;
;
;      for (int i = 0; i < N; i++)
;        A[i] = i;
;      for (int i = 0; i < N - 1; i++)
;        B[i] = A[i + 1] * 2;
;    }
;
; IR: polly.stmt.for.body4:                             ; preds = %polly.loop_header1
; IR:   %1 = add nsw i64 %polly.indvar4, 1
; IR:   %p_tmp = trunc i64 %1 to i32
; IR:   %p_mul = shl nsw i32 %p_tmp, 1
; IR:   %scevgep7 = getelementptr i32, i32* %B, i64 %polly.indvar4
; IR:   store i32 %p_mul, i32* %scevgep7
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/phi_recompute_3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  %tmp = trunc i64 %indvars.iv1 to i32
  store i32 %tmp, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc9, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc9 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 99
  br i1 %exitcond, label %for.body4, label %for.end11

for.body4:                                        ; preds = %for.cond2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx6 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv.next
  %tmp4 = load i32, i32* %arrayidx6, align 4
  %mul = shl nsw i32 %tmp4, 1
  %arrayidx8 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %mul, i32* %arrayidx8, align 4
  br label %for.inc9

for.inc9:                                         ; preds = %for.body4
  br label %for.cond2

for.end11:                                        ; preds = %for.cond2
  ret void
}
