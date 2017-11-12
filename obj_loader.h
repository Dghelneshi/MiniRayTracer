#pragma once

#include "vec3.h"
#include "triangle.h"
#include "mat4.h"

triangle **readObj(const char* filename, material *mat, size_t *numTris, bool flip_triangles = true, const Mat4& scale = Mat4::Identity,
                   const Vec3& translate = { 0, 0, 0 }, const Mat4& rotate = Mat4::Identity);