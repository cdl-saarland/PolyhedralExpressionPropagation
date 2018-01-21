; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-max-prop=0 -polly-min-prop=0 -polly-only-func=f -polly-codegen < %s | lli | FileCheck %s
;
; CHECK: [eval f(1)], s = 197348250
; CHECK: [eval f(2)], s = -1010018344
; CHECK: [eval f(3)], s = 658267918
; CHECK: [eval f(4)], s = 472873278
; CHECK: [eval f(5)], s = -589070062
; CHECK: [eval f(47)], s = 32711862
; CHECK: [eval f(100)], s = -1482393541
;
;    #include <stdio.h>
;
;    int A[100], B[100], C[100];
;
;    void f(int N) {
;      for (int i = 1; i < N - 3; i++) {
;        B[i - 1] = A[i + 1] + 2;
;        C[i + 1] = B[i - 1] * 3;
;        A[i + 3] = A[i] + B[i] * 2 + C[i] * 3;
;      }
;      for (int i = 1; i < 100; i++) {
;        B[i] = B[i] * 3 + C[i] * 2 + A[i - 1] + B[i - 1] + C[i - 1];
;      }
;    }
;
;    void eval(int N) {
;      f(N);
;      int s = 0;
;      for (int i = 0; i < 100; i++)
;        s += A[i] * B[i] + C[i];
;      printf("[eval f(%i)], s = %i\n", N, s);
;    }
;
;    int main() {
;      for (int i = 0; i < 100; i++) {
;        A[i] = 1 * i;
;        B[i] = 2 * i;
;        C[i] = 3 * i;
;      }
;
;      eval(1);
;      eval(2);
;      eval(3);
;      eval(4);
;      eval(5);
;      eval(47);
;      eval(100);
;      return 0;
;    }
;
source_filename = "../polly/test/ExpProp/main_random_5.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@A = common global [100 x i32] zeroinitializer, align 16
@B = common global [100 x i32] zeroinitializer, align 16
@C = common global [100 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [22 x i8] c"[eval f(%i)], s = %i\0A\00", align 1

define void @f(i32 %N) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc ], [ 1, %entry ]
  %sub = add nsw i32 %N, -3
  %tmp = sext i32 %sub to i64
  %cmp = icmp slt i64 %indvars.iv4, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv.next5
  %tmp9 = load i32, i32* %arrayidx, align 4
  %add1 = add nsw i32 %tmp9, 2
  %tmp10 = add nsw i64 %indvars.iv4, -1
  %arrayidx4 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp10
  store i32 %add1, i32* %arrayidx4, align 4
  %tmp11 = add nsw i64 %indvars.iv4, -1
  %arrayidx7 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp11
  %tmp12 = load i32, i32* %arrayidx7, align 4
  %mul = mul nsw i32 %tmp12, 3
  %arrayidx10 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv.next5
  store i32 %mul, i32* %arrayidx10, align 4
  %arrayidx12 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv4
  %tmp13 = load i32, i32* %arrayidx12, align 4
  %arrayidx14 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv4
  %tmp14 = load i32, i32* %arrayidx14, align 4
  %mul15 = shl nsw i32 %tmp14, 1
  %add16 = add nsw i32 %tmp13, %mul15
  %arrayidx18 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv4
  %tmp15 = load i32, i32* %arrayidx18, align 4
  %mul19 = mul nsw i32 %tmp15, 3
  %add20 = add nsw i32 %add16, %mul19
  %tmp16 = add nuw nsw i64 %indvars.iv4, 3
  %arrayidx23 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %tmp16
  store i32 %add20, i32* %arrayidx23, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond25

for.cond25:                                       ; preds = %for.inc49, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc49 ], [ 1, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body27, label %for.end51

for.body27:                                       ; preds = %for.cond25
  %arrayidx29 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp17 = load i32, i32* %arrayidx29, align 4
  %mul30 = mul nsw i32 %tmp17, 3
  %arrayidx32 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp18 = load i32, i32* %arrayidx32, align 4
  %mul33 = shl nsw i32 %tmp18, 1
  %add34 = add nsw i32 %mul30, %mul33
  %tmp19 = add nsw i64 %indvars.iv, -1
  %arrayidx37 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %tmp19
  %tmp20 = load i32, i32* %arrayidx37, align 4
  %add38 = add nsw i32 %add34, %tmp20
  %tmp21 = add nsw i64 %indvars.iv, -1
  %arrayidx41 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp21
  %tmp22 = load i32, i32* %arrayidx41, align 4
  %add42 = add nsw i32 %add38, %tmp22
  %tmp23 = add nsw i64 %indvars.iv, -1
  %arrayidx45 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %tmp23
  %tmp24 = load i32, i32* %arrayidx45, align 4
  %add46 = add nsw i32 %add42, %tmp24
  %arrayidx48 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  store i32 %add46, i32* %arrayidx48, align 4
  br label %for.inc49

for.inc49:                                        ; preds = %for.body27
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond25

for.end51:                                        ; preds = %for.cond25
  ret void
}

define void @eval(i32 %N) #0 {
entry:
  call void @f(i32 %N)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %s.0 = phi i32 [ 0, %entry ], [ %add5, %for.inc ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx2, align 4
  %mul = mul nsw i32 %tmp, %tmp1
  %arrayidx4 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx4, align 4
  %add = add nsw i32 %mul, %tmp2
  %add5 = add nsw i32 %s.0, %add
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i32 [ %s.0, %for.cond ]
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @.str, i64 0, i64 0), i32 %N, i32 %s.0.lcssa) #2
  ret void
}

declare i32 @printf(i8*, ...) #1

define i32 @main() #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  %tmp = trunc i64 %indvars.iv to i32
  store i32 %tmp, i32* %arrayidx, align 4
  %tmp3 = shl nsw i64 %indvars.iv, 1
  %arrayidx3 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp4 = trunc i64 %tmp3 to i32
  store i32 %tmp4, i32* %arrayidx3, align 4
  %tmp5 = mul nuw nsw i64 %indvars.iv, 3
  %arrayidx6 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp6 = trunc i64 %tmp5 to i32
  store i32 %tmp6, i32* %arrayidx6, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  call void @eval(i32 1)
  call void @eval(i32 2)
  call void @eval(i32 3)
  call void @eval(i32 4)
  call void @eval(i32 5)
  call void @eval(i32 47)
  call void @eval(i32 100)
  ret i32 0
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0 (http://llvm.org/git/clang.git f304e39bb9d423169133175d08ec491ff7003e01)"}
