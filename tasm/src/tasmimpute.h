// tasmimpute.h: impute missing points in a vector of Shapes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmimpute_hpp
#define stasm_tasmimpute_hpp

namespace stasm
{
void ImputeMissingPoints(            // impute missing points in a vector of Shapes
    vec_Shape&       shapes,         // io: update so all shapes have all points
    const vec_Shape& aligned_shapes, // in: shapes aligned to refshape
    const Shape&     meanshape,      // in: n x 2
    const VEC&       eigvals,        // in: 2n x 1
    const MAT&       eigvecs);       // in: 2n x 2n, inverse of eigs of cov mat

} // namespace stasm
#endif // stasm_tasmimpute_hpp
