#include "obj_loader.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <utility> // std::swap
#include "platform.h"

#ifndef _MSC_VER
#define fscanf_s fscanf
#endif

#define OBJ_LOADER_DEBUG true

triangle **readObj(const char* filename, material *mat, size_t *numTris, bool flip_triangles, const Mat4& scale, const Vec3& translate, const Mat4& rotate) {
    std::vector<Vec3> verts;
    std::vector<Vec3> norms;
    std::vector<triangle*> polys;

    Mat4 invRot = Mat4::Invert(rotate);

    FILE* f = fopen(filename, "r");

    if (f) {

        float x, y, z;
        Vec3 a, b, c;
        Vec3 an, bn, cn;

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
                        verts.push_back(Vec3(x, y, z));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) MRT_Assert(false);
                        MRT_DebugPrint("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
                else if (s == 'n') { // vertex normal

                    if (fscanf_s(f, " %f %f %f ", &x, &y, &z) == 3) {
                        norms.push_back(Vec3(x, y, z));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) MRT_Assert(false);
                        MRT_DebugPrint("Warning: unknown obj format! May slow down or crash the program!\n");
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

                        a = scale * verts[ai - 1];
                        b = scale * verts[bi - 1];
                        c = scale * verts[ci - 1];

                        a = rotate * a;
                        b = rotate * b;
                        c = rotate * c;

                        a += translate;
                        b += translate;
                        c += translate;


                        polys.push_back(new triangle(a, b, c, mat));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) MRT_Assert(false);
                        MRT_DebugPrint("Warning: unknown obj format! May slow down or crash the program!\n");
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
                        a = scale * verts[ai - 1];
                        b = scale * verts[bi - 1];
                        c = scale * verts[ci - 1];

                        an = norms[ani - 1];
                        bn = norms[bni - 1];
                        cn = norms[cni - 1];

                        an = an * invRot;
                        bn = bn * invRot;
                        cn = cn * invRot;

                        a = rotate * a;
                        b = rotate * b;
                        c = rotate * c;

                        a += translate;
                        b += translate;
                        c += translate;


                        polys.push_back(new triangle(a, b, c, an, bn, cn, mat));
                    }
                    else {
                        if (OBJ_LOADER_DEBUG) MRT_Assert(false);
                        MRT_DebugPrint("Warning: unknown obj format! May slow down or crash the program!\n");
                        break;
                    }
                }
            }
            else {
                char buf[64] = { 0 };
                do {
                    fgets(buf, sizeof(buf), f);
                } while (buf[strlen(buf) - 1] != '\n' && !feof(f)); // read until newline

                //if (OBJ_LOADER_DEBUG) MRT_Assert(false);
                MRT_DebugPrint("Warning: unknown obj format! May slow down or crash the program!\n");
            }
        }
        fclose(f);

        *numTris = polys.size();
        triangle **list = new triangle*[polys.size()];
        for (size_t i = 0; i < polys.size(); i++)
        {
            list[i] = polys[i];
        }
        return list;
    }
    else {
        *numTris = 0;
        return nullptr;
    }
}