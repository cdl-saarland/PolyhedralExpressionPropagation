; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *In, int *A, int *B, int *C) {
;      for (int i = 0; i < 100; i++) {
;        A[i] = In[i];
;        // split
;        B[i] = A[i] + 2;
;        // split
;        C[i] = B[i] + A[i] + 3;
;      }
;    }
;
; IR: polly.stmt.BB_A:                                  ; preds = %polly.loop_header
; IR:   %scevgep = getelementptr i32, i32* %In, i64 %polly.indvar
; IR:   %tmp_p_scalar_ = load i32, i32* %scevgep
; IR:   %scevgep21 = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:   store i32 %tmp_p_scalar_, i32* %scevgep21
;
; IR: polly.stmt.BB_B:                                  ; preds = %polly.stmt.BB_A
; IR:   %p_arrayidx = getelementptr inbounds i32, i32* %In, i64 %polly.indvar
; IR:   %p_tmp = load i32, i32* %p_arrayidx, align 4, !alias.scope !0, !noalias !2
; IR:   %p_add = add nsw i32 %p_tmp, 2
; IR:   %scevgep22 = getelementptr i32, i32* %B, i64 %polly.indvar
; IR:   store i32 %p_add, i32* %scevgep22
;
; IR: polly.stmt.BB_C:                                  ; preds = %polly.stmt.BB_B
; IR:   %p_arrayidx23 = getelementptr inbounds i32, i32* %In, i64 %polly.indvar
; IR:   %p_tmp24 = load i32, i32* %p_arrayidx23, align 4, !alias.scope !0, !noalias !2
; IR:   %p_arrayidx25 = getelementptr inbounds i32, i32* %In, i64 %polly.indvar
; IR:   %p_tmp26 = load i32, i32* %p_arrayidx25, align 4, !alias.scope !0, !noalias !2
; IR:   %p_add27 = add nsw i32 %p_tmp26, 2
; IR:   %p_add11 = add nsw i32 %p_add27, %p_tmp24
; IR:   %p_add12 = add nsw i32 %p_add11, 3
; IR:   %scevgep28 = getelementptr i32, i32* %C, i64 %polly.indvar
; IR:   store i32 %p_add12, i32* %scevgep28
;

;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_chain_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %In, i32* %A, i32* %B, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %BB_A, label %for.end

BB_A:
  %arrayidx = getelementptr inbounds i32, i32* %In, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 %tmp, i32* %arrayidx2, align 4
  br label %BB_B
BB_B:
  %arrayidx4 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx4, align 4
  %add = add nsw i32 %tmp1, 2
  %arrayidx6 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add, i32* %arrayidx6, align 4
  br label %BB_C
BB_C:
  %arrayidx8 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx10, align 4
  %add11 = add nsw i32 %tmp2, %tmp3
  %add12 = add nsw i32 %add11, 3
  %arrayidx14 = getelementptr inbounds i32, i32* %C, i64 %indvars.iv
  store i32 %add12, i32* %arrayidx14, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
