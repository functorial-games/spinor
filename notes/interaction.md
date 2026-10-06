# Direct-manipulation interaction sketch

This is the initial product idea, not a commitment to a particular engine or deformation algorithm.

## Core interaction

The object should behave like something a person can **pick up and investigate**, not like an animation player.

Primary gesture:

1. pointer/touch down on the central body or a manipulable ribbon region;
2. drag;
3. convert the drag into a continuous change of the selected rigid orientation / deformation handle;
4. recompute the attached spinor/ribbon field immediately;
5. release at any point and leave the object in that state.

Reverse dragging must retrace the state cleanly. A person should be able to stop just before or after 2π, back up, go forward to 4π, and discover what changes without following a scripted sequence.

## State model

Keep these separate:

- **physical orientation** in (SO(3));
- **lifted spinor state** in (SU(2));
- **history/homotopy state** needed to distinguish paths that end at the same (SO(3)) orientation;
- **deformation field** mapping the history into visible ribbons/surfaces;
- **camera**.

A 360° return of the visible central body's ordinary orientation must not erase the information carried by the lift/ribbons.

This separation also prevents a common UI error: camera orbit must not silently become object rotation. The gesture system needs an explicit rule for “grab object” versus “move viewpoint”.

## First vertical slice

Do not start with cloth dynamics.

Start with a deterministic, mathematically controlled six-ribbon construction:

- one central cube/body;
- six connections to an outer frame or six fixed anchors;
- pointer/touch dragging controls the central orientation;
- a licensed/reference deformation field maps that orientation/history to ribbon geometry;
- GPU or CPU geometry update is allowed, but the same state should be inspectable in tests;
- 0, 2π, and 4π should be reproducible exact checkpoints.

The Amir Leidel `spinny` motor-interpolation construction is a strong licensed baseline. A simpler quaternion field can be used for the first slice if it preserves the same topology and endpoint constraints.

## Tests that matter

The interaction should be testable below the rendering layer.

At minimum:

- (q(0)=+1);
- central (SO(3)) orientation at (2π) equals the orientation at 0;
- lifted spinor at (2π) is (-1), not (+1);
- lifted spinor at (4π) returns to (+1);
- fixed outer anchors remain fixed under the canonical motion;
- drag forward then exactly backward returns state and geometry within numerical tolerance;
- pause/release/resume does not reset the path class;
- touch and mouse drive the same state transition code;
- camera motion changes no spinor/deformation state;
- rendering can be disabled while the state/deformation tests still run.

A visual regression test should additionally verify that the canonical six-ribbon path does not pass through obvious self-intersection artifacts introduced by our discretization.

## Interaction mapping questions

A single-axis first slice is acceptable: map horizontal drag to a signed continuous spin angle while the selected body is grabbed.

The next useful interaction is not “more animation buttons”; it is a true 3D orientation grab. Possible mappings to investigate:

- virtual trackball / arcball controlling a quaternion;
- screen-space drag projected against a picked face/handle;
- explicit rotation-ring handles;
- direct manipulation of a ribbon point with inverse solving back to admissible field state.

The lifted state must remember the path through orientation space. Reducing every frame to the central body's final quaternion in (SO(3)) would throw away exactly the information the toy is meant to expose.

## Rendering boundary

Keep rendering downstream from the mathematical field:

```
pointer/touch
  -> interaction state
  -> lifted rotation / homotopy state
  -> deformation field
  -> sampled ribbons/surfaces
  -> renderer
```

That lets us later swap Three.js/WebGL for another renderer, including a Sokol path, without redefining the mathematics.

## Deferred experiments

Record these without putting them in the first slice:

- replace narrow ribbons with general deforming surfaces;
- extend the interaction from ribbons to Seifert surfaces with explicit boundary links and orientation;
- arbitrary number of connection points;
- connection points placed on different bodies or on a continuous boundary;
- movable outer anchors;
- branching attachment graphs;
- compare Hise-style antitwister fields, CGA motor interpolation, quaternion fields, and physically relaxed surfaces;
- use the same interaction model for other spinor/covering-space demonstrations.

The first version only needs to establish that direct manipulation makes the (2π) versus (4π) distinction more intelligible than passive playback.
