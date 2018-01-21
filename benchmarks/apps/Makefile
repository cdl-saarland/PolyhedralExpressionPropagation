DIRS = bilateral_grid harris camera_pipe interpolate local_laplacian pyramid_blending unsharp_mask

all: naive polymage

naive:
	    @-for d in $(DIRS); do \
			        make -C $$d naive; \
					        done

naive-parallel:
	    @-for d in $(DIRS); do \
			        make -C $$d naive-parallel; \
					        done

polly:
	    @-for d in $(DIRS); do \
			        make -C $$d polly; \
					        done

polly-parallel:
	    @-for d in $(DIRS); do \
			        make -C $$d polly-parallel; \
					        done

polly-prop:
	    @-for d in $(DIRS); do \
			        make -C $$d polly-prop; \
					        done

polly-prop-parallel:
	    @-for d in $(DIRS); do \
			        make -C $$d polly-prop-parallel; \
					        done

polymage:
	    @-for d in $(DIRS); do \
			        make -C $$d polymage; \
					        done

clean:
	    @-for d in $(DIRS); do \
			        make -C $$d clean; \
					        done
		rm -f *.pyc
check:
	    @-for d in $(DIRS); do \
			        make -C $$d check; \
					        done

base:
	    @-for d in $(DIRS); do \
			        make -C $$d base; \
					        done
