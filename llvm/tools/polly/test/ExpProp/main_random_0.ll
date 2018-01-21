; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen < %s | lli | FileCheck %s
;
; CHECK: [eval f(1)], s = 814785
; CHECK: [eval f(2)], s = 825762
; CHECK: [eval f(3)], s = 836586
; CHECK: [eval f(4)], s = 847294
; CHECK: [eval f(5)], s = 857854
; CHECK: [eval f(47)], s = 960141
; CHECK: [eval f(100)], s = 1492492
;
;    #include <stdio.h>
;
;    int A[100], B[100], C[100];
;
;    void f(int N) {
;      for (int i = 1; i < N; i++) {
;        A[i] = i;
;        B[i - 1] = i + 1;
;        C[i + 1] = i - 1;
;        A[i] += B[i] + C[i];
;      }
;      for (int i = 3; i < 100; i++) {
;        B[i - 3] = B[i - 2];
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
;        A[i] = 1.1 * i;
;        B[i] = 2.2 * i;
;        C[i] = 3.3 * i;
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
source_filename = "../polly/test/ExpProp/main_random_0.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@A = common global [100 x i32] zeroinitializer, align 16
@B = common global [100 x i32] zeroinitializer, align 16
@C = common global [100 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [22 x i8] c"[eval f(%i)], s = %i\0A\00", align 1

define void @f(i32 %N) #0 {
entry:
  %tmp = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv3 = phi i64 [ %indvars.iv.next4, %for.inc ], [ 1, %entry ]
  %cmp = icmp slt i64 %indvars.iv3, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv3
  %tmp7 = trunc i64 %indvars.iv3 to i32
  store i32 %tmp7, i32* %arrayidx, align 4
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %tmp8 = add nsw i64 %indvars.iv3, -1
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp8
  %tmp9 = trunc i64 %indvars.iv.next4 to i32
  store i32 %tmp9, i32* %arrayidx2, align 4
  %tmp10 = add nsw i64 %indvars.iv3, -1
  %arrayidx6 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv.next4
  %tmp11 = trunc i64 %tmp10 to i32
  store i32 %tmp11, i32* %arrayidx6, align 4
  %arrayidx8 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv3
  %tmp12 = load i32, i32* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv3
  %tmp13 = load i32, i32* %arrayidx10, align 4
  %add11 = add nsw i32 %tmp12, %tmp13
  %arrayidx13 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv3
  %tmp14 = load i32, i32* %arrayidx13, align 4
  %add14 = add nsw i32 %tmp14, %add11
  store i32 %add14, i32* %arrayidx13, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond16

for.cond16:                                       ; preds = %for.inc25, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc25 ], [ 3, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body18, label %for.end27

for.body18:                                       ; preds = %for.cond16
  %tmp15 = add nsw i64 %indvars.iv, -2
  %arrayidx21 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp15
  %tmp16 = load i32, i32* %arrayidx21, align 4
  %tmp17 = add nsw i64 %indvars.iv, -3
  %arrayidx24 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp17
  store i32 %tmp16, i32* %arrayidx24, align 4
  br label %for.inc25

for.inc25:                                        ; preds = %for.body18
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond16

for.end27:                                        ; preds = %for.cond16
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
  %tmp = trunc i64 %indvars.iv to i32
  %conv = sitofp i32 %tmp to double
  %mul = fmul fast double %conv, 1.100000e+00
  %conv1 = fptosi double %mul to i32
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  store i32 %conv1, i32* %arrayidx, align 4
  %tmp1 = trunc i64 %indvars.iv to i32
  %conv2 = sitofp i32 %tmp1 to double
  %mul3 = fmul fast double %conv2, 2.200000e+00
  %conv4 = fptosi double %mul3 to i32
  %arrayidx6 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  store i32 %conv4, i32* %arrayidx6, align 4
  %tmp2 = trunc i64 %indvars.iv to i32
  %conv7 = sitofp i32 %tmp2 to double
  %mul8 = fmul fast double %conv7, 3.300000e+00
  %conv9 = fptosi double %mul8 to i32
  %arrayidx11 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  store i32 %conv9, i32* %arrayidx11, align 4
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
