

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "brine.h"
#include "glapi.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define XMIN (-0.9f)
#define XMAX (0.9f)
#define YMIN (0.0f)
#define YMAX (0.5f)
#define ZMIN (-0.35f)
#define ZMAX (0.35f)

#define FAR_DEPTH 1.0e6f
#define SPLAT 0.85f

#define X(ret, name, params) PFN_##name name;
BRINE_GL_FUNCTIONS
#undef X

typedef struct {
    float yaw;
    float pitch;
    float dist;
    double lx;
    double ly;
    int drag;
    int paused;
} Camera;

typedef struct {
    GLuint fbo;
    GLuint color;
    GLuint depth;
} Target;

static const char *VS_DEPTH =
    "#version 330 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProj;\n"
    "uniform float uRadius;\n"
    "uniform float uPxPerUnit;\n"
    "out vec3 vEye;\n"
    "out float vHalf;\n"
    "void main(){\n"
    "  vec4 eye = uView * vec4(aPos, 1.0);\n"
    "  vEye = eye.xyz;\n"
    "  gl_Position = uProj * eye;\n"
    "  float dist = max(-eye.z, 0.001);\n"
    "  float ps = clamp(uPxPerUnit * uRadius / dist, 1.0, 128.0);\n"
    "  gl_PointSize = ps;\n"
    "  vHalf = 0.5 * ps * dist / uPxPerUnit;\n"
    "}\n";

static const char *FS_DEPTH =
    "#version 330 core\n"
    "in vec3 vEye;\n"
    "in float vHalf;\n"
    "uniform mat4 uProj;\n"
    "out vec4 frag;\n"
    "void main(){\n"
    "  vec2 ndc = gl_PointCoord * 2.0 - 1.0;\n"
    "  float d2 = dot(ndc, ndc);\n"
    "  if (d2 > 1.0) discard;\n"
    "  float R = vHalf;\n"
    "  vec2 off = ndc * R;\n"
    "  float rr = R * R - dot(off, off);\n"
    "  if (rr < 0.0) discard;\n"
    "  float z = vEye.z + sqrt(rr);\n"
    "  float clipz = uProj[2][2] * z + uProj[3][2];\n"
    "  gl_FragDepth = 0.5 * (clipz / (-z)) + 0.5;\n"
    "  frag = vec4(-z, 0.0, 0.0, 1.0);\n"
    "}\n";

static const char *VS_QUAD =
    "#version 330 core\n"
    "out vec2 vUV;\n"
    "void main(){\n"
    "  vec2 uv = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
    "  vUV = uv;\n"
    "  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char *FS_BLUR =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 frag;\n"
    "uniform sampler2D uTex;\n"
    "uniform vec2 uStep;\n"
    "uniform vec2 uRes;\n"
    "uniform float uSigmaS;\n"
    "uniform float uSigmaR;\n"
    "void main(){\n"
    "  float center = texture(uTex, vUV).r;\n"
    "  bool cvalid = center < 1.0e5;\n"
    "  float sum = cvalid ? center : 0.0;\n"
    "  float wsum = cvalid ? 1.0 : 0.0;\n"
    "  float wtot = cvalid ? 1.0 : 0.0;\n"
    "  for (int i = 1; i <= 16; i++) {\n"
    "    float fi = float(i);\n"
    "    vec2 o = uStep * fi;\n"
    "    vec2 px = o * uRes;\n"
    "    float ws = exp(-dot(px, px) / (2.0 * uSigmaS * uSigmaS));\n"
    "    float a = texture(uTex, vUV + o).r;\n"
    "    float b = texture(uTex, vUV - o).r;\n"
    "    float da = a - center;\n"
    "    float db = b - center;\n"
    "    bool va = a < 1.0e5;\n"
    "    bool vb = b < 1.0e5;\n"
    "    float wa;\n"
    "    float wb;\n"
    "    wtot += 2.0 * ws;\n"
    "    if (cvalid) {\n"
    "      wa = va ? ws * exp(-da * da / (2.0 * uSigmaR * uSigmaR)) : 0.0;\n"
    "      wb = vb ? ws * exp(-db * db / (2.0 * uSigmaR * uSigmaR)) : 0.0;\n"
    "    } else {\n"
    "      wa = va ? ws : 0.0;\n"
    "      wb = vb ? ws : 0.0;\n"
    "    }\n"
    "    sum += a * wa + b * wb;\n"
    "    wsum += wa + wb;\n"
    "  }\n"
    "  if (!cvalid && wsum < 0.7 * wtot) {\n"
    "    frag = vec4(1.0e6, 0.0, 0.0, 1.0);\n"
    "    return;\n"
    "  }\n"
    "  frag = vec4(sum / wsum, 0.0, 0.0, 1.0);\n"
    "}\n";

static const char *FS_COMP_A =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 frag;\n"
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uNarrow;\n"
    "uniform mat4 uProj;\n"
    "uniform mat4 uInvView;\n"
    "uniform vec3 uCamPos;\n"
    "uniform vec2 uInvRes;\n"
    "uniform vec3 uSunDir;\n"
    "uniform float uTime;\n"
    "uniform vec3 uBmin;\n"
    "uniform vec3 uBmax;\n"
    "vec3 skyColor(vec3 d){\n"
    "  float y = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);\n"
    "  vec3 horizon = vec3(0.78, 0.85, 0.95);\n"
    "  vec3 zenith  = vec3(0.16, 0.34, 0.72);\n"
    "  vec3 c = mix(horizon, zenith, pow(y, 0.85));\n"
    "  float s = max(dot(d, normalize(uSunDir)), 0.0);\n"
    "  c += vec3(1.0, 0.95, 0.82) * pow(s, 400.0) * 2.0;\n"
    "  c += vec3(1.0, 0.93, 0.78) * pow(s, 8.0) * 0.10;\n"
    "  return c;\n"
    "}\n"
    "vec3 tubShade(vec3 p, int face){\n"
    "  if (face == 0) {\n"
    "    vec2 g = abs(fract(p.xz * 2.0) - 0.5);\n"
    "    float line = smoothstep(0.46, 0.5, max(g.x, g.y));\n"
    "    return mix(vec3(0.56, 0.66, 0.73), vec3(0.80, 0.88, 0.93), line);\n"
    "  }\n"
    "  float hgt = clamp((p.y - uBmin.y) / max(uBmax.y - uBmin.y, 1.0e-3), 0.0, 1.0);\n"
    "  vec3 col = mix(vec3(0.46, 0.56, 0.63), vec3(0.82, 0.90, 0.95), hgt);\n"
    "  col += vec3(0.20, 0.24, 0.28) * smoothstep(0.84, 1.0, hgt);\n"
    "  return col;\n"
    "}\n"
    "vec3 sceneColor(vec3 o, vec3 d){\n"
    "  float tb = 1.0e30;\n"
    "  int face = -1;\n"
    "  if (d.y < -1.0e-6) {\n"
    "    float t = (uBmin.y - o.y) / d.y;\n"
    "    if (t > 0.0) { vec3 p = o + d * t;\n"
    "      if (p.x >= uBmin.x && p.x <= uBmax.x && p.z >= uBmin.z && p.z <= uBmax.z) { tb = t; face = 0; } }\n"
    "  }\n"
    "  if (d.x < -1.0e-6) { float t = (uBmin.x - o.x) / d.x;\n"
    "    if (t > 0.0) { vec3 p = o + d * t;\n"
    "      if (p.y >= uBmin.y && p.y <= uBmax.y && p.z >= uBmin.z && p.z <= uBmax.z && t < tb) { tb = t; face = 1; } } }\n"
    "  if (d.x > 1.0e-6) { float t = (uBmax.x - o.x) / d.x;\n"
    "    if (t > 0.0) { vec3 p = o + d * t;\n"
    "      if (p.y >= uBmin.y && p.y <= uBmax.y && p.z >= uBmin.z && p.z <= uBmax.z && t < tb) { tb = t; face = 2; } } }\n"
    "  if (d.z < -1.0e-6) { float t = (uBmin.z - o.z) / d.z;\n"
    "    if (t > 0.0) { vec3 p = o + d * t;\n"
    "      if (p.y >= uBmin.y && p.y <= uBmax.y && p.x >= uBmin.x && p.x <= uBmax.x && t < tb) { tb = t; face = 3; } } }\n"
    "  if (d.z > 1.0e-6) { float t = (uBmax.z - o.z) / d.z;\n"
    "    if (t > 0.0) { vec3 p = o + d * t;\n"
    "      if (p.y >= uBmin.y && p.y <= uBmax.y && p.x >= uBmin.x && p.x <= uBmax.x && t < tb) { tb = t; face = 4; } } }\n"
    "  if (face >= 0) {\n"
    "    vec3 p = o + d * tb;\n"
    "    return mix(vec3(0.82, 0.88, 0.94), tubShade(p, face), exp(-tb * 0.22));\n"
    "  }\n"
    "  if (d.y < -1.0e-6) {\n"
    "    float t = (uBmin.y - o.y) / d.y;\n"
    "    if (t > 0.0) {\n"
    "      vec3 hp = o + d * t;\n"
    "      vec2 g = abs(fract(hp.xz * 1.5) - 0.5);\n"
    "      float line = smoothstep(0.47, 0.5, max(g.x, g.y));\n"
    "      vec3 base = mix(vec3(0.36, 0.45, 0.50), vec3(0.48, 0.58, 0.63), line);\n"
    "      return mix(vec3(0.82, 0.88, 0.94), base, exp(-t * 0.12));\n"
    "    }\n"
    "  }\n"
    "  return skyColor(d);\n"
    "}\n";

static const char *FS_COMP_B =
    "vec3 eyePos(vec2 uv, float depth){\n"
    "  vec2 ndc = uv * 2.0 - 1.0;\n"
    "  float z = -depth;\n"
    "  vec3 eye = vec3(ndc.x * (-z) / uProj[0][0], ndc.y * (-z) / uProj[1][1], z);\n"
    "  return (uInvView * vec4(eye, 1.0)).xyz;\n"
    "}\n"
    "void main(){\n"
    "  float d = texture(uDepth, vUV).r;\n"
    "  if (d > 1.0e5) {\n"
    "    vec2 ndc = vUV * 2.0 - 1.0;\n"
    "    vec3 eye = vec3(ndc.x / uProj[0][0], ndc.y / uProj[1][1], -1.0);\n"
    "    vec3 dir = normalize((uInvView * vec4(eye, 0.0)).xyz);\n"
    "    frag = vec4(sceneColor(uCamPos, dir), 1.0);\n"
    "    return;\n"
    "  }\n"
    "  vec3 P = eyePos(vUV, d);\n"
    "  vec3 Px = eyePos(vUV + vec2(uInvRes.x, 0.0), texture(uDepth, vUV + vec2(uInvRes.x, 0.0)).r);\n"
    "  vec3 Py = eyePos(vUV + vec2(0.0, uInvRes.y), texture(uDepth, vUV + vec2(0.0, uInvRes.y)).r);\n"
    "  vec3 n = normalize(cross(Px - P, Py - P));\n"
    "  vec3 Nx = eyePos(vUV + vec2(uInvRes.x, 0.0), texture(uNarrow, vUV + vec2(uInvRes.x, 0.0)).r);\n"
    "  vec3 Ny = eyePos(vUV + vec2(0.0, uInvRes.y), texture(uNarrow, vUV + vec2(0.0, uInvRes.y)).r);\n"
    "  vec3 nd = n;\n"
    "  float wx = P.x * 34.0 + uTime * 2.1;\n"
    "  float wz = P.z * 31.0 - uTime * 1.7;\n"
    "  float wq = (P.x + P.z) * 19.0 + uTime * 1.3;\n"
    "  float dwdx = 34.0 * cos(wx) * cos(wz) * 0.5 + 19.0 * cos(wq) * 0.5;\n"
    "  float dwdz = -31.0 * sin(wx) * sin(wz) * 0.5 + 19.0 * cos(wq) * 0.5;\n"
    "  vec3 rp = vec3(-dwdx, 0.0, -dwdz);\n"
    "  n = normalize(n + rp * 0.006);\n"
    "  nd = normalize(nd + rp * 0.014);\n"
    "  vec3 V = normalize(uCamPos - P);\n"
    "  if (dot(n, V) < 0.0) n = -n;\n"
    "  if (dot(nd, V) < 0.0) nd = -nd;\n"
    "  vec3 R = reflect(-V, n);\n"
    "  vec3 T = refract(-V, n, 0.7519);\n"
    "  vec3 refl = sceneColor(P, R);\n"
    "  vec3 refr = sceneColor(P, T);\n"
    "  refr = mix(refr, refr * vec3(0.62, 0.90, 1.0), 0.38);\n"
    "  refr *= exp(-vec3(0.28, 0.09, 0.035) * (d * 0.15));\n"
    "  float fres = 0.05 + 0.95 * pow(1.0 - clamp(dot(V, n), 0.0, 1.0), 3.0);\n"
    "  vec3 col = mix(refr, refl, fres);\n"
    "  float sp2 = pow(max(dot(R, normalize(uSunDir)), 0.0), 40.0);\n"
    "  col += vec3(0.90, 0.95, 1.0) * sp2 * 0.10;\n"
    "  float dn = texture(uNarrow, vUV).r;\n"
    "  float foam = clamp((dn - d) * 22.0, 0.0, 0.4);\n"
    "  col = mix(col, vec3(0.95, 0.97, 1.0), foam * 0.75);\n"
    "  frag = vec4(col, 1.0);\n"
    "}\n";

static const char *gl_load(void)
{
#define X(ret, name, params)                                                   \
    name = (PFN_##name)glfwGetProcAddress(#name);                              \
    if (name == NULL) {                                                        \
        return #name;                                                          \
    }
    BRINE_GL_FUNCTIONS
#undef X
    return NULL;
}

static GLuint compile(GLenum type, const char *const *parts, int count)
{
    GLuint sh = glCreateShader(type);
    GLint ok = 0;
    glShaderSource(sh, count, parts, NULL);
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(sh, (GLsizei)sizeof log, NULL, log);
        fprintf(stderr, "brine: shader error: %s\n", log);
    }
    return sh;
}

static GLuint program(const char *vs, const char *const *fs, int nfs)
{
    GLuint v = compile(GL_VERTEX_SHADER, &vs, 1);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs, nfs);
    GLuint p = glCreateProgram();
    GLint ok = 0;
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, (GLsizei)sizeof log, NULL, log);
        fprintf(stderr, "brine: link error: %s\n", log);
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

static void perspective(float m[16], float fovy, float aspect, float zn, float zf)
{
    float f = 1.0f / tanf(fovy * 0.5f);
    int i;
    for (i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.0f;
    m[14] = (2.0f * zf * zn) / (zn - zf);
}

static void camera(float view[16], float inv[16], const float eye[3],
                   const float ctr[3])
{
    float up[3] = {0.0f, 1.0f, 0.0f};
    float f[3] = {ctr[0] - eye[0], ctr[1] - eye[1], ctr[2] - eye[2]};
    float s[3];
    float u[3];
    float fl;

    fl = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    f[0] /= fl;
    f[1] /= fl;
    f[2] /= fl;
    s[0] = f[1] * up[2] - f[2] * up[1];
    s[1] = f[2] * up[0] - f[0] * up[2];
    s[2] = f[0] * up[1] - f[1] * up[0];
    fl = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] /= fl;
    s[1] /= fl;
    s[2] /= fl;
    u[0] = s[1] * f[2] - s[2] * f[1];
    u[1] = s[2] * f[0] - s[0] * f[2];
    u[2] = s[0] * f[1] - s[1] * f[0];

    view[0] = s[0];
    view[4] = s[1];
    view[8] = s[2];
    view[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
    view[1] = u[0];
    view[5] = u[1];
    view[9] = u[2];
    view[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
    view[2] = -f[0];
    view[6] = -f[1];
    view[10] = -f[2];
    view[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
    view[3] = 0.0f;
    view[7] = 0.0f;
    view[11] = 0.0f;
    view[15] = 1.0f;

    inv[0] = s[0];
    inv[1] = s[1];
    inv[2] = s[2];
    inv[3] = 0.0f;
    inv[4] = u[0];
    inv[5] = u[1];
    inv[6] = u[2];
    inv[7] = 0.0f;
    inv[8] = -f[0];
    inv[9] = -f[1];
    inv[10] = -f[2];
    inv[11] = 0.0f;
    inv[12] = eye[0];
    inv[13] = eye[1];
    inv[14] = eye[2];
    inv[15] = 1.0f;
}

static int make_target(Target *t, int w, int h, int with_depth)
{
    glGenTextures(1, &t->color);
    glBindTexture(GL_TEXTURE_2D, t->color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, (GLsizei)w, (GLsizei)h, 0, GL_RED,
                 GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &t->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           t->color, 0);
    t->depth = 0;
    if (with_depth) {
        glGenTextures(1, &t->depth);
        glBindTexture(GL_TEXTURE_2D, t->depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)w,
                     (GLsizei)h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, t->depth, 0);
    }
    {
        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return (st == GL_FRAMEBUFFER_COMPLETE) ? 0 : -1;
    }
}

static void free_target(Target *t)
{
    glDeleteTextures(1, &t->color);
    if (t->depth != 0) {
        glDeleteTextures(1, &t->depth);
    }
    glDeleteFramebuffers(1, &t->fbo);
}

static void mouse_button(GLFWwindow *w, int button, int action, int mods)
{
    Camera *cam = (Camera *)glfwGetWindowUserPointer(w);
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            cam->drag = 1;
            glfwGetCursorPos(w, &cam->lx, &cam->ly);
        } else if (action == GLFW_RELEASE) {
            cam->drag = 0;
        }
    }
}

static void cursor_pos(GLFWwindow *w, double x, double y)
{
    Camera *cam = (Camera *)glfwGetWindowUserPointer(w);
    if (cam->drag) {
        cam->yaw -= (float)(x - cam->lx) * 0.008f;
        cam->pitch += (float)(y - cam->ly) * 0.008f;
        if (cam->pitch > 1.45f) {
            cam->pitch = 1.45f;
        }
        if (cam->pitch < -1.45f) {
            cam->pitch = -1.45f;
        }
    }
    cam->lx = x;
    cam->ly = y;
}

static void scroll(GLFWwindow *w, double dx, double dy)
{
    Camera *cam = (Camera *)glfwGetWindowUserPointer(w);
    (void)dx;
    cam->dist = (float)((double)cam->dist * (1.0 - dy * 0.1));
    if (cam->dist < 0.8f) {
        cam->dist = 0.8f;
    }
    if (cam->dist > 9.0f) {
        cam->dist = 9.0f;
    }
}

static void key(GLFWwindow *w, int k, int sc, int action, int mods)
{
    Camera *cam = (Camera *)glfwGetWindowUserPointer(w);
    (void)sc;
    (void)mods;
    if (action != GLFW_PRESS) {
        return;
    }
    if (k == GLFW_KEY_SPACE) {
        cam->paused = !cam->paused;
    } else if (k == GLFW_KEY_R) {
        cam->yaw = 0.45f;
        cam->pitch = 0.62f;
        cam->dist = 2.4f;
    }
}

static void dump_ppm(const char *path, int w, int h)
{
    unsigned char *buf = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    FILE *f;
    int y;
    if (buf == NULL) {
        return;
    }
    glReadPixels(0, 0, (GLsizei)w, (GLsizei)h, GL_RGB, GL_UNSIGNED_BYTE, buf);
    f = fopen(path, "wb");
    if (f != NULL) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (y = h - 1; y >= 0; y--) {
            fwrite(buf + (size_t)y * (size_t)w * 3, 1, (size_t)w * 3, f);
        }
        fclose(f);
        printf("brine: wrote %s (%dx%d)\n", path, w, h);
    }
    free(buf);
}


int main(int argc, char **argv)
{
    BrineParams p = brine_default_params();
    Brine *b;
    GLFWwindow *win;
    const char *dump = getenv("BRINE_DUMP");
    const char *dumpf = getenv("BRINE_DUMP_FRAME");
    long dump_at = (dumpf != NULL) ? atol(dumpf) : 240;
    long total = 0;
    Camera cam = {0.45f, 0.62f, 2.4f, 0.0, 0.0, 0, 0};
    float spacing = (argc > 1) ? (float)atof(argv[1]) : 0.055f;
    float lo[3] = {-0.85f, 0.03f, -0.30f};
    float hi[3] = {0.85f, 0.28f, 0.30f};
    const float sun[3] = {-0.5f, 0.75f, 0.4f};
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint prog_depth;
    GLuint prog_blur;
    GLuint prog_comp;
    Target depthT = {0, 0, 0};
    Target blurA = {0, 0, 0};
    Target blurB = {0, 0, 0};
    Target wideT = {0, 0, 0};
    float *vertex;
    size_t cap;
    double prev;
    double acc = 0.0;
    double dt = 1.0 / 120.0;
    double fps_t = 0.0;
    int frames = 0;
    int fw = 0;
    int fh = 0;
    const char *missing;

    p.bounds_min[0] = XMIN;
    p.bounds_max[0] = XMAX;
    p.bounds_min[1] = YMIN;
    p.bounds_max[1] = YMAX;
    p.bounds_min[2] = ZMIN;
    p.bounds_max[2] = ZMAX;
    p.iterations = 6;
    p.viscosity = 0.02f;
    p.damping = 0.005f;
    p.surface_tension = 0.00f;
    p.h = spacing * 1.5f;

    b = brine_new(&p);
    if (b == NULL) {
        fprintf(stderr, "brine: out of memory\n");
        return 1;
    }
    brine_emit_box(b, lo, hi, spacing);
    cap = brine_count(b);

    if (!glfwInit()) {
        fprintf(stderr, "brine: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    win = glfwCreateWindow(1200, 760, "brine", NULL, NULL);
    if (win == NULL) {
        fprintf(stderr, "brine: could not create a GL 3.3 window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(win, &cam);
    glfwSetMouseButtonCallback(win, mouse_button);
    glfwSetCursorPosCallback(win, cursor_pos);
    glfwSetScrollCallback(win, scroll);
    glfwSetKeyCallback(win, key);

    missing = gl_load();
    if (missing != NULL) {
        fprintf(stderr, "brine: missing GL entry point %s\n", missing);
        glfwTerminate();
        return 1;
    }

    prog_depth = program(VS_DEPTH, &FS_DEPTH, 1);
    prog_blur = program(VS_QUAD, &FS_BLUR, 1);
    {
        const char *comp[2];
        comp[0] = FS_COMP_A;
        comp[1] = FS_COMP_B;
        prog_comp = program(VS_QUAD, comp, 2);
    }

    vertex = (float *)malloc(cap * 3 * sizeof(float) + 16);
    if (vertex == NULL) {
        fprintf(stderr, "brine: out of memory\n");
        return 1;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cap * 3 * sizeof(float)), NULL,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)(3 * sizeof(float)),
                          (const void *)0);

    printf("brine %s: %zu particles, GL %s\n", brine_version(), cap,
           (const char *)glGetString(GL_VERSION));

    prev = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        double frame = now - prev;
        const float *pos;
        size_t i;
        int wpx;
        int hpx;
        float proj[16];
        float view[16];
        float inv[16];
        float eye[3];
        float ctr[3];
        float aspect;
        float fovy = 0.85f;

        prev = now;
        if (frame > 0.05) {
            frame = 0.05;
        }
        acc += frame;

        if (!cam.paused) {
            int sub = 0;
            while (acc >= dt && sub < 8) {
                brine_step(b, (float)dt);
                acc -= dt;
                sub++;
            }
            if (acc > 4.0 * dt) {
                acc = 0.0;
            }
        }

        pos = brine_positions(b);
        for (i = 0; i < cap; i++) {
            vertex[i * 3 + 0] = pos[i * 3 + 0];
            vertex[i * 3 + 1] = pos[i * 3 + 1];
            vertex[i * 3 + 2] = pos[i * 3 + 2];
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        (GLsizeiptr)(cap * 3 * sizeof(float)), vertex);

        glfwGetFramebufferSize(win, &wpx, &hpx);
        if (wpx < 1) {
            wpx = 1;
        }
        if (hpx < 1) {
            hpx = 1;
        }
        if (wpx != fw || hpx != fh) {
            fw = wpx;
            fh = hpx;
            if (depthT.fbo != 0) {
                free_target(&depthT);
                free_target(&blurA);
                free_target(&blurB);
                free_target(&wideT);
            }
            if (make_target(&depthT, fw, fh, 1) != 0 ||
                make_target(&blurA, fw, fh, 0) != 0 ||
                make_target(&blurB, fw, fh, 0) != 0 ||
                make_target(&wideT, fw, fh, 0) != 0) {
                fprintf(stderr, "brine: incomplete framebuffer\n");
                return 1;
            }
        }

        aspect = (float)fw / (float)fh;
        eye[0] = cam.dist * cosf(cam.pitch) * sinf(cam.yaw);
        eye[1] = 0.16f + cam.dist * sinf(cam.pitch);
        eye[2] = cam.dist * cosf(cam.pitch) * cosf(cam.yaw);
        ctr[0] = 0.0f;
        ctr[1] = 0.16f;
        ctr[2] = 0.0f;
        perspective(proj, fovy, aspect, 0.05f, 100.0f);
        camera(view, inv, eye, ctr);

        {
            float px_per_unit = (float)fh / (2.0f * tanf(fovy * 0.5f));
            GLint l_view;
            GLint l_proj;
            GLint l_radius;
            GLint l_px;

            glBindFramebuffer(GL_FRAMEBUFFER, depthT.fbo);
            glViewport(0, 0, fw, fh);
            glClearColor(FAR_DEPTH, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_BLEND);
            glUseProgram(prog_depth);
            l_view = glGetUniformLocation(prog_depth, "uView");
            l_proj = glGetUniformLocation(prog_depth, "uProj");
            l_radius = glGetUniformLocation(prog_depth, "uRadius");
            l_px = glGetUniformLocation(prog_depth, "uPxPerUnit");
            glUniformMatrix4fv(l_view, 1, GL_FALSE, view);
            glUniformMatrix4fv(l_proj, 1, GL_FALSE, proj);
            glUniform1f(l_radius, spacing * SPLAT);
            glUniform1f(l_px, px_per_unit);
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, (GLsizei)cap);
        }

        {
            GLint l_tex = glGetUniformLocation(prog_blur, "uTex");
            GLint l_step = glGetUniformLocation(prog_blur, "uStep");
            GLint l_res = glGetUniformLocation(prog_blur, "uRes");
            GLint l_ss = glGetUniformLocation(prog_blur, "uSigmaS");
            GLint l_sr = glGetUniformLocation(prog_blur, "uSigmaR");
            float ppu = (float)fh / (2.0f * tanf(fovy * 0.5f));
            float rad_px = spacing * SPLAT * ppu;
            float ss;
            float steppx;

            glDisable(GL_DEPTH_TEST);
            glUseProgram(prog_blur);
            glUniform1i(l_tex, 0);
            glUniform2f(l_res, (float)fw, (float)fh);

            ss = rad_px * 1.0f;
            steppx = 3.0f * ss / 16.0f;
            glBindFramebuffer(GL_FRAMEBUFFER, blurA.fbo);
            glViewport(0, 0, fw, fh);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthT.color);
            glUniform1f(l_ss, ss);
            glUniform1f(l_sr, spacing * 3.0f);
            glUniform2f(l_step, steppx / (float)fw, 0.0f);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindFramebuffer(GL_FRAMEBUFFER, blurB.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurA.color);
            glUniform2f(l_step, 0.0f, steppx / (float)fh);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            ss = rad_px * 2.5f;
            steppx = 3.0f * ss / 16.0f;
            glBindFramebuffer(GL_FRAMEBUFFER, blurA.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurB.color);
            glUniform1f(l_ss, ss);
            glUniform1f(l_sr, spacing * 8.0f);
            glUniform2f(l_step, steppx / (float)fw, 0.0f);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindFramebuffer(GL_FRAMEBUFFER, blurB.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurA.color);
            glUniform2f(l_step, 0.0f, steppx / (float)fh);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            ss = rad_px * 4.0f;
            steppx = 3.0f * ss / 16.0f;
            glBindFramebuffer(GL_FRAMEBUFFER, blurA.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurB.color);
            glUniform1f(l_ss, ss);
            glUniform1f(l_sr, spacing * 10.0f);
            glUniform2f(l_step, steppx / (float)fw, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindFramebuffer(GL_FRAMEBUFFER, blurB.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurA.color);
            glUniform2f(l_step, 0.0f, steppx / (float)fh);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindFramebuffer(GL_FRAMEBUFFER, wideT.fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurB.color);
            glUniform2f(l_step, 0.0f, steppx / (float)fh);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        {
            GLint l_depth = glGetUniformLocation(prog_comp, "uDepth");
            GLint l_narrow = glGetUniformLocation(prog_comp, "uNarrow");
            GLint l_proj = glGetUniformLocation(prog_comp, "uProj");
            GLint l_inv = glGetUniformLocation(prog_comp, "uInvView");
            GLint l_cam = glGetUniformLocation(prog_comp, "uCamPos");
            GLint l_res = glGetUniformLocation(prog_comp, "uInvRes");
            GLint l_sun = glGetUniformLocation(prog_comp, "uSunDir");
            GLint l_time = glGetUniformLocation(prog_comp, "uTime");
            GLint l_bmin = glGetUniformLocation(prog_comp, "uBmin");
            GLint l_bmax = glGetUniformLocation(prog_comp, "uBmax");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fw, fh);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glUseProgram(prog_comp);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, wideT.color);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, blurB.color);
            glUniform1i(l_depth, 0);
            glUniform1i(l_narrow, 1);
            glUniformMatrix4fv(l_proj, 1, GL_FALSE, proj);
            glUniformMatrix4fv(l_inv, 1, GL_FALSE, inv);
            glUniform3f(l_cam, eye[0], eye[1], eye[2]);
            glUniform2f(l_res, 1.0f / (float)fw, 1.0f / (float)fh);
            glUniform3f(l_sun, sun[0], sun[1], sun[2]);
            glUniform1f(l_time, (float)now);
            glUniform3f(l_bmin, XMIN, YMIN, ZMIN);
            glUniform3f(l_bmax, XMAX, YMAX, ZMAX);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }

        glfwSwapBuffers(win);
        glfwPollEvents();

        total++;
        if (dump != NULL && total >= dump_at) {
            const float *pp = brine_positions(b);
            float xmn = 1e9f, xmx = -1e9f, ymn = 1e9f, ymx = -1e9f;
            float zmn = 1e9f, zmx = -1e9f;
            size_t k;
            for (k = 0; k < cap; k++) {
                float x = pp[k * 3], y = pp[k * 3 + 1], z = pp[k * 3 + 2];
                if (x < xmn) { xmn = x; }
                if (x > xmx) { xmx = x; }
                if (y < ymn) { ymn = y; }
                if (y > ymx) { ymx = y; }
                if (z < zmn) { zmn = z; }
                if (z > zmx) { zmx = z; }
            }
            fprintf(stderr, "brine: h=%.3f spacing=%.3f bounds x[%.2f,%.2f] y[%.2f,%.2f] z[%.2f,%.2f]\n",
                    (double)p.h, (double)spacing,
                    (double)xmn, (double)xmx, (double)ymn, (double)ymx,
                    (double)zmn, (double)zmx);
            dump_ppm(dump, fw, fh);
            break;
        }

        frames++;
        if (now - fps_t > 0.5) {
            char title[128];
            int f = (int)(frames / (now - fps_t));
            snprintf(title, sizeof title, "brine: %zu particles, %d fps",
                     cap, f);
            glfwSetWindowTitle(win, title);
            fps_t = now;
            frames = 0;
        }
    }

    if (depthT.fbo != 0) {
        free_target(&depthT);
        free_target(&blurA);
        free_target(&blurB);
        free_target(&wideT);
    }
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog_depth);
    glDeleteProgram(prog_blur);
    glDeleteProgram(prog_comp);
    free(vertex);
    brine_free(b);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
