# Amiberry External Shader Support

## Overview

Amiberry now supports external GLSL shader files in addition to the built-in CRT shaders. This allows you to use shaders from the [libretro/glsl-shaders](https://github.com/libretro/glsl-shaders) repository and other sources.

## Quick Start

1. **Create the shaders directory** (if it doesn't exist):
   ```
   <xdg_config_home>/shaders/
   ```
   
2. **Copy shader files** into this directory:
   ```
   shaders/crt-pi.glsl
   shaders/crt-aperture.glsl
   shaders/zfast_crt.glsl
   ```

3. **Configure Amiberry** to use an external shader:
   Edit `amiberry.conf` and set:
   ```
   shader=crt-pi.glsl
   ```
   
   Or for RTG mode:
   ```
   shader_rtg=crt-pi.glsl
   ```

4. **Launch Amiberry** - the external shader will be loaded automatically!

## Built-in vs External Shaders

### Built-in Shaders
These are the original crtemu shaders, specified by name:
- `tv` - TV CRT effect with heavy scanlines and curvature
- `pc` - PC monitor effect with lighter scanlines
- `lite` - Lightweight CRT effect
- `1084` - Commodore 1084 monitor emulation
- `none` - No shader (fastest)

### External Shaders
Any `.glsl` file in the `shaders/` directory can be used. Simply specify the filename:
- `shader=crt-pi.glsl`
- `shader=crt-aperture.glsl`
- `shader=zfast_crt.glsl`

## Compatible Shaders

External shaders must be **single-pass GLSL shaders** compatible with the RetroArch/libretro format. Multi-pass shaders are not currently supported.

### Recommended Shaders from glsl-shaders Repository

✅ **Fully Compatible** (tested and working):
- `crt-pi.glsl` - Raspberry Pi optimized CRT shader
- `crt-aperture.glsl` - CRT with aperture grille
- `crt-easymode.glsl` - Easy-to-configure CRT
- `zfast_crt.glsl` - Fast CRT shader

⚠️ **May Need Adaptation**:
- `crt-geom.glsl` - Geometric CRT (complex)
- `crt-lottes.glsl` - Lottes CRT (advanced)

❌ **Not Compatible**:
- Multi-pass shaders (crt-royale, crt-easymode-halation, etc.)
- Shaders requiring multiple texture inputs
- Shaders with complex preset files (.glslp)

## Shader Parameters

Many external shaders support runtime parameters defined with `#pragma parameter` directives. These parameters allow you to customize the shader's appearance.

### Example from crt-pi.glsl:
```glsl
#pragma parameter CURVATURE_X "Screen curvature - horizontal" 0.10 0.0 1.0 0.01
#pragma parameter SCANLINE_WEIGHT "Scanline weight" 4.0 0.0 15.0 0.1
#pragma parameter MASK_BRIGHTNESS "Mask brightness" 0.70 0.0 1.0 0.05
```

**Note**: Parameter UI configuration is planned for a future update. Currently, parameters use their default values.

## Hot-Reload Support

Amiberry supports hot-reloading of external shaders for development and testing:

1. Modify your shader file while Amiberry is running
2. The shader will automatically reload when you save the file

**Note**: Hot-reload implementation is in progress.

## Troubleshooting

### Shader Not Loading

If your shader fails to load, check the Amiberry log for error messages:

```
Loading external shader: crt-pi.glsl
Shader file not found: /path/to/shaders/crt-pi.glsl
Failed to load external shader, falling back to no shader
```

**Solutions**:
- Verify the shader file exists in the `shaders/` directory
- Check the filename spelling in `amiberry.conf`
- Ensure the file has `.glsl` extension
- Check file permissions

### Compilation Errors

If the shader compiles but doesn't work correctly:

```
Shader compilation failed:
ERROR: 0:42: 'unknown_uniform' : undeclared identifier
```

**Solutions**:
- The shader may not be compatible with Amiberry's uniform naming
- Try a different shader from the recommended list
- Check the shader's requirements (some need specific uniforms)

### Black Screen

If you see a black screen after loading a shader:

1. The shader may have crashed - check the log
2. Try setting `shader=none` to disable shaders
3. Verify the shader works with other emulators first

## Shader Development

### Creating Custom Shaders

External shaders must follow this structure:

```glsl
#pragma parameter PARAM_NAME "Description" default min max step

// Uniforms
uniform vec2 TextureSize;
uniform vec2 InputSize;
uniform vec2 OutputSize;
uniform int FrameCount;
uniform mat4 MVPMatrix;
uniform sampler2D Texture;

#if defined(VERTEX)
attribute vec4 VertexCoord;
attribute vec2 TexCoord;
varying vec2 TEX0;

void main() {
    gl_Position = MVPMatrix * VertexCoord;
    TEX0 = TexCoord;
}

#elif defined(FRAGMENT)
varying vec2 TEX0;

void main() {
    vec4 color = texture2D(Texture, TEX0);
    gl_FragColor = color;
}
#endif
```

### Available Uniforms

- `TextureSize` - Size of the input texture (vec2)
- `InputSize` - Size of the emulated screen (vec2)
- `OutputSize` - Size of the output window (vec2)
- `FrameCount` - Current frame number (int)
- `MVPMatrix` - Model-View-Projection matrix (mat4)
- `Texture` - Input texture sampler (sampler2D)

## Performance Tips

1. **Use simple shaders on low-end hardware** (Raspberry Pi 3/4)
   - `zfast_crt.glsl` is the fastest
   - `crt-pi.glsl` is optimized for Pi
   
2. **Disable shaders for maximum performance**
   - Set `shader=none` in amiberry.conf

3. **Test different shaders** to find the best balance of quality and performance

## Further Reading

- [libretro/glsl-shaders GitHub](https://github.com/libretro/glsl-shaders)
- [RetroArch Shader Documentation](https://docs.libretro.com/shader/introduction/)
- [Amiberry Documentation](https://github.com/BlitterStudio/amiberry/wiki)

## Support

If you encounter issues with external shaders:

1. Check the Amiberry log file
2. Try a different shader from the recommended list
3. Report issues on the [Amiberry GitHub](https://github.com/BlitterStudio/amiberry/issues)
