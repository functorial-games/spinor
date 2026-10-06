#!/bin/sh
set -eu

cc \
  -std=c11 \
  -Wall -Wextra -Werror -pedantic \
  native/spinor_core.c \
  native/test_spinor_core.c \
  -lm \
  -o /tmp/spinor-core-test

/tmp/spinor-core-test

cc \
  -std=c11 \
  -Wall -Wextra -Werror -pedantic \
  native/spinor_core.c \
  native/spinor_field.c \
  native/spinor_ribbons.c \
  native/test_spinor_field.c \
  -lm \
  -o /tmp/spinor-field-test

/tmp/spinor-field-test
