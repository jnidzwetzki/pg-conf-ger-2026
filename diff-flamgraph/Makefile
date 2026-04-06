PERF = perf
FLAMEGRAPH_DIR ?= $(HOME)/FlameGraph

.PHONY: all process-all old new superfast diff clean

all: process-all

process-all: old.svg new.svg superfast.svg

old.svg: old.perf
	@echo "Processing $< into flamegraph..."
	$(PERF) script -i $< > old.stacks
	$(FLAMEGRAPH_DIR)/stackcollapse-perf.pl old.stacks > old.folded
	$(FLAMEGRAPH_DIR)/flamegraph.pl old.folded > $@
	@echo "Done: $@"

new.svg: new.perf
	@echo "Processing $< into flamegraph..."
	$(PERF) script -i $< > new.stacks
	$(FLAMEGRAPH_DIR)/stackcollapse-perf.pl new.stacks > new.folded
	$(FLAMEGRAPH_DIR)/flamegraph.pl new.folded > $@
	@echo "Done: $@"

superfast.svg: superfast.perf
	@echo "Processing $< into flamegraph..."
	$(PERF) script -i $< > superfast.stacks
	$(FLAMEGRAPH_DIR)/stackcollapse-perf.pl superfast.stacks > superfast.folded
	$(FLAMEGRAPH_DIR)/flamegraph.pl superfast.folded > $@
	@echo "Done: $@"

old: old.svg

new: new.svg

superfast: superfast.svg

diff: old.folded new.folded
	@echo "Generating diff flamegraph..."
	$(FLAMEGRAPH_DIR)/difffolded.pl old.folded new.folded | $(FLAMEGRAPH_DIR)/flamegraph.pl > diff.svg
	@echo "Done: diff.svg"

clean:
	rm -f old.stacks old.folded old.svg new.stacks new.folded new.svg superfast.stacks superfast.folded superfast.svg diff.svg
