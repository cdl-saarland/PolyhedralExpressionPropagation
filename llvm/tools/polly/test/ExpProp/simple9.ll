; RUN: opt %loadPolly -analyze < %s | FileCheck %s
;
; FIXME: Edit the run line and add checks!
;
; XFAIL: *
;
;    #include <stdlib.h>
;
;    void f(int *A) {
;      int *tmp = malloc(sizeof(int) * 100);
;
;      for (int i = 0; i < 100; i++)
;      S0:
;        A[i] = i;
;
;      for (int j = 0; j < 10; j++) {
;
;        for (int i = 0; i < 100; i++)
;        S1:
;          tmp[i] = A[i];
;
;        for (int i = 0; i < 100; i++)
;        S2:
;          tmp[i] += A[i] + i;
;
;        for (int i = 0; i < 100; i++)
;        S3:
;          tmp[i] += i;
;
;        for (int i = 0; i < 100; i++)
;        S4:
;          A[i] += tmp[i];
;      }
;
;      free(tmp);
;    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/simple9.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A) #0 {
entry:
  %call = call noalias i8* @malloc(i64 400) #2
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv11 = phi i64 [ %indvars.iv.next12, %for.inc ], [ 0, %entry ]
  %exitcond13 = icmp ne i64 %indvars.iv11, 100
  br i1 %exitcond13, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S0

S0:                                               ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv11
  %tmp = trunc i64 %indvars.iv11 to i32
  store i32 %tmp, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %S0
  %indvars.iv.next12 = add nuw nsw i64 %indvars.iv11, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp14 = bitcast i8* %call to i32*
  br label %for.cond3

for.cond3:                                        ; preds = %for.inc55, %for.end
  %j.0 = phi i32 [ 0, %for.end ], [ %inc56, %for.inc55 ]
  %exitcond10 = icmp ne i32 %j.0, 10
  br i1 %exitcond10, label %for.body5, label %for.end57

for.body5:                                        ; preds = %for.cond3
  br label %for.cond8

for.cond8:                                        ; preds = %for.inc15, %for.body5
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc15 ], [ 0, %for.body5 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body10, label %for.end17

for.body10:                                       ; preds = %for.cond8
  br label %S1

S1:                                               ; preds = %for.body10
  %arrayidx12 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp15 = load i32, i32* %arrayidx12, align 4
  %arrayidx14 = getelementptr inbounds i32, i32* %tmp14, i64 %indvars.iv
  store i32 %tmp15, i32* %arrayidx14, align 4
  br label %for.inc15

for.inc15:                                        ; preds = %S1
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond8

for.end17:                                        ; preds = %for.cond8
  br label %for.cond20

for.cond20:                                       ; preds = %for.inc28, %for.end17
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc28 ], [ 0, %for.end17 ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body22, label %for.end30

for.body22:                                       ; preds = %for.cond20
  br label %S2

S2:                                               ; preds = %for.body22
  %arrayidx24 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv1
  %tmp16 = load i32, i32* %arrayidx24, align 4
  %tmp17 = trunc i64 %indvars.iv1 to i32
  %add = add nsw i32 %tmp16, %tmp17
  %arrayidx26 = getelementptr inbounds i32, i32* %tmp14, i64 %indvars.iv1
  %tmp18 = load i32, i32* %arrayidx26, align 4
  %add27 = add nsw i32 %tmp18, %add
  store i32 %add27, i32* %arrayidx26, align 4
  br label %for.inc28

for.inc28:                                        ; preds = %S2
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond20

for.end30:                                        ; preds = %for.cond20
  br label %for.cond33

for.cond33:                                       ; preds = %for.inc39, %for.end30
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc39 ], [ 0, %for.end30 ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 100
  br i1 %exitcond6, label %for.body35, label %for.end41

for.body35:                                       ; preds = %for.cond33
  br label %S3

S3:                                               ; preds = %for.body35
  %arrayidx37 = getelementptr inbounds i32, i32* %tmp14, i64 %indvars.iv4
  %tmp19 = load i32, i32* %arrayidx37, align 4
  %tmp20 = trunc i64 %indvars.iv4 to i32
  %add38 = add nsw i32 %tmp19, %tmp20
  store i32 %add38, i32* %arrayidx37, align 4
  br label %for.inc39

for.inc39:                                        ; preds = %S3
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond33

for.end41:                                        ; preds = %for.cond33
  br label %for.cond44

for.cond44:                                       ; preds = %for.inc52, %for.end41
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc52 ], [ 0, %for.end41 ]
  %exitcond9 = icmp ne i64 %indvars.iv7, 100
  br i1 %exitcond9, label %for.body46, label %for.end54

for.body46:                                       ; preds = %for.cond44
  br label %S4

S4:                                               ; preds = %for.body46
  %arrayidx48 = getelementptr inbounds i32, i32* %tmp14, i64 %indvars.iv7
  %tmp21 = load i32, i32* %arrayidx48, align 4
  %arrayidx50 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv7
  %tmp22 = load i32, i32* %arrayidx50, align 4
  %add51 = add nsw i32 %tmp22, %tmp21
  store i32 %add51, i32* %arrayidx50, align 4
  br label %for.inc52

for.inc52:                                        ; preds = %S4
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond44

for.end54:                                        ; preds = %for.cond44
  br label %for.inc55

for.inc55:                                        ; preds = %for.end54
  %inc56 = add nuw nsw i32 %j.0, 1
  br label %for.cond3

for.end57:                                        ; preds = %for.cond3
  call void @free(i8* %call) #2
  ret void
}

declare noalias i8* @malloc(i64) #1

declare void @free(i8*) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0 (http://llvm.org/git/clang.git f304e39bb9d423169133175d08ec491ff7003e01)"}
