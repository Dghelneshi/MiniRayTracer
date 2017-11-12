#include "volumes.h"
#include "pcg.h"
#include "math.h" // log

bool constant_volume::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    // rec1 contains minimum hit and rec2 contains maximum hit (i,e. opposite side of the boundary)
    // if we are inside the volume, rec1 will hit behind us, rec2 will hit in front
    hit_record rec1, rec2;

    if (boundary->hit(r, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), &rec1)) {
        if (boundary->hit(r, rec1.t + 0.0001f, std::numeric_limits<float>::max(), &rec2)) {

            if (rec1.t < tmin)
                rec1.t = tmin;
            if (rec2.t > tmax)
                rec2.t = tmax;
            if (rec1.t >= rec2.t)
                return false;
            if (rec1.t < 0)
                rec1.t = 0;

            float inside_dist = (rec2.t - rec1.t);
            float hit_dist = -(1 / density) * log(randf());

            if (hit_dist < inside_dist) {
                rec->t = rec1.t + hit_dist;
                rec->p = r.eval(rec->t);
                rec->n = Vec3(1, 0, 0); // arbitrary
                rec->mat_ptr = phase_function;
                return true;
            }
        }
    }
    return false;
}