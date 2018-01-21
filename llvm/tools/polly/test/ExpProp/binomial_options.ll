; RUN: opt %loadPolly -polly-exp -polly-exp-heuristic=0 -polly-opt-isl -polly-ast -analyze < %s | FileCheck %s --check-prefix=AST
;
; XFAIL: *
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
; AST:    if (
;
; AST:    {
; AST:      for (int c0 = 0; c0 < N; c0 += 1) {
; AST:        Stmt_for_body__TO__cond_end(c0);
; AST:        Stmt_cond_end(c0);
; AST:      }
; AST:      if (N == 1)
; AST:        for (int c1 = 0; c1 < floord(numSteps + 1, 2); c1 += 1)
; AST:          Stmt_if_then(0, c1);
; AST:      if (N >= 1) {
; AST:        for (int c1 = 0; c1 < floord(numSteps, 2); c1 += 1)
; AST:          Stmt_if_then87(0, c1);
; AST:        Stmt_if_then116(0);
; AST:        if (N >= 2 || (N == 1 && numSteps <= 1)) {
; AST:          Stmt_if_then116_0(0);
; AST:        } else {
; AST:          Stmt_if_then116_1(0);
; AST:        }
; AST:      }
; AST:    }
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @binomial_options(i32 %numSteps, float* %randArray, float* %output, i32 %N) {
entry:
  %tmp = zext i32 %N to i64
  %vla = alloca float, i64 %tmp, align 16
  %tmp7 = zext i32 %N to i64
  %vla1 = alloca float, i64 %tmp7, align 16
  %tmp8 = zext i32 %N to i64
  %vla2 = alloca float, i64 %tmp8, align 16
  %tmp9 = zext i32 %N to i64
  %vla3 = alloca float, i64 %tmp9, align 16
  %tmp10 = sext i32 %N to i64
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvars.iv5 = phi i64 [ %indvars.iv.next6, %for.inc ], [ 0, %entry ]
  %cmp = icmp slt i64 %indvars.iv5, %tmp10
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds float, float* %randArray, i64 %indvars.iv5
  %tmp11 = load float, float* %arrayidx, align 4
  %sub = fsub fast float 1.000000e+00, %tmp11
  %mul = fmul fast float %sub, 5.000000e+00
  %mul4 = fmul fast float %tmp11, 3.000000e+01
  %add = fadd fast float %mul, %mul4
  %tmp12 = fmul fast float %tmp11, 9.900000e+01
  %tmp13 = fadd fast float %tmp12, 1.000000e+00
  %sub9 = fsub fast float 1.000000e+00, %tmp11
  %mul10 = fmul fast float %sub9, 2.500000e-01
  %mul11 = fmul fast float %tmp11, 1.000000e+01
  %add12 = fadd fast float %mul10, %mul11
  %conv = sitofp i32 %numSteps to float
  %div = fdiv fast float 1.000000e+00, %conv
  %mul13 = fmul fast float %add12, %div
  %conv14 = fpext float %mul13 to double
  %tmp14 = call fast double @llvm.sqrt.f64(double %conv14)
  %mul15 = fmul fast double %tmp14, 0x3FD3333340000000
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
  %arrayidx31 = getelementptr inbounds float, float* %vla, i64 %indvars.iv5
  store float %mul29, float* %arrayidx31, align 4
  %mul32 = fmul fast float %sub28, %div20
  %arrayidx34 = getelementptr inbounds float, float* %vla1, i64 %indvars.iv5
  store float %mul32, float* %arrayidx34, align 4
  %conv35 = fpext float %add to double
  %tmp15 = trunc i64 %indvars.iv5 to i32
  %conv36 = sitofp i32 %tmp15 to float
  %mul37 = fmul fast float %conv36, 2.000000e+00
  %conv38 = sitofp i32 %numSteps to float
  %sub39 = fsub fast float %mul37, %conv38
  %mul40 = fmul fast float %sub39, %conv16
  %conv41 = fpext float %mul40 to double
  %call42 = call fast double @__exp_finite(double %conv41) #2
  %mul43 = fmul fast double %call42, %conv35
  %conv44 = fpext float %tmp13 to double
  %sub45 = fsub fast double %mul43, %conv44
  %conv46 = fptrunc double %sub45 to float
  %cmp47 = fcmp fast ogt float %conv46, 0.000000e+00
  br i1 %cmp47, label %cond.true, label %cond.false

cond.true:                                        ; preds = %for.body
  br label %cond.end

cond.false:                                       ; preds = %for.body
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi float [ %conv46, %cond.true ], [ 0.000000e+00, %cond.false ]
  %arrayidx50 = getelementptr inbounds float, float* %vla2, i64 %indvars.iv5
  store float %cond, float* %arrayidx50, align 4
  br label %for.inc

for.inc:                                          ; preds = %cond.end
  %indvars.iv.next6 = add nuw nsw i64 %indvars.iv5, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %tmp16 = sext i32 %N to i64
  br label %for.cond52

for.cond52:                                       ; preds = %for.inc106, %for.end
  %indvars.iv1 = phi i64 [ %indvars.iv.next2, %for.inc106 ], [ 0, %for.end ]
  %cmp53 = icmp slt i64 %indvars.iv1, %tmp16
  br i1 %cmp53, label %for.body55, label %for.end108

for.body55:                                       ; preds = %for.cond52
  br label %for.cond56

for.cond56:                                       ; preds = %for.inc76, %for.body55
  %j.0 = phi i32 [ %numSteps, %for.body55 ], [ %sub77, %for.inc76 ]
  %cmp57 = icmp sgt i32 %j.0, 0
  br i1 %cmp57, label %for.body59, label %for.end78

for.body59:                                       ; preds = %for.cond56
  %tmp17 = sext i32 %j.0 to i64
  %cmp60 = icmp slt i64 %indvars.iv1, %tmp17
  br i1 %cmp60, label %if.then, label %if.end

if.then:                                          ; preds = %for.body59
  %arrayidx63 = getelementptr inbounds float, float* %vla, i64 %indvars.iv1
  %tmp18 = load float, float* %arrayidx63, align 4
  %arrayidx65 = getelementptr inbounds float, float* %vla2, i64 %indvars.iv1
  %tmp19 = load float, float* %arrayidx65, align 4
  %mul66 = fmul fast float %tmp18, %tmp19
  %arrayidx68 = getelementptr inbounds float, float* %vla1, i64 %indvars.iv1
  %tmp20 = load float, float* %arrayidx68, align 4
  %tmp21 = add nuw nsw i64 %indvars.iv1, 1
  %arrayidx71 = getelementptr inbounds float, float* %vla2, i64 %tmp21
  %tmp22 = load float, float* %arrayidx71, align 4
  %mul72 = fmul fast float %tmp20, %tmp22
  %add73 = fadd fast float %mul66, %mul72
  %arrayidx75 = getelementptr inbounds float, float* %vla3, i64 %indvars.iv1
  store float %add73, float* %arrayidx75, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body59
  br label %for.inc76

for.inc76:                                        ; preds = %if.end
  %sub77 = add nsw i32 %j.0, -2
  br label %for.cond56

for.end78:                                        ; preds = %for.cond56
  br label %for.cond80

for.cond80:                                       ; preds = %for.inc103, %for.end78
  %j79.0 = phi i32 [ %numSteps, %for.end78 ], [ %sub104, %for.inc103 ]
  %cmp81 = icmp sgt i32 %j79.0, 0
  br i1 %cmp81, label %for.body83, label %for.end105

for.body83:                                       ; preds = %for.cond80
  %sub84 = add nsw i32 %j79.0, -1
  %tmp23 = sext i32 %sub84 to i64
  %cmp85 = icmp slt i64 %indvars.iv1, %tmp23
  br i1 %cmp85, label %if.then87, label %if.end102

if.then87:                                        ; preds = %for.body83
  %arrayidx89 = getelementptr inbounds float, float* %vla, i64 %indvars.iv1
  %tmp24 = load float, float* %arrayidx89, align 4
  %arrayidx91 = getelementptr inbounds float, float* %vla3, i64 %indvars.iv1
  %tmp25 = load float, float* %arrayidx91, align 4
  %mul92 = fmul fast float %tmp24, %tmp25
  %arrayidx94 = getelementptr inbounds float, float* %vla1, i64 %indvars.iv1
  %tmp26 = load float, float* %arrayidx94, align 4
  %tmp27 = add nuw nsw i64 %indvars.iv1, 1
  %arrayidx97 = getelementptr inbounds float, float* %vla3, i64 %tmp27
  %tmp28 = load float, float* %arrayidx97, align 4
  %mul98 = fmul fast float %tmp26, %tmp28
  %add99 = fadd fast float %mul92, %mul98
  %arrayidx101 = getelementptr inbounds float, float* %vla2, i64 %indvars.iv1
  store float %add99, float* %arrayidx101, align 4
  br label %if.end102

if.end102:                                        ; preds = %if.then87, %for.body83
  br label %for.inc103

for.inc103:                                       ; preds = %if.end102
  %sub104 = add nsw i32 %j79.0, -2
  br label %for.cond80

for.end105:                                       ; preds = %for.cond80
  br label %for.inc106

for.inc106:                                       ; preds = %for.end105
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  br label %for.cond52

for.end108:                                       ; preds = %for.cond52
  %tmp29 = sext i32 %N to i64
  br label %for.cond110

for.cond110:                                      ; preds = %for.inc121, %for.end108
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc121 ], [ 0, %for.end108 ]
  %cmp111 = icmp slt i64 %indvars.iv, %tmp29
  br i1 %cmp111, label %for.body113, label %for.end123

for.body113:                                      ; preds = %for.cond110
  %cmp114 = icmp eq i64 %indvars.iv, 0
  br i1 %cmp114, label %if.then116, label %if.end120

if.then116:                                       ; preds = %for.body113
  %tmp30 = bitcast float* %vla2 to i32*
  %tmp31 = load i32, i32* %tmp30, align 16
  %arrayidx119 = getelementptr inbounds float, float* %output, i64 %indvars.iv
  %tmp32 = bitcast float* %arrayidx119 to i32*
  store i32 %tmp31, i32* %tmp32, align 4
  br label %if.end120

if.end120:                                        ; preds = %if.then116, %for.body113
  br label %for.inc121

for.inc121:                                       ; preds = %if.end120
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond110

for.end123:                                       ; preds = %for.cond110
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
