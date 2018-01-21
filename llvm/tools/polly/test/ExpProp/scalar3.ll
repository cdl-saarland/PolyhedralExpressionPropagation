; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int B[100], int A[100]) {
;      int N = 100;
;      int x = 0;
;      for (int i = 1; i < N; i++) {
;        if (i < 50)
;          x = 2;
;        else
;          x = 3;
;        B[i] = x;
;      }
;    }
;
; CHECK:    Statements {
; CHECK-DAG:           49 <= i0 <= 98
; CHECK-DAG:            0 <= i0 <= 48
; CHECK:    }
;
; IR: polly.stmt.if.end:                                ; preds = %polly.then
; IR:   %scevgep1 = getelementptr i32, i32* %scevgep, i64 %polly.indvar
; IR:   store i32 {{[2|3]}}, i32* %scevgep1
; IR:   br label %polly.merge
;
; IR: polly.stmt.if.end2:                               ; preds = %polly.else
; IR:   %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %polly.indvar
; IR:   store i32 {{[2|3]}}, i32* %scevgep4
; IR:   br label %polly.merge
;
source_filename = "../polly/test/ExpProp/scalar3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %B, i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 1, %entry ]
  %exitcond = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %cmp1 = icmp slt i64 %indvars.iv, 50
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  br label %if.end

if.else:                                          ; preds = %for.body
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %x.0 = phi i32 [ 2, %if.then ], [ 3, %if.else ]
  %arrayidx = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %x.0, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
