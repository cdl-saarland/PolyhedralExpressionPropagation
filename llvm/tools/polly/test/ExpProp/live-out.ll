; RUN: opt %loadPolly -polly-exp -analyze < %s | FileCheck %s
;
;    #include <stdlib.h>
;
;    int foo(int *b, int *c) {
;      int *a = malloc(100 * sizeof(int));
;      int temp;
;      int i;
;      for (i = 0; i < 50; i++)
;        a[i] = b[i] * b[i]; // S1
;      for (i = 0; i < 100; i++) {
;        if (i < 50)
;          a[i + 50] = b[i]; // S2
;        c[i] = a[i];        // S3
;      }
;      temp = a[50]; // S4
;      free(a);
;      return temp;
;    }
;
source_filename = "live-out.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define i32 @foo(i32* %b, i32* %c) {
entry:
  %call = call noalias i8* @malloc(i64 400) #2
  %tmp = bitcast i8* %call to i32*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv2 = phi i64 [ %indvars.iv.next3, %for.inc ], [ 0, %entry ]
  %exitcond4 = icmp ne i64 %indvars.iv2, 50
  br i1 %exitcond4, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %indvars.iv2
  %tmp5 = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %b, i64 %indvars.iv2
  %tmp6 = load i32, i32* %arrayidx2, align 4
  %mul = mul nsw i32 %tmp5, %tmp6
  %arrayidx4 = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv2
  store i32 %mul, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next3 = add nuw nsw i64 %indvars.iv2, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %for.cond5

for.cond5:                                        ; preds = %for.inc17, %for.end
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc17 ], [ 0, %for.end ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body7, label %for.end19

for.body7:                                        ; preds = %for.cond5
  %cmp8 = icmp slt i64 %indvars.iv, 50
  br i1 %cmp8, label %if.then, label %if.end

if.then:                                          ; preds = %for.body7
  %arrayidx10 = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %tmp7 = load i32, i32* %arrayidx10, align 4
  %tmp8 = add nuw nsw i64 %indvars.iv, 50
  %arrayidx12 = getelementptr inbounds i32, i32* %tmp, i64 %tmp8
  store i32 %tmp7, i32* %arrayidx12, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body7
  %arrayidx14 = getelementptr inbounds i32, i32* %tmp, i64 %indvars.iv
  %tmp9 = load i32, i32* %arrayidx14, align 4
  %arrayidx16 = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
  store i32 %tmp9, i32* %arrayidx16, align 4
  br label %for.inc17

for.inc17:                                        ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond5

for.end19:                                        ; preds = %for.cond5
  %arrayidx20 = getelementptr inbounds i8, i8* %call, i64 200
  %tmp10 = bitcast i8* %arrayidx20 to i32*
  %tmp11 = load i32, i32* %tmp10, align 4
  br label %end
end:
  call void @free(i8* %call) #2
  ret i32 %tmp11
}

declare noalias i8* @malloc(i64) #1

declare void @free(i8*) #1

attributes #0= { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0 (http://llvm.org/git/clang.git e88f773dad2238b87c5b425eab780d02fdc62c9a) (git@public.cdl.uni-saarland.de:optimization/Polly.git c94df81328557b85ba7897c6d5aa52b6ff470c2d)"}
