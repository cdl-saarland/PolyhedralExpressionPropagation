; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-opt-isl -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *A, int *B, int *C) {
;      for (int i = 0; i < 100; i++) {
;        A[i] = i;
;        // split
;        B[i] = A[i] + 2;
;        // split
;        C[i] = B[i] + A[i] + 3;
;      }
;    }
;
; IR: polly.stmt.BB_B:
; IR:   %p_tmp = trunc i64 %polly.indvar13 to i32
; IR:   %p_add = add nsw i32 %p_tmp, 2
; IR:   %scevgep16 = getelementptr i32, i32* %B, i64 %polly.indvar13
; IR:   store i32 %p_add, i32* %scevgep16
;
; IR: polly.stmt.BB_C:
; IR:  %p_tmp23 = trunc i64 %polly.indvar20 to i32
; IR:  %p_tmp24 = trunc i64 %polly.indvar20 to i32
; IR:  %p_add25 = add nsw i32 %p_tmp24, 2
; IR:  %p_add9 = add nsw i32 %p_add25, %p_tmp23
; IR:  %p_add10 = add nsw i32 %p_add9, 3
; IR:   %scevgep26 = getelementptr i32, i32* %C, i64 %polly.indvar20
; IR:   store i32 %p_add10, i32* %scevgep26
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_chain_1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %BB_A, label %for.end

BB_A:
  %arrayidxA = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = trunc i64 %indvars.iv to i32
  store i32 %tmp, i32* %arrayidxA, align 4
  br label %BB_B
BB_B:
  %arrayidxA2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidxA2, align 4
  %add = add nsw i32 %tmp1, 2
  %arrayidxB = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add, i32* %arrayidxB, align 4
  br label %BB_C
BB_C:
  %arrayidxB2 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidxB2, align 4
  %arrayidxA3 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidxA3, align 4
  %add9 = add nsw i32 %tmp2, %tmp3
  %add10 = add nsw i32 %add9, 3
  %arrayidxC = getelementptr inbounds i32, i32* %C, i64 %indvars.iv
  store i32 %add10, i32* %arrayidxC, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
