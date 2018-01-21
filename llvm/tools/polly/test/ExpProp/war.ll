; RUN: opt %loadPolly  -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
;
;    void f(int *A, int *B, int *C) {
;      for (int i = 0; i < 100; i++)
;  S0:   A[i] = C[i];
;      for (int i = 0; i < 100; i++)
;  S1:   B[i] += C[i];
;      for (int i = 0; i < 100; i++)
;  S2:   C[i] = 0;
;      for (int i = 0; i < 100; i++)
;  S3:   B[i] += A[i];
;    }
;
; CHECK:         Statements {
; CHECK-NEXT:    	Stmt_S0
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_S0[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_S0[i0] -> [0, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S0[i0] -> MemRef_C[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S0[i0] -> MemRef_A[i0] };
; CHECK-NEXT:    	Stmt_S1
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_S1[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_S1[i0] -> [1, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S1[i0] -> MemRef_C[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S1[i0] -> MemRef_B[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S1[i0] -> MemRef_B[i0] };
; CHECK-NEXT:    	Stmt_S2
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_S2[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_S2[i0] -> [2, i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S2[i0] -> MemRef_C[i0] };
; CHECK-NEXT:    	Stmt_S3
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_S3[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_S3[i0] -> [3, i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S3[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S3[i0] -> MemRef_B[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_S3[i0] -> MemRef_B[i0] };
; CHECK-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/war.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B, i32* %C) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc ], [ 0, %entry ]
  %exitcond9 = icmp ne i64 %indvars.iv7, 100
  br i1 %exitcond9, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S0

S0:                                               ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32* %C, i64 %indvars.iv7
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv7
  store i32 %tmp, i32* %arrayidx2, align 4
  br label %for.inc

for.inc:                                          ; preds = %S0
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc11, %for.end
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc11 ], [ 0, %for.end ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 100
  br i1 %exitcond6, label %for.body6, label %for.end13

for.body6:                                        ; preds = %for.cond4
  br label %S1

S1:                                               ; preds = %for.body6
  %arrayidx8 = getelementptr inbounds i32, i32* %C, i64 %indvars.iv4
  %tmp10 = load i32, i32* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv4
  %tmp11 = load i32, i32* %arrayidx10, align 4
  %add = add nsw i32 %tmp11, %tmp10
  store i32 %add, i32* %arrayidx10, align 4
  br label %for.inc11

for.inc11:                                        ; preds = %S1
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond4

for.end13:                                        ; preds = %for.cond4
  br label %for.cond15

for.cond15:                                       ; preds = %for.inc20, %for.end13
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc20 ], [ 0, %for.end13 ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body17, label %for.end22

for.body17:                                       ; preds = %for.cond15
  br label %S2

S2:                                               ; preds = %for.body17
  %arrayidx19 = getelementptr inbounds i32, i32* %C, i64 %indvars.iv1
  store i32 0, i32* %arrayidx19, align 4
  br label %for.inc20

for.inc20:                                        ; preds = %S2
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond15

for.end22:                                        ; preds = %for.cond15
  br label %for.cond24

for.cond24:                                       ; preds = %for.inc32, %for.end22
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc32 ], [ 0, %for.end22 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body26, label %for.end34

for.body26:                                       ; preds = %for.cond24
  br label %S3

S3:                                               ; preds = %for.body26
  %arrayidx28 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp12 = load i32, i32* %arrayidx28, align 4
  %arrayidx30 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  %tmp13 = load i32, i32* %arrayidx30, align 4
  %add31 = add nsw i32 %tmp13, %tmp12
  store i32 %add31, i32* %arrayidx30, align 4
  br label %for.inc32

for.inc32:                                        ; preds = %S3
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond24

for.end34:                                        ; preds = %for.cond24
  ret void
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
