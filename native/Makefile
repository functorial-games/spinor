ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
BUILD ?= $(ROOT)/build/native
MOVIE ?= $(ROOT)/build/movie/render-frames
ICK ?=
ICK_FLAGS ?= -fno-link-libatomic
SOURCES = $(ROOT)/native/spinor_core.c $(ROOT)/native/spinor_field.c $(ROOT)/native/spinor_ribbons.c
WARNINGS = -std=c11 -Wall -Wextra -Werror -pedantic

.PHONY: test movie
test:
	test -x "$(ICK)"
	mkdir -p "$(BUILD)"
	"$(ICK)" $(ICK_FLAGS) $(WARNINGS) "$(ROOT)/native/spinor_core.c" "$(ROOT)/native/test_spinor_core.c" -lm -o "$(BUILD)/core-test"
	"$(BUILD)/core-test"
	"$(ICK)" $(ICK_FLAGS) $(WARNINGS) $(SOURCES) "$(ROOT)/native/test_spinor_field.c" -lm -o "$(BUILD)/field-test"
	"$(BUILD)/field-test"
movie:
	test -x "$(ICK)"
	mkdir -p "$(dir $(MOVIE))"
	"$(ICK)" $(ICK_FLAGS) -std=c11 -O2 -Wall -Wextra -Werror "$(ROOT)/movie/render_frames.c" $(SOURCES) -lm -o "$(MOVIE)"
