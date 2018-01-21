; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST_DCE
;
;    void f(int *restrict B, int A[100][100]) {
;      int N = 100;
;      for (int i = 0; i < N / 2; i++)
;        A[i][2 * i] = B[i];
;      for (int i = 0; i < N; i++)
;        for (int j = 0; j < N; j++)
;          A[i][j] = A[i][j] + 3;
;    }
;
; AST_DCE: if (1)
;
; AST_DCE-NOT: Stmt_for_body
; AST_DCE:    for (int c0 = 0; c0 <= 99; c0 += 1)
; AST_DCE:      for (int c1 = 0; c1 <= 99; c1 += 1) {
; AST_DCE:        if (c1 == 2 * c0) {
; AST_DCE:          Stmt_for_body11(c0, 2 * c0);
; AST_DCE:        } else {
; AST_DCE:          Stmt_for_body11_0(c0, c1);
; AST_DCE:        }
; AST_DCE:      }

;
source_filename = "../polly/test/ExpProp/simple5.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* noalias %B, [100 x i32]* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc ], [ 0, %entry ]
  %exitcond7 = icmp ne i64 %indvars.iv4, 50
  br i1 %exitcond7, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %B, i64 %indvars.iv4
  %tmp = load i32, i32* %arrayidx, align 4
  %tmp8 = shl nsw i64 %indvars.iv4, 1
  %arrayidx4 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv4, i64 %tmp8
  store i32 %tmp, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc23, %for.end
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc23 ], [ 0, %for.end ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body8, label %for.end25

for.body8:                                        ; preds = %for.cond6
  br label %for.cond9

for.cond9:                                        ; preds = %for.inc20, %for.body8
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc20 ], [ 0, %for.body8 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body11, label %for.end22

for.body11:                                       ; preds = %for.cond9
  %arrayidx15 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv1, i64 %indvars.iv
  %tmp9 = load i32, i32* %arrayidx15, align 4
  %add = add nsw i32 %tmp9, 3
  %arrayidx19 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv1, i64 %indvars.iv
  store i32 %add, i32* %arrayidx19, align 4
  br label %for.inc20

for.inc20:                                        ; preds = %for.body11
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond9

for.end22:                                        ; preds = %for.cond9
  br label %for.inc23

for.inc23:                                        ; preds = %for.end22
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond6

for.end25:                                        ; preds = %for.cond6
  ret void
}
