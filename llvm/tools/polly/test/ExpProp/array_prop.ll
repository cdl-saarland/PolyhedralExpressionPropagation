; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s
;
;    void f(int *A, int *B) {
;      for (int i = 0; i < 100; i++) {
;        int x = A[i];
;        // split
;        B[i] = 0;
;        // split
;        B[i] = x;
;      }
;    }
;
; AST:         for (int c0 = 0; c0 <= 99; c0 += 1)
; AST-NEXT:      Stmt_for_body2(c0);
; AST-NOT:       Stmt
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/array_non_prop.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  br label %for.body1

for.body1:                                         ; preds = %for.cond
  %arrayidx2 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 0, i32* %arrayidx2, align 4
  br label %for.body2

for.body2:                                         ; preds = %for.cond
  %arrayidx4 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
