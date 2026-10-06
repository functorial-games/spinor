# Repeatable 0 → 2π → 4π movie

This directory owns the Spinor-specific state trajectory and host frame
rendering. It deliberately stops at numbered stills.

The renderer calls the repository's existing semantic C directly:

- `native/spinor_core.c` for signed, unwrapped Spin(3) state;
- `native/spinor_field.c` for the ambient contraction;
- `native/spinor_ribbons.c` for the same six semantic ribbons used by Android.

It does not replay a captured phone session and it does not contain an MP4
encoder.

## Render stills

```sh
sh movie/render-frames.sh
```

Defaults:

- 240 frames;
- 480 × 480 PPM;
- 30 fps is the intended assembly clock;
- 0 → 2π over 3 seconds;
- hold at 2π for 1 second;
- 2π → 4π over 3 seconds;
- hold at 4π for 1 second.

`trajectory.tsv` records the exact angle and semantic checkpoint for every
frame. `frames.sha256` records every numbered still.

Override the frame rasterization inputs explicitly:

```sh
SPINOR_MOVIE_FRAMES=120 \
SPINOR_MOVIE_WIDTH=360 \
SPINOR_MOVIE_HEIGHT=360 \
sh movie/render-frames.sh build/movie/frames
```

## Assemble the MP4

Movie assembly belongs to the canonical Kitchen
`tasks/movie-from-stills/build.sh` boundary:

```sh
sh /path/to/kitchen/tasks/movie-from-stills/build.sh \
  'build/movie/frames/frame-%06d.ppm' \
  30 \
  build/movie/spinor-4pi-demo.mp4
```

The checked-in GitHub workflow pins a concrete Kitchen revision, renders the
stills, verifies the video frame count, writes a receipt, and publishes the
MP4/trajectory/receipt as both a workflow artifact and assets on the existing
`v0.0.1-pre.1` prerelease.

Flexible Pipes owns the registered repeatable cross-repository invocation;
Kitchen owns the pasteable composition script and the stills → MP4 encoder.
