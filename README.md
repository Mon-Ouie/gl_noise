This program contains the implementation of a few noise functions in
functions. It fills a 3D buffer with points sampled from those functions and
turns samples with a value above a certain threshold into cubes which it
displays.

Controls
--------

Use the arrow keys to move around and the mouse to rotate the camera around.

Supported options
-----------------

- `--debug`: Enables debug output from the GL implementation
- `--fs`: Enables fullscreen mode.
- `--test`: Just draw a single cube at a fixed location
- `--white`: Generates white noise.
- `--simplex`: Generates 3D simplex noise
- `--perlin4d`: Generatess 4D Perlin noise. The 4th dimension is treated as
  time.
- 3D Perlin noise is used by default.
