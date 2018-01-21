; RUN: opt %loadPolly -polly-only-func=eval -polly-exp -polly-exp-heuristic=0 -polly-codegen < %s | lli | FileCheck %s
;
; CHECK: [eval f(1)], s = 4950,9900,14850,19800
; CHECK: [eval f(2)], s = 4950,9900,14850,19800
; CHECK: [eval f(3)], s = 4950,9900,14851,19800
; CHECK: [eval f(4)], s = 4950,9899,14850,19801
; CHECK: [eval f(5)], s = 4950,9897,14847,19800
; CHECK: [eval f(47)], s = 4950,8910,12915,17952
; CHECK: [eval f(100)], s = 4950,5147,5442,10585
;
;    #include <stdio.h>
;
;    int A[100], B[100], C[100], D[100];
;
;    void f(int N) {
;      for (int i = 1; i < N - 1; i++) {
;        B[i] = A[i] + 1;
;        C[i] = B[i] + 2;
;        D[i] = C[i] + C[i - 1];
;      }
;    }
;
;    void eval(int N) {
;      f(N);
;      int a = 0, b = 0, c = 0, d = 0;
;      for (int i = 0; i < 100; i++) {
;        a += A[i];
;        b += B[i];
;        c += C[i];
;        d += D[i];
;      }
;      printf("[eval f(%i)], s = %i,%i,%i,%i\n", N, a, b, c, d);
;    }
;
;    int main() {
;      for (int i = 0; i < 100; i++) {
;        A[i] = 1 * i;
;        B[i] = 2 * i;
;        C[i] = 3 * i;
;        D[i] = 4 * i;
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
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/main_random_7b.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@A = common global [100 x i32] zeroinitializer, align 16
@B = common global [100 x i32] zeroinitializer, align 16
@C = common global [100 x i32] zeroinitializer, align 16
@D = common global [100 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [31 x i8] c"[eval f(%i)], s = %i,%i,%i,%i\0A\00", align 1

define void @f(i32 %N) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %sub = add nsw i32 %N, -1
  %tmp = sext i32 %sub to i64
  %cmp = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %tmp2, 1
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  store i32 %add, i32* %arrayidx2, align 4
  %arrayidx4 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx4, align 4
  %add5 = add nsw i32 %tmp3, 2
  %arrayidx7 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  store i32 %add5, i32* %arrayidx7, align 4
  %arrayidx9 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp4 = load i32, i32* %arrayidx9, align 4
  %tmp5 = add nsw i64 %indvars.iv, -1
  %arrayidx12 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %tmp5
  %tmp6 = load i32, i32* %arrayidx12, align 4
  %add13 = add nsw i32 %tmp4, %tmp6
  %arrayidx15 = getelementptr inbounds [100 x i32], [100 x i32]* @D, i64 0, i64 %indvars.iv
  store i32 %add13, i32* %arrayidx15, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

define void @eval(i32 %N) #0 {
entry:
  call void @f(i32 %N)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %d.0 = phi i32 [ 0, %entry ], [ %add9, %for.inc ]
  %c.0 = phi i32 [ 0, %entry ], [ %add6, %for.inc ]
  %b.0 = phi i32 [ 0, %entry ], [ %add3, %for.inc ]
  %a.0 = phi i32 [ 0, %entry ], [ %add, %for.inc ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %arrayidx8 = getelementptr inbounds [100 x i32], [100 x i32]* @D, i64 0, i64 %indvars.iv
  %tmp = load i32, i32* %arrayidx8, align 4
  %add9 = add nsw i32 %d.0, %tmp
  %arrayidx5 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx5, align 4
  %add6 = add nsw i32 %c.0, %tmp1
  %arrayidx2 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx2, align 4
  %add3 = add nsw i32 %b.0, %tmp2
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* @A, i64 0, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %a.0, %tmp3
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %d.0.lcssa = phi i32 [ %d.0, %for.cond ]
  %c.0.lcssa = phi i32 [ %c.0, %for.cond ]
  %b.0.lcssa = phi i32 [ %b.0, %for.cond ]
  %a.0.lcssa = phi i32 [ %a.0, %for.cond ]
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([31 x i8], [31 x i8]* @.str, i64 0, i64 0), i32 %N, i32 %a.0.lcssa, i32 %b.0.lcssa, i32 %c.0.lcssa, i32 %d.0.lcssa) #2
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
  %tmp4 = shl nsw i64 %indvars.iv, 1
  %arrayidx3 = getelementptr inbounds [100 x i32], [100 x i32]* @B, i64 0, i64 %indvars.iv
  %tmp5 = trunc i64 %tmp4 to i32
  store i32 %tmp5, i32* %arrayidx3, align 4
  %tmp6 = mul nuw nsw i64 %indvars.iv, 3
  %arrayidx6 = getelementptr inbounds [100 x i32], [100 x i32]* @C, i64 0, i64 %indvars.iv
  %tmp7 = trunc i64 %tmp6 to i32
  store i32 %tmp7, i32* %arrayidx6, align 4
  %tmp8 = shl nsw i64 %indvars.iv, 2
  %arrayidx9 = getelementptr inbounds [100 x i32], [100 x i32]* @D, i64 0, i64 %indvars.iv
  %tmp9 = trunc i64 %tmp8 to i32
  store i32 %tmp9, i32* %arrayidx9, align 4
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
