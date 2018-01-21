; RUN: opt %loadPolly -polly-prepare -polly-exp -polly-exp-heuristic=0 -polly-opt-isl -polly-ast -analyze < %s | FileCheck %s --check-prefix=OPT
; opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-opt-isl -polly-ast -analyze < %s | FileCheck %s --check-prefix=OPT_DCE_BENEFICIAL
;
;    int f(int *A, int *B, int N) {
;      int x = 21;
;      for (int i = 0; i < N; i++) {
;        x = A[i];
;        B[i] = A[i];
;      }
;      return x;
;    }
;
; OPT:         if (1
; Fusion min:
;              {
;                if (N >= 1) {
;                  Stmt_for_inc(N - 1);
;                } else if (N <= -1) {
;                  Stmt_for_cond(0);
;                }
;                if (N >= 0)
;                  Stmt_for_cond(N);
;                for (int c0 = 0; c0 < N; c0 += 1)
;                  Stmt_for_body_split(c0);
;              }
; OPT:         {
; OPT-NEXT:      if (N <= -1) {
; OPT-NEXT:        Stmt_for_cond(0);
; OPT-NEXT:      } else {
; OPT-NEXT:        if (N >= 1)
; OPT-NEXT:          Stmt_for_inc(N - 1);
; OPT-NEXT:        Stmt_for_cond(N);
; OPT-NEXT:      }
; OPT-NEXT:      for (int c0 = 0; c0 < N; c0 += 1)
; OPT-NEXT:        Stmt_for_body_split(c0);
; OPT-NEXT:    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/scalar_return_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define i32 @f(i32* %A, i32* %B, i32 %N) {
entry:
  %tmp = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %entry ]
  %x.0 = phi i32 [ 21, %entry ], [ %tmp1, %for.inc ]
  %cmp = icmp slt i64 %indvars.iv, %tmp
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx2, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %B, i64 %indvars.iv
  store i32 %tmp2, i32* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi i32 [ %x.0, %for.cond ]
  ret i32 %x.0.lcssa
}

