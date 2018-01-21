#CXX=icpc
#CXX_FLAGS=-qopenmp -ipo -fp-model fast=1 -ftz -O3 -xhost -fPIC -shared

#GXX=g++
#GXX_FLAGS=-fopenmp -ffast-math -march=native -O3 -fPIC -shared
#GXX_FLAGS=-fopenmp  -O3 -fPIC -shared

CXX=clang++
CXX_FLAGS=-O3 -fPIC -shared -ffast-math -march=native -mtune=native
#CXX_FLAGS+=-mllvm -vectorizer-min-trip-count=8

DEBUG_FLAGS=-mllvm -debug-only=polly-opt-isl,polly-ast,polly-codegen,polly-exp-light


all: $(APP) graph

polymage: $(APP)

naive naive-parallel polly polly-parallel polly-prop polly-prop-parallel: $(APP)_naive

polymage naive-parallel polly-parallel polly-prop-parallel: PARA_FLAGS=-fopenmp=libomp

polly polly-parallel polly-prop polly-prop-parallel: POLLY_FLAGS=-mllvm -polly -mllvm -polly-ignore-aliasing
polly polly-parallel polly-prop polly-prop-parallel: POLLY_FLAGS+=-mllvm -polly-dependences-computeout=0
polly polly-parallel polly-prop polly-prop-parallel: POLLY_FLAGS+=-mllvm -polly-run-inliner
polly polly-parallel polly-prop polly-prop-parallel: POLLY_FLAGS+=-mllvm -polly-allow-nonaffine
polly polly-parallel polly-prop polly-prop-parallel: POLLY_FLAGS+=-mllvm -simplifycfg-sink-common=false

polly-prop polly-prop-parallel: EXP_FLAGS=-mllvm -polly-enable-propagation
#polly-prop polly-prop-parallel: EXP_FLAGS+=-mllvm -polly-prop-all-but-prop

polly-parallel polly-prop-parallel: POLLY_PARA_FLAGS=-mllvm -polly-parallel
polly-parallel polly-prop-parallel: POLLY_PARA_FLAGS+=-mllvm -polly-parallel-force
#polly-parallel polly-prop-parallel: POLLY_PARA_FLAGS+=-mllvm -polly-gomp-static
#polly-parallel polly-prop-parallel: POLLY_PARA_FLAGS+=-mllvm -polly-gomp-hyperthreading
naive-parallel: PARA_FLAGS+=-DENABLE_OMP_PRAGMAS


$(APP)_opt.so: $(APP)_polymage.cpp
	$(CXX) $(CXX_FLAGS) $(PARA_FLAGS) $< -o $@ &> out.ll
	#$(GXX) $(GXX_FLAGS)  $< -o $@ &> out.ll

$(APP)_naive.so: $(APP)_polymage_naive.cpp
	$(CXX) $(CXX_FLAGS) $(PARA_FLAGS) $(POLLY_FLAGS) $(EXP_FLAGS) $(POLLY_PARA_FLAGS) $(DEBUG_FLAGS) $(EXTRA_FLAGS) $< -o $@ &> out.ll

graph: graph.png

graph.png: graph.dot
	dot -Tpng $< > $@

clean:
	rm -f *.pyc *.so *.ll graph.png result.jpg

check:
	compare base.jpg result.jpg diff.jpg; if [ $$? -ne 0 ] ; then echo "Result differs for $(APP)"; compare -metric mae base.jpg result.jpg diff2.jpg; fi

base: polymage
	cp result.jpg base.jpg
