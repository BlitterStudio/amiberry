#version 450
//
// Commodore 1084S CRT shader — Vulkan single-pass
//
// Targets visual accuracy to the real Commodore 1084S monitor:
//   - 14" shadow mask CRT (0.42mm dot pitch, NOT Trinitron/aperture grille)
//   - P22 phosphor set with warm color temperature
//   - 15kHz horizontal scan (PAL 312.5 lines / NTSC 262.5 lines)
//   - Noticeable barrel distortion (curved glass tube)
//   - Edge convergence drift typical of consumer monitors
//   - Subtle analog signal noise and phosphor persistence
//

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D backbuffer;

layout(push_constant) uniform PushConstants {
    vec4 cropUV;       // u_min, v_min, u_max, v_max
    vec2 resolution;   // output resolution in pixels
    vec2 size;         // source texture size in pixels
    float time;        // time in seconds
} pc;

// ---------- helpers ----------

// Sample with gamma decode (sRGB → linear). The 1084S's P22 phosphors
// have a gamma of roughly 2.2.
vec3 tsample(vec2 tc)
{
    return pow(max(texture(backbuffer, tc).rgb, vec3(0.0)), vec3(2.2));
}

// Cheap 5-tap blur approximation for bloom/halation without a separate pass.
// Samples a small cross pattern around the center to simulate phosphor glow
// bleeding into neighboring pixels, as seen on the real 1084S especially in
// bright areas (white text on blue background, etc).
vec3 cheap_bloom(vec2 tc, vec2 texel_size)
{
    vec3 center = tsample(tc);
    float r = 2.0; // blur radius in texels
    vec3 bloom = tsample(tc + vec2( r, 0.0) * texel_size)
               + tsample(tc + vec2(-r, 0.0) * texel_size)
               + tsample(tc + vec2(0.0,  r) * texel_size)
               + tsample(tc + vec2(0.0, -r) * texel_size);
    bloom *= 0.25;
    // Mix bloom intensity based on brightness — bright pixels bleed more
    float luma = dot(center, vec3(0.299, 0.587, 0.114));
    return mix(center, bloom, smoothstep(0.15, 0.8, luma) * 0.15);
}

// Attempt at the Hejl/Burgess filmic tone map (close to ACES fit)
vec3 filmic(vec3 c)
{
    vec3 x = max(vec3(0.0), c - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

// Barrel distortion — the 1084S had a noticeably curved 14" tube,
// more curvature than a late-model flat Trinitron.
vec2 barrel(vec2 uv)
{
    uv = (uv - 0.5) * 2.0;
    uv *= 1.06;                                    // slight zoom to hide edge clipping
    uv.x *= 1.0 + pow(abs(uv.y) / 5.5, 2.0);    // horizontal barrel
    uv.y *= 1.0 + pow(abs(uv.x) / 5.0, 2.0);    // vertical barrel
    uv = uv * 0.5 + 0.5;
    uv = uv * 0.92 + 0.04;                        // safe-area inset
    return uv;
}

// Pseudo-random hash
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// ---------- main ----------

void main()
{
    // Map UV through crop region
    vec2 uv = mix(pc.cropUV.xy, pc.cropUV.zw, inUV);

    // --- Barrel distortion ---
    // Mix 65% curved / 35% flat to match the 1084S's moderate curvature
    vec2 curved_uv = mix(barrel(uv), uv, 0.35);

    // Kill pixels outside the visible phosphor area
    if (curved_uv.x < 0.0 || curved_uv.x > 1.0 ||
        curved_uv.y < 0.0 || curved_uv.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // --- Edge convergence error ---
    // Consumer CRTs like the 1084S had slight RGB mis-convergence
    // that increased toward the edges. This is a key visual signature.
    vec2 center_dist = curved_uv - 0.5;
    float edge = dot(center_dist, center_dist) * 4.0;
    float cs = 0.0005;   // convergence spread (pixels)

    vec2 texel = vec2(1.0) / pc.size;

    // --- Horizontal instability ---
    // Very subtle — the 1084S had a good sync circuit but still showed
    // minor jitter at high contrast transitions.
    float jitter = sin(0.1 * pc.time + curved_uv.y * 13.0)
                 * sin(0.23 * pc.time + curved_uv.y * 19.0)
                 * 0.0003;

    // --- Sample with chromatic aberration ---
    vec2 rc = vec2(jitter + curved_uv.x + cs * (1.0 + 2.0 * edge), curved_uv.y + cs * 0.7 * edge);
    vec2 gc = vec2(jitter + curved_uv.x, curved_uv.y);
    vec2 bc = vec2(jitter + curved_uv.x - cs * (1.2 + 1.5 * edge), curved_uv.y + cs * 0.5 * edge);

    vec3 col;
    col.r = cheap_bloom(rc, texel).r;
    col.g = cheap_bloom(gc, texel).g;
    col.b = cheap_bloom(bc, texel).b;

    // Add a small base brightness (CRT black level is never truly 0)
    col += 0.015;

    // --- P22 phosphor color temperature ---
    // The 1084S's P22 phosphor set produced a warm, slightly amber-tinted image.
    // Red and green phosphors were efficient; blue was weaker than modern LCDs.
    col *= vec3(1.05, 1.00, 0.86);

    // --- Contrast / gamma curve ---
    // CRT contrast is non-linear — approximation of the 1084S's response.
    // The pow5 term adds punch to highlights without crushing shadows.
    col = col * 1.1 + 0.55 * col * col + 0.8 * col * col * col * col * col;
    col = clamp(col, vec3(0.0), vec3(10.0));

    // --- Vignette ---
    // Corner darkening from the electron beam having to travel further.
    // The 1084S showed moderate vignette, more than a pro monitor.
    float vig = 16.0 * curved_uv.x * curved_uv.y * (1.0 - curved_uv.x) * (1.0 - curved_uv.y);
    vig = pow(vig, 0.35) * 1.15;
    vig = clamp(vig, 0.0, 1.0);
    col *= vig;

    // --- Scanlines ---
    // The 1084S at 15kHz PAL showed ~256 visible scanlines.
    // Amiga line-doubled modes (400-512 texels) represent 200-256 lines.
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    float effective_lines = pc.size.y;
    if (effective_lines > 300.0) effective_lines *= 0.5; // line-doubled

    // Compute how many output pixels each scanline occupies.
    // When scanlines are too fine to resolve (small window), fade them out
    // gracefully — just as a real CRT's scanlines become invisible at higher
    // resolutions or smaller screen sizes.
    float pixels_per_line = pc.resolution.y / effective_lines;
    float scanline_opacity = smoothstep(1.5, 4.0, pixels_per_line);

    float scanline = 0.5 + 0.5 * cos(curved_uv.y * effective_lines * 3.14159);
    float scan_str = pow(scanline, 0.8);
    // Reduce scanline depth in bright areas (phosphor glow fills gap)
    float brightness_mask = smoothstep(0.0, 0.6, luma) * 0.7;
    scan_str = mix(scan_str, 1.0, brightness_mask);
    // Fade scanlines based on display density
    scan_str = mix(1.0, scan_str, scanline_opacity * 0.85);
    col *= scan_str;

    // --- Shadow mask (dot trio) ---
    // The 1084S used a shadow mask with triangular dot trios, not an
    // aperture grille. The dot pitch was 0.42mm. At typical viewing
    // distances the pattern is barely visible but still affects color purity.
    // We simulate this with a repeating 3x3 pattern modulated by position.
    vec2 mask_pos = gl_FragCoord.xy;
    int mx = int(mod(mask_pos.x, 3.0));
    int my = int(mod(mask_pos.y, 3.0));
    vec3 mask;
    // Staggered dot trio pattern — alternating rows offset by 1.5 pixels
    if (my == 1) mx = int(mod(mask_pos.x + 1.5, 3.0));
    if (mx == 0)      mask = vec3(1.0,  0.72, 0.72);
    else if (mx == 1) mask = vec3(0.72, 1.0,  0.72);
    else              mask = vec3(0.72, 0.72, 1.0);
    // Fade shadow mask at low display density (same logic as scanlines)
    mask = mix(vec3(1.0), mask, scanline_opacity);
    col *= mask;

    // --- Tone mapping ---
    col = filmic(col);

    // --- Analog noise ---
    // The 1084S connected via composite or RGB showed very little noise,
    // but the analog signal path still introduced subtle grain.
    vec2 noise_seed = curved_uv * pc.resolution.xy + pc.time;
    col -= 0.006 * vec3(hash(noise_seed), hash(noise_seed + 1.0), hash(noise_seed + 2.0));

    // --- Subtle brightness flicker ---
    // Power supply ripple caused barely perceptible brightness variation.
    col *= 1.0 - 0.002 * sin(100.0 * pc.time);

    outColor = vec4(max(col, vec3(0.0)), 1.0);
}
