; RUN: opt %loadPolly -polly-exp -polly-prop-times=10 -polly-exp-heuristic=0 -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
;
;    #include <math.h>
;
;    #define RISKFREE 0.02f
;    #define VOLATILITY 0.30f
;
;    void binomial_options(int numSteps, const float *randArray, float *output,
;                          int N) {
;      float puByr[N];
;      float pdByr[N];
;      float callA[N];
;      float callB[N];
;
;      for (int i = 0; i < N; i++) {
;        float inRand = randArray[i];
;
;        float s = (1.0f - inRand) * 5.0f + inRand * 30.f;
;        float x = (1.0f - inRand) * 1.0f + inRand * 100.f;
;        float optionYears = (1.0f - inRand) * 0.25f + inRand * 10.f;
;        float dt = optionYears * (1.0f / (float)numSteps);
;        float vsdt = VOLATILITY * sqrt(dt);
;        float rdt = RISKFREE * dt;
;        float r = exp(rdt);
;        float rInv = 1.0f / r;
;        float u = exp(vsdt);
;        float d = 1.0f / u;
;        float pu = (r - d) / (u - d);
;        float pd = 1.0f - pu;
;        puByr[i] = pu * rInv;
;        pdByr[i] = pd * rInv;
;
;        float profit = s * exp(vsdt * (2.0f * i - (float)numSteps)) - x;
;        callA[i] = profit > 0 ? profit : 0.0f;
;      }
;
;      for (int i = 0; i < N; i++) {
;
;        for (int j = numSteps; j > 0; j -= 2) {
;          if (i < j) {
;            callB[i] = puByr[i] * callA[i] + pdByr[i] * callA[i + 1];
;          }
;        }
;      }
;
;      for (int i = 0; i < N; i++) {
;        for (int j = numSteps; j > 0; j -= 2) {
;
;          if (i < j - 1) {
;            callA[i] = puByr[i] * callB[i] + pdByr[i] * callB[i + 1];
;          }
;        }
;      }
;
;      for (int i = 0; i < N; i++) {
;        // write result for this block to global mem
;        if (i == 0)
;          output[i] = callA[0];
;      }
;    }
;
; AST: if (
;
; AST-NOT: Stmt_for_body
;
; AST:    if (N >= 1) {
; AST:      if (N == 2 && numSteps >= 2) {
; AST:        Stmt_if_then124(0);
; AST:      } else if (N == 1 && numSteps <= 1) {
; AST:        Stmt_if_then124_0(0);
; AST:      } else if (N >= 2 && numSteps <= 1) {
; AST:        Stmt_if_then124_0_0(0);
; AST:      } else if (N == 1) {
; AST:        Stmt_if_then124_1(0);
; AST:      } else if (N >= 4) {
; AST:        Stmt_if_then124_2_0(0);
; AST:      } else {
; AST:        Stmt_if_then124_2(0);
; AST:      }
; AST:    }
;
source_filename = "binopt.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @binomial_options(i32 %numSteps, float* %randArray, float* %output, i32 %N) {
entry:
  %tmp = zext i32 %N to i64
  %puByr = alloca float, i64 %tmp, align 16
  %tmp9 = zext i32 %N to i64
  %pdByr = alloca float, i64 %tmp9, align 16
  %tmp10 = zext i32 %N to i64
  %callA = alloca float, i64 %tmp10, align 16
  %tmp11 = zext i32 %N to i64
  %callB = alloca float, i64 %tmp11, align 16
  %tmp12 = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv7 = phi i64 [ %indvars.iv.next8, %for.inc ], [ 0, %entry ]
  %cmp = icmp slt i64 %indvars.iv7, %tmp12
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds float, float* %randArray, i64 %indvars.iv7
  %tmp13 = load float, float* %arrayidx, align 4
  %sub = fsub fast float 1.000000e+00, %tmp13
  %mul = fmul fast float %sub, 5.000000e+00
  %mul4 = fmul fast float %tmp13, 3.000000e+01
  %add = fadd fast float %mul, %mul4
  %tmp14 = fmul fast float %tmp13, 9.900000e+01
  %tmp15 = fadd fast float %tmp14, 1.000000e+00
  %sub9 = fsub fast float 1.000000e+00, %tmp13
  %mul10 = fmul fast float %sub9, 2.500000e-01
  %mul11 = fmul fast float %tmp13, 1.000000e+01
  %add12 = fadd fast float %mul10, %mul11
  %conv = sitofp i32 %numSteps to float
  %div = fdiv fast float 1.000000e+00, %conv
  %mul13 = fmul fast float %add12, %div
  %conv14 = fpext float %mul13 to double
  %tmp16 = call fast double @llvm.sqrt.f64(double %conv14)
  %mul15 = fmul fast double %tmp16, 0x3FD3333340000000
  %conv16 = fptrunc double %mul15 to float
  %mul17 = fmul fast float %mul13, 0x3F947AE140000000
  %conv18 = fpext float %mul17 to double
  %call = call fast double @__exp_finite(double %conv18) #2
  %conv19 = fptrunc double %call to float
  %div20 = fdiv fast float 1.000000e+00, %conv19
  %conv21 = fpext float %conv16 to double
  %call22 = call fast double @__exp_finite(double %conv21) #2
  %conv23 = fptrunc double %call22 to float
  %div24 = fdiv fast float 1.000000e+00, %conv23
  %sub25 = fsub fast float %conv19, %div24
  %sub26 = fsub fast float %conv23, %div24
  %div27 = fdiv fast float %sub25, %sub26
  %sub28 = fsub fast float 1.000000e+00, %div27
  %mul29 = fmul fast float %div27, %div20
  %arrayidx31 = getelementptr inbounds float, float* %puByr, i64 %indvars.iv7
  store float %mul29, float* %arrayidx31, align 4
  %mul32 = fmul fast float %sub28, %div20
  %arrayidx34 = getelementptr inbounds float, float* %pdByr, i64 %indvars.iv7
  store float %mul32, float* %arrayidx34, align 4
  %conv35 = fpext float %add to double
  %tmp17 = trunc i64 %indvars.iv7 to i32
  %conv36 = sitofp i32 %tmp17 to float
  %mul37 = fmul fast float %conv36, 2.000000e+00
  %conv38 = sitofp i32 %numSteps to float
  %sub39 = fsub fast float %mul37, %conv38
  %mul40 = fmul fast float %conv16, %sub39
  %conv41 = fpext float %mul40 to double
  %call42 = call fast double @__exp_finite(double %conv41) #2
  %mul43 = fmul fast double %conv35, %call42
  %conv44 = fpext float %tmp15 to double
  %sub45 = fsub fast double %mul43, %conv44
  %conv46 = fptrunc double %sub45 to float
  %cmp47 = fcmp fast ogt float %conv46, 0.000000e+00
  %cond = select i1 %cmp47, float %conv46, float 0.000000e+00
  %arrayidx50 = getelementptr inbounds float, float* %callA, i64 %indvars.iv7
  store float %cond, float* %arrayidx50, align 4
  br label %for.inc

for.inc:                                          ; preds = %cond.end
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp18 = sext i32 %N to i64
  br label %for.cond52

for.cond52:                                       ; preds = %for.inc79, %for.end
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for.inc79 ], [ 0, %for.end ]
  %cmp53 = icmp slt i64 %indvars.iv4, %tmp18
  br i1 %cmp53, label %for.body55, label %for.end81

for.body55:                                       ; preds = %for.cond52
  br label %for.cond56

for.cond56:                                       ; preds = %for.inc76, %for.body55
  %j.0 = phi i32 [ %numSteps, %for.body55 ], [ %sub77, %for.inc76 ]
  %cmp57 = icmp sgt i32 %j.0, 0
  br i1 %cmp57, label %for.body59, label %for.end78

for.body59:                                       ; preds = %for.cond56
  %tmp19 = sext i32 %j.0 to i64
  %cmp60 = icmp slt i64 %indvars.iv4, %tmp19
  br i1 %cmp60, label %if.then, label %if.end

if.then:                                          ; preds = %for.body59
  %arrayidx63 = getelementptr inbounds float, float* %puByr, i64 %indvars.iv4
  %tmp20 = load float, float* %arrayidx63, align 4
  %arrayidx65 = getelementptr inbounds float, float* %callA, i64 %indvars.iv4
  %tmp21 = load float, float* %arrayidx65, align 4
  %mul66 = fmul fast float %tmp20, %tmp21
  %arrayidx68 = getelementptr inbounds float, float* %pdByr, i64 %indvars.iv4
  %tmp22 = load float, float* %arrayidx68, align 4
  %tmp23 = add nuw nsw i64 %indvars.iv4, 1
  %arrayidx71 = getelementptr inbounds float, float* %callA, i64 %tmp23
  %tmp24 = load float, float* %arrayidx71, align 4
  %mul72 = fmul fast float %tmp22, %tmp24
  %add73 = fadd fast float %mul66, %mul72
  %arrayidx75 = getelementptr inbounds float, float* %callB, i64 %indvars.iv4
  store float %add73, float* %arrayidx75, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body59
  br label %for.inc76

for.inc76:                                        ; preds = %if.end
  %sub77 = add nsw i32 %j.0, -2
  br label %for.cond56

for.end78:                                        ; preds = %for.cond56
  br label %for.inc79

for.inc79:                                        ; preds = %for.end78
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  br label %for.cond52

for.end81:                                        ; preds = %for.cond52
  %tmp25 = sext i32 %N to i64
  br label %for.cond83

for.cond83:                                       ; preds = %for.inc114, %for.end81
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc114 ], [ 0, %for.end81 ]
  %cmp84 = icmp slt i64 %indvars.iv1, %tmp25
  br i1 %cmp84, label %for.body86, label %for.end116

for.body86:                                       ; preds = %for.cond83
  br label %for.cond88

for.cond88:                                       ; preds = %for.inc111, %for.body86
  %j87.0 = phi i32 [ %numSteps, %for.body86 ], [ %sub112, %for.inc111 ]
  %cmp89 = icmp sgt i32 %j87.0, 0
  br i1 %cmp89, label %for.body91, label %for.end113

for.body91:                                       ; preds = %for.cond88
  %sub92 = add nsw i32 %j87.0, -1
  %tmp26 = sext i32 %sub92 to i64
  %cmp93 = icmp slt i64 %indvars.iv1, %tmp26
  br i1 %cmp93, label %if.then95, label %if.end110

if.then95:                                        ; preds = %for.body91
  %arrayidx97 = getelementptr inbounds float, float* %puByr, i64 %indvars.iv1
  %tmp27 = load float, float* %arrayidx97, align 4
  %arrayidx99 = getelementptr inbounds float, float* %callB, i64 %indvars.iv1
  %tmp28 = load float, float* %arrayidx99, align 4
  %mul100 = fmul fast float %tmp27, %tmp28
  %arrayidx102 = getelementptr inbounds float, float* %pdByr, i64 %indvars.iv1
  %tmp29 = load float, float* %arrayidx102, align 4
  %tmp30 = add nuw nsw i64 %indvars.iv1, 1
  %arrayidx105 = getelementptr inbounds float, float* %callB, i64 %tmp30
  %tmp31 = load float, float* %arrayidx105, align 4
  %mul106 = fmul fast float %tmp29, %tmp31
  %add107 = fadd fast float %mul100, %mul106
  %arrayidx109 = getelementptr inbounds float, float* %callA, i64 %indvars.iv1
  store float %add107, float* %arrayidx109, align 4
  br label %if.end110

if.end110:                                        ; preds = %if.then95, %for.body91
  br label %for.inc111

for.inc111:                                       ; preds = %if.end110
  %sub112 = add nsw i32 %j87.0, -2
  br label %for.cond88

for.end113:                                       ; preds = %for.cond88
  br label %for.inc114

for.inc114:                                       ; preds = %for.end113
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond83

for.end116:                                       ; preds = %for.cond83
  %tmp32 = sext i32 %N to i64
  br label %for.cond118

for.cond118:                                      ; preds = %for.inc129, %for.end116
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc129 ], [ 0, %for.end116 ]
  %cmp119 = icmp slt i64 %indvars.iv, %tmp32
  br i1 %cmp119, label %for.body121, label %for.end131

for.body121:                                      ; preds = %for.cond118
  %cmp122 = icmp eq i64 %indvars.iv, 0
  br i1 %cmp122, label %if.then124, label %if.end128

if.then124:                                       ; preds = %for.body121
  %tmp33 = bitcast float* %callA to i32*
  %tmp34 = load i32, i32* %tmp33, align 16
  %arrayidx127 = getelementptr inbounds float, float* %output, i64 %indvars.iv
  %tmp35 = bitcast float* %arrayidx127 to i32*
  store i32 %tmp34, i32* %tmp35, align 4
  br label %if.end128

if.end128:                                        ; preds = %if.then124, %for.body121
  br label %for.inc129

for.inc129:                                       ; preds = %if.end128
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond118

for.end131:                                       ; preds = %for.cond118
  ret void
}

declare i8* @llvm.stacksave() #1

declare double @llvm.sqrt.f64(double) #2

declare double @__exp_finite(double) #3

declare void @llvm.stackrestore(i8*) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 (http://llvm.org/git/clang.git d7b64fd376dc1d420d32b0df8461fac965b4df53)"}
