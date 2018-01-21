; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f1(int *a, int *b) {
;      int temp[100];
;      int i;
;      for (i = 0; i < 50; i++) {
;        /*temp[i] = a[i] + a[i-1];             //S1*/
;        temp[i] = a[i]; // S1
;      }
;      for (i = 0; i < 100; i++) {
;        /*a[i] = 0;*/
;        a[i] = temp[i];
;      }
;      for (i = 0; i < 100; i++) {
;        a[i] = b[i];                           // body15
;      }
;      // return sum;
;    }
;
; CHECK:         	Stmt_for_body15
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body15[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body15[i0] -> [i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body15[i0] -> MemRef_b[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body15[i0] -> MemRef_a[i0] };
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f1(i32* %a, i32* %b) {
entry:
  %temp = alloca [100 x i32], align 16
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc ], [ 0, %entry ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 50
  br i1 %exitcond6, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %a, i64 %indvars.iv4
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* %temp, i64 0, i64 %indvars.iv4
  store i32 %tmp, i32* %arrayidx2, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond3

for.cond3:                                        ; preds = %for.inc10, %for.end
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc10 ], [ 0, %for.end ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body5, label %for.end12

for.body5:                                        ; preds = %for.cond3
  %arrayidx7 = getelementptr inbounds [100 x i32], [100 x i32]* %temp, i64 0, i64 %indvars.iv1
  %tmp7 = load i32, i32* %arrayidx7, align 4
  %arrayidx9 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv1
  store i32 %tmp7, i32* %arrayidx9, align 4
  br label %for.inc10

for.inc10:                                        ; preds = %for.body5
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond3

for.end12:                                        ; preds = %for.cond3
  br label %for.cond13

for.cond13:                                       ; preds = %for.inc20, %for.end12
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc20 ], [ 0, %for.end12 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body15, label %for.end22

for.body15:                                       ; preds = %for.cond13
  %arrayidx17 = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %tmp8 = load i32, i32* %arrayidx17, align 4
  %arrayidx19 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  store i32 %tmp8, i32* %arrayidx19, align 4
  br label %for.inc20

for.inc20:                                        ; preds = %for.body15
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond13

for.end22:                                        ; preds = %for.cond13
  ret void
}
