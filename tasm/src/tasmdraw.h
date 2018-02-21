// tasmdraw.h: draw the landmark table, for debugging purposes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef STASM_TASMDRAW_HPP
#define STASM_TASMDRAW_HPP

namespace stasm
{
void TasmDraw( // draw images for checking tasm parameters and landmark table
    const ShapeFile& sh,             // in
    int              irefshape,      // in
    const Shape&     meanshape,      // in
    const char*      outdir);        // in: output directory (-d flag)

} // namespace stasm
#endif // STASM_TASMDRAW_HPP
