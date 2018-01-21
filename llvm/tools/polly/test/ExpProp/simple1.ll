; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s --check-prefix=SCOP
; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -polly-ast -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *A) {
;      for (int i = 0; i < 100; i++)
;        A[i] = 1;
;      for (int i = 0; i < 100; i++)
;        A[i] = A[i] + 1;
;    }
;
; SCOP-NOT: ReadAccess
; SCOP:     [#Recompute Values: 1]
; SCOP-NOT: ReadAccess
;
; AST: if (1)
; AST-NOT: Stmt_for_body
; AST:          for (int c0 = 0; c0 <= 99; c0 += 1)
; AST:            Stmt_for_loop2(c0);
; AST-NOT: Stmt_for_body
; AST:      else
;
; IR:      polly.stmt.for.loop2:
; IR-NEXT:   %p_add = add nsw i32 1, 1
; IR-NEXT:   %scevgep = getelementptr i32, i32* %A, i64 %polly.indvar
; IR-NEXT:   store i32 %p_add, i32* %scevgep, align 4, !alias.scope !0, !noalias !2
;
source_filename = "../polly/test/ExpProp/simple1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc ], [ 0, %entry ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  store i32 1, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc9, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc9 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.loop2, label %for.end11

for.loop2:                                        ; preds = %for.cond2
  %arrayidx6 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx6, align 4
  %add = add nsw i32 %tmp, 1
  %arrayidx8 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  store i32 %add, i32* %arrayidx8, align 4
  br label %for.inc9

for.inc9:                                         ; preds = %for.loop2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond2

for.end11:                                        ; preds = %for.cond2
  ret void
}
