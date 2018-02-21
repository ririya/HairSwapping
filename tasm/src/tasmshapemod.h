// tasmshapemod.h: generate the shape model
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmshapemod_hpp
#define stasm_tasmshapemod_hpp

namespace stasm
{
void CopyShapeVec(
    vec_Shape&       newshapes,  // out
    const vec_Shape& oldshapes); // in

int NbrUsedPoints(       // return the number of used points in shape
    const Shape& shape); // in

void GenAndWriteShapeMod(             // generate the shape model and write it to a .mh file
    ShapeFile&  sh,                   // io: shapes_ field modified if there are missing points
                                      //     entire sh changed if TASM_SUBSET_DESC_MODS
    const char* outdir,               // in: output directory (-d flag)
    const char* modname,              // in: e.g. "yaw00"
    const char* cmdline,              // in: command line used to invoke tasm
    bool        force_facedet,        // in: facedet the images themselves (ignore shapefile facedets)
    bool        write_detpars,        // in: write the detpars to facedet.shape
    bool        write_imputed_shapes, // in: write imputed.shapes file if shapes are imputed
    bool        write_imgs);          // in: generate images for checking the LANDMARK_INFO_TAB

} // namespace stasm
#endif // stasm_tasmshapemod_hpp
