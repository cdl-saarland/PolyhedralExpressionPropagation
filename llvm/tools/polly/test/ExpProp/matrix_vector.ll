; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
;
;    void f(int A[100][100], int c, int d) {
;      int Tmp[100][100];
;      for (int i = 0; i < 100; i++)
;        for (int j = 0; j < 100; j++)
;          Tmp[i][j] = A[i][j] * c;
;      for (int i = 0; i < 100; i++)
;        for (int j = 0; j < 100; j++)
;          A[i][j] = Tmp[i][j] + d;
;    }
;
; AST: if (1)

; AST-NOT: Stmt_for_body3

; AST:     for (int c0 = 0; c0 <= 99; c0 += 1)
; AST:       for (int c1 = 0; c1 <= 99; c1 += 1)
; AST:         Stmt_for_body20(c0, c1);

;
source_filename = "matrix_vector.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f([100 x i32]* %A, i32 %c, i32 %d) {
entry:
  %Tmp = alloca [100 x [100 x i32]], align 16
  br label %for.cond

for.cond:                                         ; preds = %for.inc10, %entry
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc10 ], [ 0, %entry ]
  %exitcond9 = icmp ne i64 %indvars.iv7, 100
  br i1 %exitcond9, label %for.body, label %for.end12

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc ], [ 0, %for.body ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 100
  br i1 %exitcond6, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %arrayidx5 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv7, i64 %indvars.iv4
  %tmp = load i32, i32* %arrayidx5, align 4
  %mul = mul nsw i32 %tmp, %c
  %arrayidx9 = getelementptr inbounds [100 x [100 x i32]], [100 x [100 x i32]]* %Tmp, i64 0, i64 %indvars.iv7, i64 %indvars.iv4
  store i32 %mul, i32* %arrayidx9, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  br label %for.inc10

for.inc10:                                        ; preds = %for.end
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond

for.end12:                                        ; preds = %for.cond
  br label %for.cond14

for.cond14:                                       ; preds = %for.inc32, %for.end12
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc32 ], [ 0, %for.end12 ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body16, label %for.end34

for.body16:                                       ; preds = %for.cond14
  br label %for.cond18

for.cond18:                                       ; preds = %for.inc29, %for.body16
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc29 ], [ 0, %for.body16 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body20, label %for.end31

for.body20:                                       ; preds = %for.cond18
  %arrayidx24 = getelementptr inbounds [100 x [100 x i32]], [100 x [100 x i32]]* %Tmp, i64 0, i64 %indvars.iv1, i64 %indvars.iv
  %tmp10 = load i32, i32* %arrayidx24, align 4
  %add = add nsw i32 %tmp10, %d
  %arrayidx28 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 %indvars.iv1, i64 %indvars.iv
  store i32 %add, i32* %arrayidx28, align 4
  br label %for.inc29

for.inc29:                                        ; preds = %for.body20
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond18

for.end31:                                        ; preds = %for.cond18
  br label %for.inc32

for.inc32:                                        ; preds = %for.end31
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond14

for.end34:                                        ; preds = %for.cond14
  ret void
}
