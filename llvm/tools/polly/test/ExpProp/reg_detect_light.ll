; RUN: opt -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;
;    #define NIter 6
;    #define MAX_GRID 6
;    #define LENGTH 6
;    void f(long diff[MAX_GRID][MAX_GRID][LENGTH],
;           long sum_diff[MAX_GRID][MAX_GRID][LENGTH],
;           long sum_tang[MAX_GRID][MAX_GRID], long mean[MAX_GRID][MAX_GRID]) {
;
;      for (long t = 0; t < NIter; t++) {
;        for (long j = 0; j <= MAX_GRID - 1; j++)
;          for (long i = j; i <= MAX_GRID - 1; i++)
;            for (long cnt = 0; cnt <= LENGTH - 1; cnt++)
;              diff[j][i][cnt] = sum_tang[j][i];
;
;        for (long j = 0; j <= MAX_GRID - 1; j++) {
;          for (long i = j; i <= MAX_GRID - 1; i++) {
;            sum_diff[j][i][0] = diff[j][i][0];
;            for (long cnt = 1; cnt <= LENGTH - 1; cnt++)
;              sum_diff[j][i][cnt] = sum_diff[j][i][cnt - 1] + diff[j][i][cnt];
;            mean[j][i] = sum_diff[j][i][LENGTH - 1];
;          }
;        }
;      }
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_for_body9
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body9[5, i1, i2, i3] : i1 >= 0 and 0 <= i2 <= 5 - i1 and 0 <= i3 <= 5 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body9[i0, i1, i2, i3] -> [5, 0, i1, i2, i3, 0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body9[i0, i1, i2, i3] -> MemRef_sum_tang[i1, i1 + i2] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body9[i0, i1, i2, i3] -> MemRef_diff[i1, i1 + i2, i3] };
; CHECK-NEXT:    	Stmt_for_body27	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body27[5, i1, i2] : i1 >= 0 and 0 <= i2 <= 5 - i1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body27[i0, i1, i2] -> [5, 1, i1, i2, 0, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body27[i0, i1, i2] -> MemRef_sum_diff[i1, i1 + i2, 0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body27[i0, i1, i2] -> MemRef_sum_tang[i1, i1 + i2] };
; CHECK-NEXT:    	Stmt_for_body37	 [#Recompute Values: 2]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body37[i0, i1, i2, 0] : 0 <= i0 <= 5 and i1 >= 0 and 0 <= i2 <= 5 - i1 }; CopyDom: { Stmt_for_body37[i0, i1, i2, i3] : 0 <= i0 <= 5 and i1 >= 0 and 0 <= i2 <= 5 - i1 and 0 <= i3 <= 1; Stmt_for_body37[5, i1, i2, i3] : i1 >= 0 and 0 <= i2 <= 5 - i1 and 2 <= i3 <= 4 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body37[i0, i1, i2, i3] -> [i0, 1, i1, i2, 1, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body37[i0, i1, i2, i3] -> MemRef_sum_diff[i1, i1 + i2, 1 + i3] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_body37[i0, i1, i2, i3] -> MemRef_sum_tang[i1, i1 + i2] };
; CHECK-NEXT:    	Stmt_for_end49
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_end49[5, i1, i2] : i1 >= 0 and 0 <= i2 <= 5 - i1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_end49[i0, i1, i2] -> [5, 1, i1, i2, 2, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_end49[i0, i1, i2] -> MemRef_mean[i1, i1 + i2] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_end49[i0, i1, i2] -> MemRef_sum_tang[i1, i1 + i2] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_end49[i0, i1, i2] -> MemRef_sum_diff[i1, i1 + i2, 4] };
; CHECK-NEXT:    	Stmt_for_body37_0	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body37_0[5, i1, i2, i3] : i1 >= 0 and 0 <= i2 <= 5 - i1 and 2 <= i3 <= 4; Stmt_for_body37_0[i0, i1, i2, 1] : 0 <= i0 <= 5 and i1 >= 0 and 0 <= i2 <= 5 - i1 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body37_0[i0, i1, i2, 1] -> [i0, 1, i1, i2, 1, 1]; Stmt_for_body37_0[5, i1, i2, i3] -> [5, 1, i1, i2, 1, i3] : i3 >= 2 };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body37_0[i0, i1, i2, i3] -> MemRef_sum_diff[i1, i1 + i2, i3] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body37_0[i0, i1, i2, i3] -> MemRef_sum_diff[i1, i1 + i2, 1 + i3] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body37_0[i0, i1, i2, i3] -> MemRef_sum_tang[i1, i1 + i2] };
; CHECK-NEXT:    }
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f([6 x [6 x i64]]* %diff, [6 x [6 x i64]]* %sum_diff, [6 x i64]* %sum_tang, [6 x i64]* %mean) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc61, %entry
  %t.0 = phi i64 [ 0, %entry ], [ %inc62, %for.inc61 ]
  %exitcond6 = icmp ne i64 %t.0, 6
  br i1 %exitcond6, label %for.body, label %for.end63

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc17, %for.body
  %j.0 = phi i64 [ 0, %for.body ], [ %inc18, %for.inc17 ]
  %exitcond2 = icmp ne i64 %j.0, 6
  br i1 %exitcond2, label %for.body3, label %for.end19

for.body3:                                        ; preds = %for.cond1
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc14, %for.body3
  %i.0 = phi i64 [ %j.0, %for.body3 ], [ %inc15, %for.inc14 ]
  %exitcond1 = icmp ne i64 %i.0, 6
  br i1 %exitcond1, label %for.body6, label %for.end16

for.body6:                                        ; preds = %for.cond4
  br label %for.cond7

for.cond7:                                        ; preds = %for.inc, %for.body6
  %cnt.0 = phi i64 [ 0, %for.body6 ], [ %inc, %for.inc ]
  %exitcond = icmp ne i64 %cnt.0, 6
  br i1 %exitcond, label %for.body9, label %for.end

for.body9:                                        ; preds = %for.cond7
  %arrayidx10 = getelementptr inbounds [6 x i64], [6 x i64]* %sum_tang, i64 %j.0, i64 %i.0
  %tmp = load i64, i64* %arrayidx10, align 8
  %arrayidx13 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %diff, i64 %j.0, i64 %i.0, i64 %cnt.0
  store i64 %tmp, i64* %arrayidx13, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body9
  %inc = add nuw nsw i64 %cnt.0, 1
  br label %for.cond7

for.end:                                          ; preds = %for.cond7
  br label %for.inc14

for.inc14:                                        ; preds = %for.end
  %inc15 = add nuw nsw i64 %i.0, 1
  br label %for.cond4

for.end16:                                        ; preds = %for.cond4
  br label %for.inc17

for.inc17:                                        ; preds = %for.end16
  %inc18 = add nuw nsw i64 %j.0, 1
  br label %for.cond1

for.end19:                                        ; preds = %for.cond1
  br label %for.cond21

for.cond21:                                       ; preds = %for.inc58, %for.end19
  %j20.0 = phi i64 [ 0, %for.end19 ], [ %inc59, %for.inc58 ]
  %exitcond5 = icmp ne i64 %j20.0, 6
  br i1 %exitcond5, label %for.body23, label %for.end60

for.body23:                                       ; preds = %for.cond21
  br label %for.cond25

for.cond25:                                       ; preds = %for.inc55, %for.body23
  %i24.0 = phi i64 [ %j20.0, %for.body23 ], [ %inc56, %for.inc55 ]
  %exitcond4 = icmp ne i64 %i24.0, 6
  br i1 %exitcond4, label %for.body27, label %for.end57

for.body27:                                       ; preds = %for.cond25
  %arrayidx30 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %diff, i64 %j20.0, i64 %i24.0, i64 0
  %tmp7 = load i64, i64* %arrayidx30, align 8
  %arrayidx33 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %sum_diff, i64 %j20.0, i64 %i24.0, i64 0
  store i64 %tmp7, i64* %arrayidx33, align 8
  br label %for.cond35

for.cond35:                                       ; preds = %for.inc47, %for.body27
  %cnt34.0 = phi i64 [ 1, %for.body27 ], [ %inc48, %for.inc47 ]
  %exitcond3 = icmp ne i64 %cnt34.0, 6
  br i1 %exitcond3, label %for.body37, label %for.end49

for.body37:                                       ; preds = %for.cond35
  %sub = add nsw i64 %cnt34.0, -1
  %arrayidx40 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %sum_diff, i64 %j20.0, i64 %i24.0, i64 %sub
  %tmp8 = load i64, i64* %arrayidx40, align 8
  %arrayidx43 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %diff, i64 %j20.0, i64 %i24.0, i64 %cnt34.0
  %tmp9 = load i64, i64* %arrayidx43, align 8
  %add = add nsw i64 %tmp8, %tmp9
  %arrayidx46 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %sum_diff, i64 %j20.0, i64 %i24.0, i64 %cnt34.0
  store i64 %add, i64* %arrayidx46, align 8
  br label %for.inc47

for.inc47:                                        ; preds = %for.body37
  %inc48 = add nuw nsw i64 %cnt34.0, 1
  br label %for.cond35

for.end49:                                        ; preds = %for.cond35
  %arrayidx52 = getelementptr inbounds [6 x [6 x i64]], [6 x [6 x i64]]* %sum_diff, i64 %j20.0, i64 %i24.0, i64 5
  %tmp10 = load i64, i64* %arrayidx52, align 8
  %arrayidx54 = getelementptr inbounds [6 x i64], [6 x i64]* %mean, i64 %j20.0, i64 %i24.0
  store i64 %tmp10, i64* %arrayidx54, align 8
  br label %for.inc55

for.inc55:                                        ; preds = %for.end49
  %inc56 = add nuw nsw i64 %i24.0, 1
  br label %for.cond25

for.end57:                                        ; preds = %for.cond25
  br label %for.inc58

for.inc58:                                        ; preds = %for.end57
  %inc59 = add nuw nsw i64 %j20.0, 1
  br label %for.cond21

for.end60:                                        ; preds = %for.cond21
  br label %for.inc61

for.inc61:                                        ; preds = %for.end60
  %inc62 = add nuw nsw i64 %t.0, 1
  br label %for.cond

for.end63:                                        ; preds = %for.cond
  ret void
}
