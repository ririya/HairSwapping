// tasmhat.h: generate HAT descriptor models
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmhat_hpp
#define stasm_tasmhat_hpp

namespace stasm
{
void GenHatMod( // generate the HAT descriptor model for one point at the given pyr level
    int               ilev,     // in: pyramid level (0 is full scale)
    int               ipoint,   // in
    int               npoints,  // in
    const vec_double& dists,    // in: [idesc] distance from true landmark of descriptors
    const vec_VEC     descs,    // in: [idesc] HAT descriptors for current point
    const char*       msg,      // in: appended to err msg if any e.g. "(level 1 point 2)"
    const char*       outdir,   // in: output directory (-d flag)
    const char*       modname,  // in: e.g. "yaw00"
    const char*       cmdline); // in: command line used to invoke tasm

} // namespace stasm
#endif // stasm_tasmhat_hpp
