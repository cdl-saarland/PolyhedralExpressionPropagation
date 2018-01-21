; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *A, int *B, int *C) {
;      for (int i = 0; i < 100; i++) {
;        A[i] = i;
;        if (i > 50)
;          A[i] *= 2;
;        // split
;        B[i] = A[i] + 2;
;      }
;    }
;
; AST:   for (int c0 = 0; c0 <= 99; c0 += 1) {
; AST:      if (c0 <= 50) {
; AST:        Stmt_for_body(c0);
; AST:        Stmt_if_end(c0);
; AST:      } else {
; AST:        Stmt_if_then(c0);
; AST:        Stmt_if_end_0(c0);
; AST:      }
; AST:    }
;
; IR: polly.then:                                       ; preds = %polly.cond
; IR:   br label %polly.stmt.for.body
;
; IR: polly.stmt.for.body:                              ; preds = %polly.then
; IR:   %scevgep = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:   %9 = trunc i64 %polly.indvar to i32
; IR:   store i32 %9, i32* %scevgep, align 4, !alias.scope !0, !noalias !2
; IR:   br label %polly.stmt.if.end
;
; IR: polly.stmt.if.end:
; IR:   %p_tmp = trunc i64 %polly.indvar to i32
; IR:   %p_add = add nsw i32 %p_tmp, 2
; IR:   %scevgep3 = getelementptr i32, i32* %B, i64 %polly.indvar
; IR:   store i32 %p_add, i32* %scevgep3, align 4, !alias.scope !3, !noalias !4
; IR:   br label %polly.merge
;
; IR: polly.else:                                       ; preds = %polly.cond
; IR:   br label %polly.stmt.if.then
;
; IR: polly.stmt.if.then:                               ; preds = %polly.else
; IR:   %p_tmp4 = trunc i64 %polly.indvar to i32
; IR:   %p_mul = shl nsw i32 %p_tmp4, 1
; IR:   %scevgep5 = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:   store i32 %p_mul, i32* %scevgep5, align 4, !alias.scope !0, !noalias !2
; IR:   br label %polly.stmt.if.end6
;
; IR: polly.stmt.if.end6:                               ; preds = %polly.stmt.if.then
; IR:   %p_tmp7 = trunc i64 %polly.indvar to i32
; IR:   %p_mul8 = shl nsw i32 %p_tmp7, 1
; IR:   %p_add9 = add nsw i32 %p_mul8, 2
; IR:   %scevgep10 = getelementptr i32, i32* %B, i64 %polly.indvar
; IR:   store i32 %p_add9, i32* %scevgep10, align 4, !alias.scope !3, !noalias !4
; IR:   br label %polly.merge



source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_chain_3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = trunc i64 %indvars.iv to i32
  store i32 %tmp, i32* %arrayidx, align 4
  %cmp1 = icmp sgt i64 %indvars.iv, 50
  br i1 %cmp1, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %arrayidx3 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx3, align 4
  %mul = shl nsw i32 %tmp1, 1
  store i32 %mul, i32* %arrayidx3, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %arrayidx5 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx5, align 4
  %add = add nsw i32 %tmp2, 2
  %arrayidx7 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %add, i32* %arrayidx7, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
