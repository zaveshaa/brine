CC        = cc
WARN      = -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wconversion
CFLAGS    = -std=c11 -O2 $(WARN) -Iinclude -Idemos
LDFLAGS   = -lm
PREFIX   ?= /usr/local

ifeq ($(WERROR),1)
WARN     += -Werror
endif

CORE      = src/brine.c src/grid.c src/kernel.c src/bounds.c src/foam.c
HDR       = $(wildcard include/*.h)

DROP      ?= drop
DROP_SRC   = $(CORE) demos/drop.c

TEST      ?= brine_test
TEST_SRC   = $(CORE) demos/wheel.c tests/test_brine.c

GL        ?= brine_gl
GL_SRC     = $(CORE) demos/wheel.c demos/gl.c
GL_CFLAGS  = $(shell pkg-config --cflags glfw3 2>/dev/null)
GL_LIBS    = $(shell pkg-config --libs glfw3 2>/dev/null) -lGL
GL_OMP    ?= -fopenmp

GOLDEN    ?= tools/golden
GOLDEN_REF ?= tests/golden.ppm
GOLDEN_TOL ?= 6.0
GIF       ?= brine.gif

all: $(DROP)

$(DROP): $(DROP_SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(DROP_SRC) $(LDFLAGS)

$(TEST): $(TEST_SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(LDFLAGS)

$(GL): $(GL_SRC) $(HDR)
	$(CC) $(CFLAGS) $(GL_OMP) $(GL_CFLAGS) -o $@ $(GL_SRC) $(GL_LIBS) $(LDFLAGS) $(GL_OMP)

$(GOLDEN): tools/golden.c
	$(CC) $(CFLAGS) -o $@ tools/golden.c $(LDFLAGS)

test: $(TEST)
	./$(TEST)

run: $(DROP)
	./$(DROP)

gl: $(GL)

run-gl: $(GL)
	./$(GL)

golden: $(GL) $(GOLDEN)
	BRINE_WINDOW=480x304 BRINE_DUMP=/tmp/brine_golden.ppm BRINE_DUMP_FRAME=300 ./$(GL)
	./$(GOLDEN) /tmp/brine_golden.ppm $(GOLDEN_REF) $(GOLDEN_TOL)

golden-update: $(GL)
	BRINE_WINDOW=480x304 BRINE_DUMP=$(GOLDEN_REF) BRINE_DUMP_FRAME=300 ./$(GL)
	@echo "brine: wrote reference $(GOLDEN_REF)"

gif: $(GL)
	rm -f /tmp/brine_gif_*.ppm
	BRINE_GIF=/tmp/brine_gif BRINE_GIF_EVERY=2 BRINE_GIF_MAX=80 ./$(GL)
	ffmpeg -y -framerate 24 -start_number 0 -i /tmp/brine_gif_%04d.ppm \
		-vf "scale=640:-1:flags=lanczos,split[a][b];[a]palettegen[p];[b][p]paletteuse" \
		-loop 0 $(GIF)
	@echo "brine: wrote $(GIF)"

help:
	@echo "brine: position-based fluids in C11, no dependencies"
	@echo ""
	@echo "  make          build $(DROP) (the terminal demo)"
	@echo "  make test     run the checks"
	@echo "  make run      watch a block of water settle (ASCII)"
	@echo "  make run-gl   watch it in a window (needs GLFW)"
	@echo "  make gif      record a $(GIF) of the GL demo (needs ffmpeg)"
	@echo "  make golden   check the GL render against $(GOLDEN_REF)"
	@echo "  make clean    remove the build"

clean:
	rm -f $(DROP) $(TEST) $(GL) $(GOLDEN) $(GIF)

.PHONY: all test run gl run-gl golden golden-update gif help clean
