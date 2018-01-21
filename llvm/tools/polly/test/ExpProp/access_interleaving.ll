; RUN: opt %loadPolly -polly-exp -polly-codegen -S -polly-ignore-aliasing < %s | FileCheck %s
;
;    void f(int *IN, int *OUT) {
;      int A[100], B[100], C[100];
;      for (int i = 0; i < 100; i++)
;        A[i] = 2 * IN[i];
;
;      for (int i = 0; i < 100; i++)
;        B[i] = A[i];
;
;      for (int i = 0; i < 100; i++)
;        B[i] = B[i] + B[i];
;
;      for (int i = 0; i < 100; i++)
;        C[i] = B[i] + B[i] * B[i];
;
;      for (int i = 0; i < 100; i++)
;        OUT[i] = C[i];
;    }
;
; CHECK:       polly.stmt.for.body47:
; CHECK-NEXT:    %p_arrayidx = getelementptr inbounds i32, i32* %IN, i64 %polly.indvar
; CHECK-NEXT:    %p_tmp = load i32, i32* %p_arrayidx
; CHECK-NEXT:    %p_mul = shl nsw i32 %p_tmp, 1
; CHECK-NEXT:    %p_add = add nsw i32 %p_mul, %p_mul
; CHECK-NEXT:    %p_mul37 = mul nsw i32 %p_add, %p_add
; CHECK-NEXT:    %p_add38 = add nsw i32 %p_add, %p_mul37
; CHECK-NEXT:    %scevgep = getelementptr i32, i32* %OUT, i64 %polly.indvar
; CHECK-NEXT:    store i32 %p_add38
;
source_filename = "t.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %IN, i32* %OUT) #0 {
entry:
  %A = alloca [100 x i32], align 16
  %B = alloca [100 x i32], align 16
  %C = alloca [100 x i32], align 16
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv10 = phi i64 [ %indvars.iv.next11, %for.inc ], [ 0, %entry ]
  %exitcond12 = icmp ne i64 %indvars.iv10, 100
  br i1 %exitcond12, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %IN, i64 %indvars.iv10
  %tmp = load i32, i32* %arrayidx, align 4
  %mul = shl nsw i32 %tmp, 1
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 0, i64 %indvars.iv10
  store i32 %mul, i32* %arrayidx2, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next11 = add nuw nsw i64 %indvars.iv10, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc11, %for.end
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc11 ], [ 0, %for.end ]
  %exitcond9 = icmp ne i64 %indvars.iv7, 100
  br i1 %exitcond9, label %for.body6, label %for.end13

for.body6:                                        ; preds = %for.cond4
  %arrayidx8 = getelementptr inbounds [100 x i32], [100 x i32]* %A, i64 0, i64 %indvars.iv7
  %tmp13 = load i32, i32* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv7
  store i32 %tmp13, i32* %arrayidx10, align 4
  br label %for.inc11

for.inc11:                                        ; preds = %for.body6
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond4

for.end13:                                        ; preds = %for.cond4
  br label %for.cond15

for.cond15:                                       ; preds = %for.inc24, %for.end13
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc24 ], [ 0, %for.end13 ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 100
  br i1 %exitcond6, label %for.body17, label %for.end26

for.body17:                                       ; preds = %for.cond15
  %arrayidx19 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv4
  %tmp14 = load i32, i32* %arrayidx19, align 4
  %arrayidx21 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv4
  %tmp15 = load i32, i32* %arrayidx21, align 4
  %add = add nsw i32 %tmp14, %tmp15
  %arrayidx23 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv4
  store i32 %add, i32* %arrayidx23, align 4
  br label %for.inc24

for.inc24:                                        ; preds = %for.body17
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond15

for.end26:                                        ; preds = %for.cond15
  br label %for.cond28

for.cond28:                                       ; preds = %for.inc41, %for.end26
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc41 ], [ 0, %for.end26 ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body30, label %for.end43

for.body30:                                       ; preds = %for.cond28
  %arrayidx32 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv1
  %tmp16 = load i32, i32* %arrayidx32, align 4
  %arrayidx34 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv1
  %tmp17 = load i32, i32* %arrayidx34, align 4
  %arrayidx36 = getelementptr inbounds [100 x i32], [100 x i32]* %B, i64 0, i64 %indvars.iv1
  %tmp18 = load i32, i32* %arrayidx36, align 4
  %mul37 = mul nsw i32 %tmp17, %tmp18
  %add38 = add nsw i32 %tmp16, %mul37
  %arrayidx40 = getelementptr inbounds [100 x i32], [100 x i32]* %C, i64 0, i64 %indvars.iv1
  store i32 %add38, i32* %arrayidx40, align 4
  br label %for.inc41

for.inc41:                                        ; preds = %for.body30
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond28

for.end43:                                        ; preds = %for.cond28
  br label %for.cond45

for.cond45:                                       ; preds = %for.inc52, %for.end43
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc52 ], [ 0, %for.end43 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body47, label %for.end54

for.body47:                                       ; preds = %for.cond45
  %arrayidx49 = getelementptr inbounds [100 x i32], [100 x i32]* %C, i64 0, i64 %indvars.iv
  %tmp19 = load i32, i32* %arrayidx49, align 4
  %arrayidx51 = getelementptr inbounds i32, i32* %OUT, i64 %indvars.iv
  store i32 %tmp19, i32* %arrayidx51, align 4
  br label %for.inc52

for.inc52:                                        ; preds = %for.body47
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond45

for.end54:                                        ; preds = %for.cond45
  ret void
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0 (http://llvm.org/git/clang.git f304e39bb9d423169133175d08ec491ff7003e01)"}
