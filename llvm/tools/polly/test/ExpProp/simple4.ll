; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *restrict B, int A[100][100]) {
;      int N = 100;
;      for (int i = 0; i < N; i++)
;        for (int j = 0; j < N; j++)
;          A[i][j] = i + j;
;      for (int i = 1; i < N; i++)
;        B[i] = A[i][i - 1] + 2;
;    }
;
; IR: polly.stmt.for.body12:
; IR:   %3 = add nsw i64 %polly.indvar13, 1
; IR:   %p_tmp = add nuw nsw i64 %3, %polly.indvar13
; IR:   %p_tmp9 = trunc i64 %p_tmp to i32
; IR:   %p_add17 = add nsw i32 %p_tmp9, 2
; IR:   %scevgep17 = getelementptr i32, i32* %scevgep16, i64 %polly.indvar13
; IR:   store i32 %p_add17, i32* %scevgep17
;
source_filename = "../polly/test/ExpProp/simple4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, [100 x i32]* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc6, %entry
  %indvars.iv6 = phi i64 [ %indvars.iv.next7, %for.inc6 ], [ 0, %entry ]
  %exitcond8 = icmp ne i64 %indvars.iv6, 100
  br i1 %exitcond8, label %for.body, label %for.end8

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %indvars.iv2 = phi i64 [ %indvars.iv.next3, %for.inc ], [ 0, %for.body ]
  %exitcond5 = icmp ne i64 %indvars.iv2, 100
  br i1 %exitcond5, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %tmp = add nuw nsw i64 %indvars.iv6, %indvars.iv2
  %arrayidx5 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv6, i64 %indvars.iv2
  %tmp9 = trunc i64 %tmp to i32
  store i32 %tmp9, i32* %arrayidx5, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %indvars.iv.next3 = add nuw nsw i64 %indvars.iv2, 1
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  br label %for.inc6

for.inc6:                                         ; preds = %for.end
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  br label %for.cond

for.end8:                                         ; preds = %for.cond
  br label %for.cond10

for.cond10:                                       ; preds = %for.inc20, %for.end8
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc20 ], [ 1, %for.end8 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body12, label %for.end22

for.body12:                                       ; preds = %for.cond10
  %tmp10 = add nsw i64 %indvars.iv, -1
  %arrayidx16 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv, i64 %tmp10
  %tmp11 = load i32, i32* %arrayidx16, align 4
  %add17 = add nsw i32 %tmp11, 2
  %arrayidx19 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add17, i32* %arrayidx19, align 4
  br label %for.inc20

for.inc20:                                        ; preds = %for.body12
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond10

for.end22:                                        ; preds = %for.cond10
  ret void
}
