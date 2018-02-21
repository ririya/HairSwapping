// tasmalignshapes.cpp: Align training the shapes.
//                      Knows how to deal with shapes with missing points.
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
class RememberUnusedPoints
{
public:
    RememberUnusedPoints(   // constructor
        const Shape& shape) // in
    {
        unused_.resize(shape.rows);
        for (int row = 0; row < shape.rows; row++)
            unused_[row] = !PointUsed(shape, row);
    }

    void Restore_(    // set points marked true in unused_ to 0,0
        Shape& shape) // io
    {
        for (int row = 0; row < shape.rows; row++)
            if (unused_[row])
                shape(row, IX) = shape(row, IY) = 0;
    }
private:
    vec_bool unused_; // points in shape which are 0,0 are marked as true
};

static void GetShapeCentroid( // this excludes unused points
    double&      xmean,       // out
    double&      ymean,       // out
    const Shape& shape)       // in
{
    const int nused = NbrUsedPoints(shape);
    xmean = SumElems(shape.col(IX)) / nused;
    ymean = SumElems(shape.col(IY)) / nused;
}

// Centralize shape so its centroid is 0,0.
// This knows about unused points, and ensures they remain unused
// (i.e. with a value of 0,0) after centralization.

static void CentralizeShape(
    Shape& shape)            // io
{
    RememberUnusedPoints unused(shape); // remember which points are 0,0
    double xmean, ymean;
    GetShapeCentroid(xmean, ymean, shape);
    shape.col(IX) -= xmean;
    shape.col(IY) -= ymean;
    JitterPointsAt00InPlace(shape);
    unused.Restore_(shape); // restore original 0,0 points
}

static void CheckSameDim(
    const MAT&  mat,
    const MAT&  mat1,
    const char* msg)
{
    if (mat.rows != mat1.rows || mat.cols != mat1.cols)
        Err("%s matrices have different sizes %dx%d %dx%d",
            msg, mat.rows, mat.cols, mat1.rows, mat1.cols);
}

// Add shape to newshape on a point by point basis.
// Same as newshape += shape, but with this difference: if a point isn't
// used in shape, we use the corresponding point in the alternative shape
// altshape instead

static void SumShapes(
    Shape&       newshape, // io
    const Shape& shape,    // in
    const Shape& altshape) // in
{
    CheckSameDim(newshape, shape,    "SumShapes");
    CheckSameDim(newshape, altshape, "SumShapes");

    const int npoints = shape.rows;
    for (int ipoint = 0; ipoint < npoints; ipoint++)
    {
        if (PointUsed(shape, ipoint))
        {
            newshape(ipoint, IX) += shape(ipoint, IX);
            newshape(ipoint, IY) += shape(ipoint, IY);
        }
        else
        {
            newshape(ipoint, IX) += altshape(ipoint, IX);
            newshape(ipoint, IY) += altshape(ipoint, IY);
        }
    }
}

static void AlignShapeInPlace(
    Shape&       shape,              // io
    const Shape& anchorshape)        // in
{
    TransformShapeInPlace(shape, AlignmentMat(shape, anchorshape));
}

// This aligns the shapes using the method in CootesTaylor 2004 section 4.2.
// But we differ from that method as follows:
//
//   1. we don't normalize meanshape len to 1 (so can put aligned shapes
//      on top of images directly for debugging visualization)
//
//   2. we don't use tangent spaces
//
//   3. We ignore unused (i.e. 0,0) points, i.e. we can deal with
//      shapes with unused points
//      The passed in refshape must have all its points, though (TODO really?)

static void AlignShapes(
    vec_Shape&   shapes,    // io:  shapes aligned to refshape
    Shape&       meanshape, // out: the mean of the aligned shapes
    const Shape& refshape)  // in
{
    int nshapes = NSIZE(shapes);
    int ishape;
    for (ishape = 0; ishape < nshapes; ishape++)
        CentralizeShape(shapes[ishape]);

    // generate meanshape, the average shape

    static const int MAX_PASS = 30;
    refshape.copyTo(meanshape);
    int pass;
    for (pass = 0; pass < MAX_PASS; pass++)
    {
        Shape new_meanshape(meanshape.rows, meanshape.cols, 0.);
        Shape altshape(meanshape.clone()); // alternate shape: see SumShapes
        for (ishape = 0; ishape < nshapes; ishape++)
        {
            AlignShapeInPlace(shapes[ishape], meanshape);
            SumShapes(new_meanshape, shapes[ishape], altshape);
        }
        // constrain new average shape by aligning it to the ref shape
        AlignShapeInPlace(new_meanshape, refshape);
        const double CONF_MaxShapeAlignDist = 1e-6f;
        // norm below returns the summed L2 dist between elems of the shapes
        if (cv::norm(meanshape, new_meanshape) < CONF_MaxShapeAlignDist)
            break;                          // note break (converged)
        meanshape = new_meanshape;
    }
    if (pass > MAX_PASS)
        Err("Could not align shapes in %d passes", MAX_PASS);

    // align all the shapes to the average shape

    for (ishape = 0; ishape < nshapes; ishape++)
        AlignShapeInPlace(shapes[ishape], meanshape);
}

// Print the angle of the left and rightmost points in shape.
// Also print the angle of the eyes in shape (after aligning shapes, this
// angle should be near zero).

static void ShowShapeAngles(
    const Shape& shape)      // in
{
    // find the leftmost and rightmost points

    double left = FLT_MAX, right = FLT_MIN;
    int ileft = -1, iright = -1;
    for (int ipoint = 0; ipoint < shape.rows; ipoint++)
    {
        if (shape(ipoint, IX) < left)
        {
            left = shape(ipoint, IX);
            ileft = ipoint;
        }
        else if (shape(ipoint, IX) > right)
        {
            right = shape(ipoint, IX);
            iright = ipoint;
        }
    }
    const double angle = -atan2(shape(iright, IY) - shape(ileft, IY),
                                shape(iright, IX) - shape(ileft, IX));

    lprintf("Mean shape outermost points (%d,%d) angle %.3g degrees",
        ileft, iright, RadsToDegrees(angle));

    const double eyeangle = EyeAngle(shape);
    if (Valid(eyeangle))
        lprintf(", eye angle %.3g degrees\n", eyeangle);
    // else
    //     lprintf("Eye angle not available\n");
}

void AlignTrainingShapes(
    Shape&     meanshape,      // out
    vec_Shape& aligned_shapes, // io
    Shape&     refshape)       // in
{
    Shape centered_refshape(refshape.clone());
    CentralizeShape(centered_refshape);
    AlignShapes(aligned_shapes, meanshape, centered_refshape);
    ShowShapeAngles(meanshape);
}

} // namespace stasm
