// tasmimpute.cpp: impute missing points in a vector of Shapes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
// Return shape with the missing points filled in.
// Missing points are marked as 0,0 in shape.  We impute the
// position of these missing points using the shape model
// defined by meanshape, eigvecs, etc.

static Shape ImputeMissingPointsForOneShape(
    const Shape& shape,          // in
    const Shape& aligned_shape,  // in: shape aligned to ref shape
    const Shape& meanshape,      // in: n x 2
    const VEC&   eigvals,        // in: neigs x 1
    const MAT&   eigvecs,        // in: 2n x neigs
    const MAT&   eigvecsi,       // in: neigs x 2n, inverse of eigvecs
    double       bmax,           // in
    int          max_iters,      // in
    double       max_dist,       // in
    double       max_dist_delta) // in
{
    // static for efficiency (init once)
    static const VEC pointweights(PointWeights());

    VEC b(NSIZE(eigvals), 1, 0.); // eigvec weights, init to 0
    Shape pinnedpoints(aligned_shape.clone());
    Shape outshape(aligned_shape.clone());
    double dist = FLT_MAX, LastDist = 0;

    int iter;
    for (iter = 0;
            dist > max_dist &&
            ABS(dist - LastDist) > max_dist_delta &&
            iter < max_iters;
         iter++)
    {
        LastDist = dist;
        // call ConformShapeToMod to fill in the missing points
        outshape = ConformShapeToMod(b,
                        outshape, meanshape, eigvals, eigvecs, eigvecsi,
                        bmax, pointweights);
        dist = ForcePinnedPoints(outshape, pinnedpoints);
    }
    static int printed;
    PrintOnce(printed, "%d imputation iters...\n", iter);
    if (iter >= max_iters)
        Err("ImputeMissingPointsForOneShape reached max_iters %d", max_iters);
    return TransformShape(outshape, AlignmentMat(outshape, shape));
}

void ImputeMissingPoints(            // impute missing points in a vector of Shapes
    vec_Shape&       shapes,         // io: update so all shapes have all points
    const vec_Shape& aligned_shapes, // in: shapes aligned to refshape
    const Shape&     meanshape,      // in: n x 2 where n is npoints in shape
    const VEC&       eigvals,        // in: 2n x 1
    const MAT&       eigvecs)        // in: 2n x 2n, inverse of eigs of cov mat
{
    // constants below are somewhat arb
    // maxmin formula for IMPUTE_NEIGS works out to 20 eigs for 77 point shapes
    const int    IMPUTE_NEIGS = MAX(1, MIN(20, NSIZE(eigvals) / 3)); // consts are arb
    const double IMPUTE_BMAX = 1.8;
    const int    IMPUTE_MAX_ITERS = 50;
    const double IMPUTE_MAX_DIST = 0.5;
    const double IMPUTE_MAX_DIST_DELTA = 0.01;

    // keep only IMPUTE_NEIGS
    const MAT eigvals1(DimKeep(eigvals, IMPUTE_NEIGS, 1));
    const MAT eigvecs1(DimKeep(eigvecs, eigvecs.rows, IMPUTE_NEIGS));
    const MAT eigvecsi(eigvecs1.t()); // take transpose to get inverse

    lprintf("Imputing missing points\n");
    for (int ishape = 0; ishape < NSIZE(shapes); ishape++)
        for (int ipoint = 0; ipoint < shapes[ishape].rows; ipoint++)
            if (!PointUsed(shapes[ishape], ipoint)) // point missing?
            {
                shapes[ishape] = ImputeMissingPointsForOneShape(
                                      shapes[ishape], aligned_shapes[ishape],
                                      meanshape, eigvals1, eigvecs1, eigvecsi,
                                      IMPUTE_BMAX, IMPUTE_MAX_ITERS,
                                      IMPUTE_MAX_DIST, IMPUTE_MAX_DIST_DELTA);

                break; // move on to the next shape
            }
}

} // namespace stasm
