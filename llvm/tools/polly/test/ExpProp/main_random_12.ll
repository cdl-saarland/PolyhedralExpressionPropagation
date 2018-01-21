; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen < %s | lli | FileCheck %s
;
; CHECK: [eval f(1)], s = 671550
; CHECK: [eval f(2)], s = 671577
; CHECK: [eval f(3)], s = 671707
; CHECK: [eval f(4)], s = 672064
; CHECK: [eval f(5)], s = 672332
; CHECK: [eval f(47)], s = 1155141
; CHECK: [eval f(100)], s = 5333396
;
;    #include <stdio.h>
;
;    int A[100], B[100], C[100];
;
;    void f(int N) {
;      for (int i = 0; i < N; i++) {
;        B[i] = i + 1;
;        C[i] = i - 1;
;        A[i] = B[i] + C[i];
;      }
;      for (int i = 0; i < N; i++) {
;        B[i] += i + 1;
;        C[i] -= i - 1;
;        A[i] *= B[i] + C[i];
;      }
;      for (int i = 3; i < N - 1; i++) {
;        B[i] -= B[i + 1];
;        C[i] -= C[i - 1];
;        A[i] += B[i - 2] + C[i - 3];
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
source_filename = "../polly/test/ExpProp/main_random_10.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@B = common global [100 x i32] zeroinitializer, align 16
@C = common global [100 x i32] zeroinitializer, align 16
@A = common global [100 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [22 x i8] c"[eval f(%i)], s = %i\0A\00", align 1

define void @f(i32 %N) #0 {
entry:
  %tmp = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc ], [ 0, %entry ]
  %cmp = icmp slt i64 %indvars.iv7, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv7
  %tmp10 = trunc i64 %indvars.iv.next8 to i32
  store i32 %tmp10, i32* %arrayidx, align 4
  %tmp11 = add nsw i64 %indvars.iv7, -1
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv7
  %tmp12 = trunc i64 %tmp11 to i32
  store i32 %tmp12, i32* %arrayidx2, align 4
  %arrayidx4 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv7
  %tmp13 = load i32, i32* %arrayidx4, align 4
  %arrayidx6 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv7
  %tmp14 = load i32, i32* %arrayidx6, align 4
  %add7 = add nsw i32 %tmp13, %tmp14
  %arrayidx9 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv7
  store i32 %add7, i32* %arrayidx9, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp15 = sext i32 %N to i64
  br label %for.cond11

for.cond11:                                       ; preds = %for.inc29, %for.end
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc29 ], [ 0, %for.end ]
  %cmp12 = icmp slt i64 %indvars.iv4, %tmp15
  br i1 %cmp12, label %for.body13, label %for.end31

for.body13:                                       ; preds = %for.cond11
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %arrayidx16 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv4
  %tmp16 = load i32, i32* %arrayidx16, align 4
  %tmp17 = trunc i64 %indvars.iv.next5 to i32
  %add17 = add nsw i32 %tmp16, %tmp17
  store i32 %add17, i32* %arrayidx16, align 4
  %tmp18 = add nsw i64 %indvars.iv4, -1
  %arrayidx20 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv4
  %tmp19 = load i32, i32* %arrayidx20, align 4
  %tmp20 = trunc i64 %tmp18 to i32
  %sub21 = sub nsw i32 %tmp19, %tmp20
  store i32 %sub21, i32* %arrayidx20, align 4
  %arrayidx23 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv4
  %tmp21 = load i32, i32* %arrayidx23, align 4
  %arrayidx25 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv4
  %tmp22 = load i32, i32* %arrayidx25, align 4
  %add26 = add nsw i32 %tmp21, %tmp22
  %arrayidx28 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv4
  %tmp23 = load i32, i32* %arrayidx28, align 4
  %mul = mul nsw i32 %tmp23, %add26
  store i32 %mul, i32* %arrayidx28, align 4
  br label %for.inc29

for.inc29:                                        ; preds = %for.body13
  br label %for.cond11

for.end31:                                        ; preds = %for.cond11
  br label %for.cond33

for.cond33:                                       ; preds = %for.inc59, %for.end31
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc59 ], [ 3, %for.end31 ]
  %sub34 = add nsw i32 %N, -1
  %tmp24 = sext i32 %sub34 to i64
  %cmp35 = icmp slt i64 %indvars.iv, %tmp24
  br i1 %cmp35, label %for.body36, label %for.end61

for.body36:                                       ; preds = %for.cond33
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx39 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv.next
  %tmp25 = load i32, i32* %arrayidx39, align 4
  %arrayidx41 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp26 = load i32, i32* %arrayidx41, align 4
  %sub42 = sub nsw i32 %tmp26, %tmp25
  store i32 %sub42, i32* %arrayidx41, align 4
  %tmp27 = add nsw i64 %indvars.iv, -1
  %arrayidx45 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %tmp27
  %tmp28 = load i32, i32* %arrayidx45, align 4
  %arrayidx47 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp29 = load i32, i32* %arrayidx47, align 4
  %sub48 = sub nsw i32 %tmp29, %tmp28
  store i32 %sub48, i32* %arrayidx47, align 4
  %tmp30 = add nsw i64 %indvars.iv, -2
  %arrayidx51 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %tmp30
  %tmp31 = load i32, i32* %arrayidx51, align 4
  %tmp32 = add nsw i64 %indvars.iv, -3
  %arrayidx54 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %tmp32
  %tmp33 = load i32, i32* %arrayidx54, align 4
  %add55 = add nsw i32 %tmp31, %tmp33
  %arrayidx57 = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  %tmp34 = load i32, i32* %arrayidx57, align 4
  %add58 = add nsw i32 %tmp34, %add55
  store i32 %add58, i32* %arrayidx57, align 4
  br label %for.inc59

for.inc59:                                        ; preds = %for.body36
  br label %for.cond33

for.end61:                                        ; preds = %for.cond33
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
