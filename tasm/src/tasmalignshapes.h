// tasmalignshapes.h: align training shapes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmalignshapes_hpp
#define stasm_tasmalignshapes_hpp

namespace stasm
{
void AlignTrainingShapes(
    Shape&     meanshape,      // io
    vec_Shape& aligned_shapes, // io
    Shape&     refshape);      // in

} // namespace stasm
#endif // stasm_tasmalignshapes_hpp
