#pragma once

#include "vec3.h"
#include "triangle.h"
#include "stdio.h"
#include <vector>
#undef min
#undef max
#include <algorithm> // std::swap
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#include "windows.h"

#ifdef __clang__
#define fscanf_s fscanf
#endif

#define OBJ_LOADER_DEBUG true

scene_object **readObj(const char* filename, material *mat, size_t *numTris, bool flip_triangles = true, float scale = 1.0f, const vec3& translate = { 0, 0, 0 }) {
    std::vector<vec3> verts;
    std::vector<vec3> norms;
    std::vector<scene_object*> objs;

    FILE* f;
    fopen_s(&f, filename, "r");

    if (f) {
        
        float x, y, z;
        vec3 a, b, c;
        vec3 an, bn, cn;

        while (!feof(f)) {

            char s = getc(f);

            if (s == '#') { // comment
                char buf[64] = { 0 };
                do {
                    fgets(buf, sizeof(buf), f);
                } while (buf[strlen(buf) - 1] != '\n' && !feof(f)); // read until newline
            }
            else if (s == '\n' || s == ' ' || s == '\t') {
                // continue
            }
            else if (s == 'v') {

                s = getc(f);

                if (s == ' ' || s == '\t') { // vertex

                    if (fscanf_s(f, " %f %f %f ", &x, &y, &z) == 3) {
                        verts.push_back(vec3(x, y, z));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) DebugBreak();
                        OutputDebugStringA("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
                else if (s == 'n') { // vertex normal

                    if (fscanf_s(f, " %f %f %f ", &x, &y, &z) == 3) {
                        norms.push_back(vec3(x, y, z));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) DebugBreak();
                        OutputDebugStringA("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
            }
            else if (s == 'f') { // face
                
                if (norms.empty()) { // format: f 1 2 3
                    int ai, bi, ci;

                    if (fscanf_s(f, " %i %i %i ", &ai, &bi, &ci) == 3) {

                        if (flip_triangles) {
                            std::swap(ai, ci);
                        }

                        a = verts[ai - 1] * scale;
                        b = verts[bi - 1] * scale;
                        c = verts[ci - 1] * scale;

                        a += translate;
                        b += translate;
                        c += translate;

                            
                        objs.push_back(new triangle(a,b,c,mat));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) DebugBreak();
                        OutputDebugStringA("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
                else { // format : f1//4 2//5 3//6

                    int ai, bi, ci;
                    int ani, bni, cni;

                    if (fscanf_s(f, " %i//%i %i//%i %i//%i ", &ai, &ani, &bi, &bni, &ci, &cni) == 6) {

                        if (flip_triangles) {
                            std::swap(ai, ci);
                            std::swap(ani, cni);
                        }
                        a = verts[ai - 1] * scale;
                        b = verts[bi - 1] * scale;
                        c = verts[ci - 1] * scale;
                        an = norms[ani - 1];
                        bn = norms[bni - 1];
                        cn = norms[cni - 1];

                        a += translate;
                        b += translate;
                        c += translate;


                        objs.push_back(new triangle(a, b, c, an, bn, cn, mat));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) DebugBreak();
                        OutputDebugStringA("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
            }
            else {
                char buf[64] = { 0 };
                do {
                    fgets(buf, sizeof(buf), f);
                } while (buf[strlen(buf) - 1] != '\n' && !feof(f)); // read until newline

                //if (OBJ_LOADER_DEBUG) DebugBreak();
                OutputDebugStringA("Warning: unknown obj format! May slow down or crash the program!\n");
            }
        }
        fclose(f);

        *numTris = objs.size();
        scene_object **list = new scene_object*[objs.size()];
        for (size_t i = 0; i < objs.size(); i++)
        {
            list[i] = objs[i];
        }
        return list;
    }
    else {
        *numTris = 0;
        return nullptr;
    }
}