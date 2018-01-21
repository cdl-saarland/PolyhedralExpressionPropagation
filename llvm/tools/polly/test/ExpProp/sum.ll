; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-codegen < %s -o %t
; RUN: lli %t > %t_out_1
; RUN: lli %s > %t_out_2
; RUN: diff %t_out_1 %t_out_2
;
;    #include <stdio.h>
;
;    void f(double *d) {
;      if (d) {
;        double s = 1.0;
;        for (int i = 0; i < 100; i++)
;          s += 2.0 * i;
;        *d = s;
;      }
;    }
;
;    int main(int argc, char *argv[]) {
;      double d;
;      f(&d);
;      printf("f() = %lf\n", d);
;      return 0;
;    }
;
source_filename = "/home/johannes/projects/llvm/polly/test/ExpProp/sum.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@.str = private unnamed_addr constant [11 x i8] c"f() = %lf\0A\00", align 1

define void @f(double* %d) {
entry:
  %tobool = icmp eq double* %d, null
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %if.then
  %s.0 = phi double [ 1.000000e+00, %if.then ], [ %add, %for.inc ]
  %i.0 = phi i32 [ 0, %if.then ], [ %inc, %for.inc ]
  %exitcond = icmp ne i32 %i.0, 100
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %conv = sitofp i32 %i.0 to double
  %mul = fmul fast double %conv, 2.000000e+00
  %add = fadd fast double %s.0, %mul
  %inc = add nuw nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi double [ %s.0, %for.cond ]
  store double %s.0.lcssa, double* %d, align 8
  br label %if.end

if.end:                                           ; preds = %entry, %for.end
  ret void
}

define i32 @main(i32 %argc, i8** %argv) {
entry:
  %d = alloca double, align 8
  call void @f(double* nonnull %d)
  %tmp = load double, double* %d, align 8
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str, i64 0, i64 0), double %tmp) #2
  ret i32 0
}

declare i32 @printf(i8*, ...) #1
