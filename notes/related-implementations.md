# Related implementations

Research pass: 2026-10-06.

These are implementation references, not a requirement to copy their architecture.

## Amir Leidel — `spinny`

Repository:

https://github.com/amirleidel/spinny

License: MIT.

This is the strongest source-code reference found in this pass because it explicitly says it was inspired by Jason Hise and it publishes the deformation mathematics.

Stack:

- Python
- NumPy
- `clifford`
- Mayavi
- Traits / TraitsUI

The program models the six-ribbon antitwister using conformal geometric algebra, `Cl(4,1)`, and motor interpolation.

The useful decomposition in `Spinor_Cube_Ver2.2.2.py` is:

- `mlog(M)`: logarithm of a conformal motor using a Chasles-theorem decomposition;
- `generate_weights()`: three normalized interpolation weights;
- `generate_rotors(string_axis, rotation_axis)`: constructs the center, intermediate, and fixed-end control motors;
- `interpolate(...)`: exponentiates the weighted sum of motor logarithms;
- `string_points(...)`: samples the interpolated motor along a ribbon and transforms its two boundaries;
- `cube_faces(lam)`: rotates the cube;
- UI state `lam ∈ [-1,1]`: scrubbed parameter for the double-turn motion.

The README gives the construction explicitly. If (R) is the central rotation and (S) establishes ribbon orientation, three roto-translation motors (M_0,M_1,M_2) are blended through

[
M(lambda,alpha)
=
expleft(sum_i B_i(alpha)log M_i(lambda)ight).
]

This is an excellent candidate for a mathematically controlled first deformation model because it gives us an actual continuous ribbon field rather than cloth simulation.

Limit: its interaction is parameter scrubbing, not free grabbing/manipulation.

## parallax1s — `belt-trick`

Repository:

https://github.com/parallax1s/belt-trick

No explicit license file was present at repository root in this pass. Treat it as a **read-only reference unless licensing is clarified**.

Stack / structure:

- Three.js 0.160
- browser ES modules
- Node's built-in test runner
- explicit browser tests
- separate `geometry`, `math`, `render`, presentation/state modules

Interesting files:

- `src/math/spinor-field.js`
- `src/geometry/ribbon-data.js`
- `src/render/field-glsl.js`
- `src/render/ribbon-material.js`
- `src/scene-contract.js`
- `src/spin-sync-state.js`

The page labels itself “Belt trick — topology observatory” and treats 0°, 360°, and 720° as explicit semantic states. It visualizes a core spinor lift from (q=+1) to (q=-1) at 360° and back to (q=+1) at 720°.

The CPU reference field in `spinor-field.js` is worth reading. It includes:

- quaternion multiplication and vector rotation;
- axis-angle spinors;
- several radial twist profiles;
- core grip / anchor fade functions;
- a field deformation `deformPoint(point, options)`;
- “fixed”, “spinning”, and “naive” homotopy modes;
- matching CPU/GPU field contracts.

The ribbon geometry itself is pre-tessellated into sections and deformed by the field. This separation is attractive for GPU work: topology/mesh layout can remain mostly static while the deformation field changes.

Limit: the visible interaction is primarily story/scrubber/playback/camera inspection. That leaves room for our direct-grab interaction.

## Greg Egan — Dirac applet and technical notes

Applet:

https://www.gregegan.net/APPLETS/21/21.html

Technical notes:

https://www.gregegan.net/APPLETS/21/DiracNotes.html

Egan gives a particularly clean mathematical interpretation:

- a belt configuration defines a path through (SO(3));
- for a fixed end orientation there are two path homotopy classes;
- even and odd twists occupy different classes;
- the universal cover is the double cover (SU(2));
- a (2π) rotation changes the sign of a spinor while a (4π) rotation returns it.

This is valuable for invariants and explanatory overlays. It should not be treated as source code for reuse; Egan's page is copyright, all rights reserved.

## GeoGebra spinning-cube / belt-trick construction

https://www.geogebra.org/m/jksua6fg

This interactive construction explicitly says it was inspired by Jason Hise. The public worksheet exposes formulas for a spatially varying rotation of ribbon curves, including a radial/logistic twist falloff.

It is useful as a compact independent check that the phenomenon does not require a general cloth solver: a deterministic field applied to sampled curves is enough for a convincing interactive object.

Check GeoGebra/work licensing before copying worksheet code literally.

## Holroyd et al. — spinor linkage

Paper:

https://arxiv.org/abs/2107.01681

HTML rendering:

https://ar5iv.labs.arxiv.org/html/2107.01681

The paper develops a mechanical implementation of the plate trick and explicitly discusses Hise's six-belt animation. It is useful for distinguishing:

- a topological demonstration;
- a geometrically controlled ribbon deformation;
- a literal mechanical linkage.

For future interaction modes, the mechanical linkage is a useful source of constraints and alternative “handles”, even if the first software toy remains purely geometric.

## Francis / Kauffman / Sandin / Hart — Air On The Dirac Strings

Reference page:

https://www.math.uci.edu/~vmm/Surface/dirac-belt/DiracBelt.html

The page describes the 1993 animation `Air On The Dirac Strings`, designed by George Francis, Louis Kauffman, and Daniel Sandin, with computer graphics by Chris Hartman and John Hart.

It collects the Dirac belt trick, Philippine wine-glass / plate trick, and orientation entanglement in the same visual lineage. Keep it as historical and presentation reference.

## Tarek Sherif — `tesseract-explorer`

Repository:

https://github.com/tsherif/tesseract-explorer

License: MIT.

This is not a belt-trick implementation, but it is a useful browser-native 4D interaction reference:

- WebGL 2 rendering;
- mouse camera controls;
- interactive rotation in each of the six coordinate planes in 4D;
- perspective and orthographic 4D→3D projection;
- unfolding, scaling, autorotation, cell visibility.

It is also a useful modern replacement/reference for the old Hise Tesseract Explorer trail when we need known-source browser code.

## Older antitwister lead

A historical program is still referenced at:

http://Antitwister.ariwatch.com

Contemporary discussion describes it as predating Hise's animations and as generating antitwister movies interactively from settings. This pass did not verify source availability or license. Keep as a recovery lead only.

## What to steal conceptually, not literally

The converging architecture across the best references is:

```
interaction state
      |
      v
spinor / rotor / motor field
      |
      v
sampled ribbon or surface geometry
      |
      v
ordinary 3D renderer
```

For the first implementation, the most promising combination is:

- Leidel for a published, licensed six-ribbon deformation construction;
- Hise for visual target and interaction intent;
- `parallax1s/belt-trick` for CPU/GPU field separation and test ideas;
- Sherif for browser-native 4D control patterns;
- Egan for the mathematical invariants we should expose and test.
