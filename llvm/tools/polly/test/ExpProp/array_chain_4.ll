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
;        // split
;        C[i] = B[i] + A[i] + 3;
;      }
;    }
;
; AST:         for (int c0 = 0; c0 <= 99; c0 += 1) {
; AST-NEXT:      if (c0 <= 50) {
; AST-NEXT:        Stmt_for_body(c0);
; AST-NEXT:        Stmt_if_end(c0);
; AST-NEXT:        Stmt_BB_C(c0);
; AST-NEXT:      } else {
; AST-NEXT:        Stmt_if_then(c0);
; AST-NEXT:        Stmt_if_end_0(c0);
; AST-NEXT:        Stmt_BB_C_0(c0);
; AST-NEXT:      }
; AST-NEXT:    }
;
; i + (i+2) + 3
; IR: polly.stmt.BB_C:
; IR:   %p_tmp11 = trunc i64 %polly.indvar to i32
; IR:   %p_tmp12 = trunc i64 %polly.indvar to i32
; IR:   %p_add13 = add nsw i32 %p_tmp12, 2
; IR:   %p_add12 = add nsw i32 %p_add13, %p_tmp11
; IR:   %p_add1314 = add nsw i32 %p_add12, 3
; IR:   %scevgep15 = getelementptr i32, i32* %C, i64 %polly.indvar
; IR:   store i32 %p_add1314, i32* %scevgep15
;
; (2*i) + (i+2) + 3
; IR: polly.stmt.BB_C23:
; IR:  %p_tmp24 = trunc i64 %polly.indvar to i32
; IR:  %p_mul25 = shl nsw i32 %p_tmp24, 1
; IR:  %p_tmp26 = trunc i64 %polly.indvar to i32
; IR:  %p_mul27 = shl nsw i32 %p_tmp26, 1
; IR:  %p_add28 = add nsw i32 %p_mul27, 2
; IR:  %p_add1229 = add nsw i32 %p_add28, %p_mul25
; IR:  %p_add1330 = add nsw i32 %p_add1229, 3
; IR:  %scevgep31 = getelementptr i32, i32* %C, i64 %polly.indvar
; IR:  store i32 %p_add1330, i32* %scevgep31
;
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
  br label %BB_C

BB_C:
  %arrayidx9 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx9, align 4
  %arrayidx11 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp4 = load i32, i32* %arrayidx11, align 4
  %add12 = add nsw i32 %tmp3, %tmp4
  %add13 = add nsw i32 %add12, 3
  %arrayidx15 = getelementptr inbounds i32, i32* %C, i64 %indvars.iv
  store i32 %add13, i32* %arrayidx15, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
