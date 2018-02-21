// tasmconf.cpp: configuration routines for tasm
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
bool IsTasmHatDesc( // true if the given point uses a HAT descriptor (not a classical descriptor)
    int ilev,       // in: pyramid level (0 is full scale)
    int ipoint)     // in
{
    CV_Assert(ipoint >= 0 && ipoint < NELEMS(LANDMARK_INFO_TAB));
    return ilev <= HAT_START_LEV && (LANDMARK_INFO_TAB[ipoint].bits & AT_Hat) != 0;
}

bool FaceDetFalsePos(     // true if given detpar is wrong (off the shape)
    const DetPar& detpar, // in
    const Shape&  shape)  // in
{
    // boundaries of the detected face rectangle
    const double left  = detpar.x - .5 * detpar.width;
    const double top   = detpar.y - .5 * detpar.height;
    const double right = detpar.x + .5 * detpar.width;
    const double bot   = detpar.y + .5 * detpar.height;

    Shape shape17(Shape17OrEmpty(shape));
    if (shape17.rows == 0)  // could not convert the shape to a shape17?
    {
        // a bit of a hack: check that the face rect covers at least half the points
        int nvalid = 0;
        for (int ipoint = 0; ipoint < shape.rows; ipoint++)
            if (InRect(shape(ipoint, IX), shape(ipoint, IY), left, top, right, bot))
                nvalid++;
        return nvalid < shape.rows / 2;
    }
    // Is any point outside the face box box?  We do however allow the eyebrows
    // and bottom of the mouth to be outside the box, since this if fairly
    // common.
    for (int ipoint = 0; ipoint < shape17.rows; ipoint++)
    {
        if (ipoint != L17_LEyebrowOuter &&
            ipoint != L17_LEyebrowInner &&
            ipoint != L17_REyebrowInner &&
            ipoint != L17_REyebrowOuter &&
            ipoint != L17_CBotOfBotLip  &&
            !InRect(shape17(ipoint, IX), shape17(ipoint, IY), left, top, right, bot))
        {
            return true;    // note return, it's a false positive
        }
    }
#if 0 // TODO
    bool bad = false;
    // is eye mouth distance too small? (i.e. face box is too big, this
    // sometimes happens if facedet thinks sunglasses are eyes).
    double eyemouth = EyeMouthDist(shape17);
    pgPrintf("EyeMouth/detpar.height %7.3f ", EyeMouth/detpar.height);
    if (eyemouth / detpar.height < .3)
    {
        // logprintf("Offending EyeMouth %4.2f ", eyemouth / detpar.height);
        return true;    // note return, it's a false positive because eyemouth dist too small
    }
#endif
    return false; // not a false positive
}

bool PointUsableForTraining( // true if a point is not obscured or otherwise bad
                             // and always true if no tagbits are set
    const Shape& shape,      // in
    int          ipoint,     // in
    unsigned     tagbits,    // in: tagbits from tag field in shape file
    const char*  base)       // in
{
    const unsigned info_bits = LANDMARK_INFO_TAB[ipoint].bits;

    CV_Assert(NELEMS(LANDMARK_INFO_TAB) == shape.rows);
    CV_Assert(ipoint >= 0 && ipoint < shape.rows);

    // ignore all points in an image tagged as a bad image or if the image is cropped
    const unsigned bad = tagbits & (AT_BadImg|AT_Cropped);
    if (bad)
    {
        static int printed;
        PrintOnce(printed,
            "\n    ignoring bad or cropped face in %s (tag 0x%x)... ",
            base, tagbits);
    }
    // ignore if the point is not manually landmarked (indicated as 0,0 in the shapefile)
    const bool used = PointUsed(shape, ipoint);
    if (!used)
    {
        static int printed;
        PrintOnce(printed,
            "\n    point %d in %s is missing...\n", ipoint, base);
    }
    // ignore if it's an eye point and is tagged as bad (e.g. because the eye is closed)
    const unsigned badeye = (info_bits & AT_Eye) && (tagbits & AT_BadEye);
    if (badeye)
    {
        static int printed;
        PrintOnce(printed,
            "\n    ignoring eye in %s (tag 0x%x)... ", base, tagbits);
    }
    // ignore if the point is obscured by facial hair
    const unsigned obscured_hair = tagbits & info_bits & (AT_Beard|AT_Mustache);
    if (obscured_hair)
    {
        static int printed;
        PrintOnce(printed,
            "\n    point %d in %s is obscured by facial hair (tag 0x%x)... ",
            ipoint, base, tagbits);
    }
    // ignore if the point is obscured by glasses
    const unsigned obscured_glasses = tagbits & info_bits & AT_Glasses;
    if (obscured_glasses)
    {
        static int printed;
        PrintOnce(printed,
            "\n    point %d in %s is obscured by glasses (tag 0x%x)... ",
            ipoint, base, tagbits);
    }
    return used && !bad && !badeye && !obscured_hair && !obscured_glasses;
}

} // namespace stasm
