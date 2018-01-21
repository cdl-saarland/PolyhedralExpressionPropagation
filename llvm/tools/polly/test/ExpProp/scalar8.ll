; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s
;
;    int g(int);
;    void f(int B[100], int A[100]) {
;      int N = 100;
;      for (int i = 1; i < N; i++) {
;        int x = g(i);
;        // split
;        B[i] = x;
;      }
;    }
;
; CHECK:      polly.stmt.for.body.cont:
;
; CHECK-NEXT:  %0 = add nsw i64 %polly.indvar, 1
; CHECK-NEXT:  %p_tmp = trunc i64 %0 to i32
; CHECK-NEXT:  %p_call = call i32 @g(i32 %p_tmp)
; CHECK-NEXT:  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; CHECK-NEXT:  store i32 %p_call, i32* %scevgep1
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar8.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp = trunc i64 %indvars.iv to i32
  %call = call i32 @g(i32 %tmp)
  br label %for.body.cont

for.body.cont:
  %arrayidx = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %call, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

declare i32 @g(i32) #1

attributes #1 = { nounwind readnone }
