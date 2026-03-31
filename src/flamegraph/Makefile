CC = gcc
CFLAGS = -O2 -g -Wall -Wextra
TARGET = flamegraph-workload

PERF = perf
PERF_FREQUENCY = 111
PERF_DATA = flamegraph.perf
STACKS_FILE = flamegraph.stacks
FOLDED_FILE = flamegraph.folded
SVG_FILE = flamegraph.svg
FLAMEGRAPH_DIR ?= $(HOME)/FlameGraph

.PHONY: all clean flamegraph flamegraph-process flamegraph-perf-internal

all: $(TARGET)

$(TARGET): flame.c
	$(CC) $(CFLAGS) -o $@ flame.c

flamegraph: $(PERF_DATA)
	@echo "perf data recorded in $(PERF_DATA). Run make flamegraph-process."

$(PERF_DATA): $(TARGET)
	@echo "Recording perf data for $(TARGET) ..."
	$(PERF) record -g -F $(PERF_FREQUENCY) -o $@ -- ./$(TARGET)
	@echo "perf record finished"

flamegraph-process: $(PERF_DATA)
	@echo "Converting perf data to stack traces..."
	$(PERF) script -i $(PERF_DATA) > $(STACKS_FILE)
	@echo "Collapsing perf stacks..."
	$(FLAMEGRAPH_DIR)/stackcollapse-perf.pl $(STACKS_FILE) > $(FOLDED_FILE)
	@echo "Generating SVG..."
	$(FLAMEGRAPH_DIR)/flamegraph.pl $(FOLDED_FILE) > $(SVG_FILE)
	@echo "Done: $(SVG_FILE)"

flamegraph-perf-internal: $(PERF_DATA)
	@echo "Generating flamegraph report from $(PERF_DATA)..."
	@cp -f $(PERF_DATA) perf.data
	$(PERF) script report flamegraph --allow-download

clean:
	rm -f $(TARGET) $(PERF_DATA) $(STACKS_FILE) $(FOLDED_FILE) $(SVG_FILE) flamegraph.html perf.data perf.data.old
