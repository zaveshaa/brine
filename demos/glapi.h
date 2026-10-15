#ifndef BRINE_GLAPI_H
#define BRINE_GLAPI_H

#include <stddef.h>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef unsigned char GLboolean;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_TRIANGLES 0x0004
#define GL_FLOAT 0x1406
#define GL_DEPTH_TEST 0x0B71
#define GL_LESS 0x0201
#define GL_LEQUAL 0x0203
#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_VERSION 0x1F02
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RED 0x1903
#define GL_RG 0x8227
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_RGBA16F 0x881A
#define GL_RGBA32F 0x8814
#define GL_R32F 0x822E
#define GL_RG32F 0x8230
#define GL_ONE 0x0001
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_NONE 0

#define BRINE_GL_FUNCTIONS                                                     \
    X(void, glClear, (GLbitfield mask))                                        \
    X(void, glClearColor, (GLclampf r, GLclampf g, GLclampf b, GLclampf a))    \
    X(void, glEnable, (GLenum cap))                                            \
    X(void, glDisable, (GLenum cap))                                           \
    X(void, glBlendFunc, (GLenum sfactor, GLenum dfactor))                     \
    X(void, glDepthFunc, (GLenum func))                                        \
    X(void, glGenTextures, (GLsizei n, GLuint *textures))                      \
    X(void, glBindTexture, (GLenum target, GLuint texture))                    \
    X(void, glDeleteTextures, (GLsizei n, const GLuint *textures))             \
    X(void, glTexImage2D,                                                      \
      (GLenum target, GLint level, GLint internalformat, GLsizei width,        \
       GLsizei height, GLint border, GLenum format, GLenum type,               \
       const void *pixels))                                                    \
    X(void, glTexParameteri, (GLenum target, GLenum pname, GLint param))       \
    X(void, glActiveTexture, (GLenum texture))                                 \
    X(void, glGenFramebuffers, (GLsizei n, GLuint *framebuffers))              \
    X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer))            \
    X(void, glDeleteFramebuffers, (GLsizei n, const GLuint *framebuffers))     \
    X(void, glFramebufferTexture2D,                                            \
      (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,     \
       GLint level))                                                           \
    X(GLenum, glCheckFramebufferStatus, (GLenum target))                       \
    X(void, glDrawBuffers, (GLsizei n, const GLenum *bufs))                    \
    X(void, glUniform1i, (GLint location, GLint v0))                           \
    X(void, glReadPixels,                                                      \
      (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,         \
       GLenum type, void *pixels))                                             \
    X(void, glUniform2f, (GLint location, GLfloat v0, GLfloat v1))             \
    X(void, glUniform3f,                                                       \
      (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))                    \
    X(void, glUniform1fv, (GLint location, GLsizei count, const GLfloat *v))   \
    X(void, glViewport, (GLint x, GLint y, GLsizei w, GLsizei h))             \
    X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count))           \
    X(void, glGenVertexArrays, (GLsizei n, GLuint *arrays))                    \
    X(void, glBindVertexArray, (GLuint array))                                 \
    X(void, glDeleteVertexArrays, (GLsizei n, const GLuint *arrays))           \
    X(void, glGenBuffers, (GLsizei n, GLuint *buffers))                        \
    X(void, glBindBuffer, (GLenum target, GLuint buffer))                      \
    X(void, glBufferData,                                                      \
      (GLenum target, GLsizeiptr size, const void *data, GLenum usage))        \
    X(void, glBufferSubData,                                                   \
      (GLenum target, GLintptr offset, GLsizeiptr size, const void *data))     \
    X(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers))               \
    X(void, glEnableVertexAttribArray, (GLuint index))                         \
    X(void, glVertexAttribPointer,                                             \
      (GLuint index, GLint size, GLenum type, GLboolean normalized,            \
       GLsizei stride, const void *pointer))                                   \
    X(GLuint, glCreateShader, (GLenum type))                                   \
    X(void, glShaderSource,                                                    \
      (GLuint shader, GLsizei count, const GLchar *const *string,              \
       const GLint *length))                                                   \
    X(void, glCompileShader, (GLuint shader))                                  \
    X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint *params))       \
    X(void, glGetShaderInfoLog,                                                \
      (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog))      \
    X(void, glDeleteShader, (GLuint shader))                                   \
    X(GLuint, glCreateProgram, (void))                                         \
    X(void, glAttachShader, (GLuint program, GLuint shader))                   \
    X(void, glLinkProgram, (GLuint program))                                   \
    X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params))     \
    X(void, glGetProgramInfoLog,                                               \
      (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog))     \
    X(void, glUseProgram, (GLuint program))                                    \
    X(void, glDeleteProgram, (GLuint program))                                 \
    X(GLint, glGetUniformLocation, (GLuint program, const GLchar *name))       \
    X(void, glUniformMatrix4fv,                                                \
      (GLint location, GLsizei count, GLboolean transpose,                     \
       const GLfloat *value))                                                  \
    X(void, glUniform1f, (GLint location, GLfloat v0))                         \
    X(const unsigned char *, glGetString, (GLenum name))

#define X(ret, name, params)                                                   \
    typedef ret (*PFN_##name) params;                                          \
    extern PFN_##name name;
BRINE_GL_FUNCTIONS
#undef X

#endif
