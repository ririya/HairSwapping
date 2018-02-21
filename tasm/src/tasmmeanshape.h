// tasmmeanshape.cpp: generate mean shape in the face detector frame
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmmeanshape_hpp
#define stasm_tasmmeanshape_hpp

namespace stasm
{
void MeanShapeAlignedToFaceDets(
    Shape&           meanshape,      // out
    const ShapeFile& sh,             // in
    const vec_int&   nused_points,   // in
    const char*      outdir,         // in: output directory (-d flag)
    const char*      modname,        // in: e.g. "yaw00"
    const char*      cmdline,        // in: command line used to invoke tasm
    bool             force_facedet,  // in: facedet the images themselves (ignore shapefile facedets)
    bool             write_detpars); // in: write the image detpars to facedet.shape

} // namespace stasm
#endif // stasm_tasmmeanshape_hpp
