; RUN: opt %loadPolly -polly-enable-propagation  -polly-process-unprofitable=true -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-enable-propagation  -polly-process-unprofitable=false -polly-exp -polly-exp-heuristic=0 -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-enable-propagation  -polly-process-unprofitable=false -polly-exp -polly-exp-heuristic=0 -polly-codegen -S < %s | FileCheck %s --check-prefix=IR
;
;    void f(int *A) {
;      for (int i = 0; i < 100; i++) {
;        int y;
;        for (int h = 0; h < 100; h++) {
;          int x = i + h % 5;
;          // split
;          A[i] = A[i] + x;
;          y = x % 3;
;        }
;        A[i] += y;
;      }
;    }
;
; CHECK:         Invalid Context:
; CHECK-NEXT:    {  : 1 = 0 }
;
; CHECK:         	Stmt_for_body3_split	 [#Recompute Values: 1]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_body3_split[i0, i1] : 0 <= i0 <= 99 and 0 <= i1 <= 94 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_body3_split[i0, i1] -> [i0, 0, i1] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body3_split[i0, i1] -> MemRef_A[i0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_body3_split[i0, i1] -> MemRef_A[i0] };
; CHECK:         	Stmt_for_end	 [#Recompute Values: 2]
; CHECK-NEXT:            Domain :=
; CHECK-NEXT:                { Stmt_for_end[i0] : 0 <= i0 <= 99 };
; CHECK-NEXT:            Schedule :=
; CHECK-NEXT:                { Stmt_for_end[i0] -> [i0, 1, 0] };
; CHECK-NEXT:            MustWriteAccess :=	[Reduction Type: +] [Scalar: 0]
; CHECK-NEXT:                { Stmt_for_end[i0] -> MemRef_A[i0] };
; CHECK-NEXT:            ReadAccess :=	[Reduction Type: NONE] [Scalar: 0]
; CHECK-NEXT:                null;
; CHECK-NEXT:           new: { Stmt_for_end[i0] -> MemRef_A[i0] };
;
; IR:  polly.stmt.for.end:
; IR:    %p_tmp9 = trunc i64 %polly.indvar to i32
; IR:    %p_rem10 = srem i32 95, 5
; IR:    %p_add11 = add nsw i32 %p_tmp9, %p_rem10
; IR:    %p_arrayidx = getelementptr inbounds i32, i32* %A, i64 %polly.indvar
; IR:    %p_tmp2 = load i32, i32* %p_arrayidx, align 4, !alias.scope !0, !noalias !2
; IR:    %p_add412 = add nsw i32 %p_tmp2, %p_add11
; IR:    %p_tmp13 = trunc i64 %polly.indvar to i32
; IR:    %p_rem14 = srem i32 96, 5
; IR:    %p_add15 = add nsw i32 %p_tmp13, %p_rem14
; IR:    %p_add416 = add nsw i32 %p_add412, %p_add15
; IR:    %p_tmp17 = trunc i64 %polly.indvar to i32
; IR:    %p_rem18 = srem i32 97, 5
; IR:    %p_add19 = add nsw i32 %p_tmp17, %p_rem18
; IR:    %p_add420 = add nsw i32 %p_add416, %p_add19
; IR:    %p_tmp21 = trunc i64 %polly.indvar to i32
; IR:    %p_rem22 = srem i32 98, 5
; IR:    %p_add23 = add nsw i32 %p_tmp21, %p_rem22
; IR:    %p_add424 = add nsw i32 %p_add420, %p_add23
; IR:    %p_tmp25 = trunc i64 %polly.indvar to i32
; IR:    %p_rem26 = srem i32 99, 5
; IR:    %p_add27 = add nsw i32 %p_tmp25, %p_rem26
; IR:    %p_add428 = add nsw i32 %p_add424, %p_add27
; IR:    %p_tmp29 = trunc i64 %polly.indvar to i32
; IR:    %p_rem30 = srem i32 99, 5
; IR:    %p_add31 = add nsw i32 %p_tmp29, %p_rem30
; IR:    %p_rem5 = srem i32 %p_add31, 3
; IR:    %p_add8 = add nsw i32 %p_add428, %p_rem5
; IR:    %scevgep32 = getelementptr i32, i32* %A, i64 %polly.indvar
; IR:    store i32 %p_add8, i32* %scevgep32
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/profitable_2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc9, %entry
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc9 ], [ 0, %entry ]
  %y.0 = phi i32 [ undef, %entry ], [ %y.1.lcssa, %for.inc9 ]
  %exitcond1 = icmp ne i64 %indvars.iv, 100
  br i1 %exitcond1, label %for.body, label %for.end11

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %y.1 = phi i32 [ %y.0, %for.body ], [ %rem5, %for.inc ]
  %h.0 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ]
  %exitcond = icmp ne i32 %h.0, 100
  br i1 %exitcond, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %rem = srem i32 %h.0, 5
  %tmp = trunc i64 %indvars.iv to i32
  %add = add nsw i32 %tmp, %rem
  br label %for.body3.split

for.body3.split:
  %arrayidx = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp2 = load i32, i32* %arrayidx, align 4
  %add4 = add nsw i32 %tmp2, %add
  store i32 %add4, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %rem5 = srem i32 %add, 3
  %inc = add nuw nsw i32 %h.0, 1
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  %y.1.lcssa = phi i32 [ %y.1, %for.cond1 ]
  %arrayidx7 = getelementptr inbounds i32, i32* %A, i64 %indvars.iv
  %tmp3 = load i32, i32* %arrayidx7, align 4
  %add8 = add nsw i32 %tmp3, %y.1.lcssa
  store i32 %add8, i32* %arrayidx7, align 4
  br label %for.inc9

for.inc9:                                         ; preds = %for.end
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond

for.end11:                                        ; preds = %for.cond
  ret void
}
