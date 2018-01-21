; RUN: opt %loadPolly -polly-scops -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-function-scops -analyze < %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"

; Derived from the following code:
;
; void foo(long n, double A[n][n]) {
;   for (long j = 0; j < n; j++)
;     for (long i = 0; i < n; i++)
;       A[i][j] = 1.0;
; }


; CHECK: Assumed Context:
; CHECK:   [n] -> {  :  }

; CHECK: p0: %n
; CHECK-NOT: p1

; CHECK: Schedule :=
; CHECK:   [n] -> { Stmt_for_i[i0] -> [i0] };
; CHECK: MustWriteAccess :=
; CHECK:   [n] -> { Stmt_for_i[i0] -> MemRef_A[i0, i0] };


define void @foo(i64 %n, double* %A) {
entry:
  br label %for.j

for.j:
  %j = phi i64 [ 0, %entry ], [ %j.inc, %for.i.end ]
  %j.inc = add nsw i64 %j, 1
  %j.exitcond = icmp eq i64 %j.inc, %n
  br i1 %j.exitcond, label %end, label %for.i

for.i:
  %i = phi i64 [ 0, %for.j ], [ %i.inc, %for.i ]
  %tmp = mul nsw i64 %i, %n
  %vlaarrayidx.sum = add i64 %j, %tmp
  %arrayidx = getelementptr inbounds double, double* %A, i64 %vlaarrayidx.sum
  store double 1.0, double* %arrayidx
  %i.inc = add nsw i64 %i, 1
  %i.exitcond = icmp eq i64 %i.inc, %n
  br i1 %i.exitcond, label %for.i.end, label %for.i

for.i.end:
  br label %for.j

end:
  ret void
}
