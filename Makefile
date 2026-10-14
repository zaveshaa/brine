CC        = cc
WARN      = -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wconversion
CFLAGS    = -std=c11 -O2 $(WARN) -Iinclude
LDFLAGS   = -lm
PREFIX   ?= /usr/local

ifeq ($(WERROR),1)
WARN     += -Werror
endif

CORE      = src/brine.c src/grid.c src/kernel.c src/bounds.c
HDR       = $(wildcard include/*.h)

DROP      ?= drop
DROP_SRC   = $(CORE) demos/drop.c

TEST      ?= brine_test
TEST_SRC   = $(CORE) tests/test_brine.c

GL        ?= brine_gl
GL_SRC     = $(CORE) demos/gl.c
GL_CFLAGS  = $(shell pkg-config --cflags glfw3 2>/dev/null)
GL_LIBS    = $(shell pkg-config --libs glfw3 2>/dev/null) -lGL
GL_OMP    ?= -fopenmp

all: $(DROP)

$(DROP): $(DROP_SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(DROP_SRC) $(LDFLAGS)

$(TEST): $(TEST_SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(LDFLAGS)

$(GL): $(GL_SRC) $(HDR)
	$(CC) $(CFLAGS) $(GL_OMP) $(GL_CFLAGS) -o $@ $(GL_SRC) $(GL_LIBS) $(LDFLAGS) $(GL_OMP)

test: $(TEST)
	./$(TEST)

run: $(DROP)
	./$(DROP)

gl: $(GL)

run-gl: $(GL)
	./$(GL)

help:
	@echo "brine: position-based fluids in C11, no dependencies"
	@echo ""
	@echo "  make          build $(DROP) (the terminal demo)"
	@echo "  make test     run the checks"
	@echo "  make run      watch a block of water settle (ASCII)"
	@echo "  make run-gl   watch it in a window (needs GLFW)"
	@echo "  make clean    remove the build"

clean:
	rm -f $(DROP) $(TEST) $(GL)

.PHONY: all test run gl run-gl help clean
