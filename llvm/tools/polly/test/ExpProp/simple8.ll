; RUN: opt %loadPolly -polly-exp -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
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
;      for (int i = 0; i < 100; i++)
;      S1:
;        tmp[i] = A[i];
;
;      for (int i = 0; i < 100; i++)
;      S2:
;        tmp[i] += A[i] + i;
;
;      for (int i = 0; i < 100; i++)
;      S3:
;        tmp[i] += i;
;
;      for (int i = 0; i < 100; i++)
;      S4:
;        A[i] += tmp[i];
;
;      free(tmp);
;    }
;
; CHECK:  { Stmt_S0[i0] : 1 = 0 };
; CHECK:  { Stmt_S1[i0] : 1 = 0 };
; CHECK:  { Stmt_S2[i0] : 1 = 0 };
; CHECK:  { Stmt_S3[i0] : 1 = 0 };
; CHECK:  { Stmt_S4[i0] : 0 <= i0 <= 99 };
;
; IR: polly.stmt.S4:
; IR:   %p_tmp = trunc i64 %polly.indvar to i32
; IR:   %p_tmp1 = trunc i64 %polly.indvar to i32
; IR:   %p_tmp2 = trunc i64 %polly.indvar to i32
; IR:   %p_tmp16 = trunc i64 %polly.indvar to i32
; IR:   %p_add = add nsw i32 %p_tmp1, %p_tmp16
; IR:   %p_add23 = add nsw i32 %p_tmp2, %p_add
; IR:   %p_tmp19 = trunc i64 %polly.indvar to i32
; IR:   %p_add34 = add nsw i32 %p_add23, %p_tmp19
; IR:   %p_add47 = add nsw i32 %p_tmp, %p_add34
; IR:   %scevgep = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:   store i32 %p_add47
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/simple8.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A) #0 {
entry:
  %call = call noalias i8* @malloc(i64 400) #2
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv10 = phi i64 [ %indvars.iv.next11, %for.inc ], [ 0, %entry ]
  %exitcond12 = icmp ne i64 %indvars.iv10, 100
  br i1 %exitcond12, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %S0

S0:                                               ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv10
  %tmp = trunc i64 %indvars.iv10 to i32
  store i32 %tmp, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %S0
  %indvars.iv.next11 = add nuw nsw i64 %indvars.iv10, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp13 = bitcast i8* %call to i32*
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc11, %for.end
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc11 ], [ 0, %for.end ]
  %exitcond9 = icmp ne i64 %indvars.iv7, 100
  br i1 %exitcond9, label %for.body6, label %for.end13

for.body6:                                        ; preds = %for.cond4
  br label %S1

S1:                                               ; preds = %for.body6
  %arrayidx8 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv7
  %tmp14 = load i32, i32* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds i32, i32* %tmp13, i64 %indvars.iv7
  store i32 %tmp14, i32* %arrayidx10, align 4
  br label %for.inc11

for.inc11:                                        ; preds = %S1
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond4

for.end13:                                        ; preds = %for.cond4
  br label %for.cond16

for.cond16:                                       ; preds = %for.inc24, %for.end13
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc24 ], [ 0, %for.end13 ]
  %exitcond6 = icmp ne i64 %indvars.iv4, 100
  br i1 %exitcond6, label %for.body18, label %for.end26

for.body18:                                       ; preds = %for.cond16
  br label %S2

S2:                                               ; preds = %for.body18
  %arrayidx20 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv4
  %tmp15 = load i32, i32* %arrayidx20, align 4
  %tmp16 = trunc i64 %indvars.iv4 to i32
  %add = add nsw i32 %tmp15, %tmp16
  %arrayidx22 = getelementptr inbounds i32, i32* %tmp13, i64 %indvars.iv4
  %tmp17 = load i32, i32* %arrayidx22, align 4
  %add23 = add nsw i32 %tmp17, %add
  store i32 %add23, i32* %arrayidx22, align 4
  br label %for.inc24

for.inc24:                                        ; preds = %S2
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond16

for.end26:                                        ; preds = %for.cond16
  br label %for.cond29

for.cond29:                                       ; preds = %for.inc35, %for.end26
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc35 ], [ 0, %for.end26 ]
  %exitcond3 = icmp ne i64 %indvars.iv1, 100
  br i1 %exitcond3, label %for.body31, label %for.end37

for.body31:                                       ; preds = %for.cond29
  br label %S3

S3:                                               ; preds = %for.body31
  %arrayidx33 = getelementptr inbounds i32, i32* %tmp13, i64 %indvars.iv1
  %tmp18 = load i32, i32* %arrayidx33, align 4
  %tmp19 = trunc i64 %indvars.iv1 to i32
  %add34 = add nsw i32 %tmp18, %tmp19
  store i32 %add34, i32* %arrayidx33, align 4
  br label %for.inc35

for.inc35:                                        ; preds = %S3
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond29

for.end37:                                        ; preds = %for.cond29
  br label %for.cond40

for.cond40:                                       ; preds = %for.inc48, %for.end37
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc48 ], [ 0, %for.end37 ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body42, label %for.end50

for.body42:                                       ; preds = %for.cond40
  br label %S4

S4:                                               ; preds = %for.body42
  %arrayidx44 = getelementptr inbounds i32, i32* %tmp13, i64 %indvars.iv
  %tmp20 = load i32, i32* %arrayidx44, align 4
  %arrayidx46 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp21 = load i32, i32* %arrayidx46, align 4
  %add47 = add nsw i32 %tmp21, %tmp20
  store i32 %add47, i32* %arrayidx46, align 4
  br label %for.inc48

for.inc48:                                        ; preds = %S4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond40

for.end50:                                        ; preds = %for.cond40
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
