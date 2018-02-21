// tasmhat.cpp: generate HAT descriptor models
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
static void WriteHatMod(    // write a hat model to a .mh file as C++ code
    int         ilev,       // in: pyramid level (0 is full scale)
    int         ipoint,     // in
    int         npoints,    // in
    const VEC&  coeffs,     // in: coeffs[0] is the intercept
    const char* outdir,     // in: output directory (-d flag)
    const char* modname,    // in: e.g. "yaw00"
    const char* cmdline,    // in: command line used to invoke tasm
    bool        write_func) // in: true to write linmod function as well
{
    char format[SLEN]; // e.g. "%s/mh/%s_lev%d_p%2.2d_hat.mh" for npoints 10..99
    const int ndigits = NumDigits(npoints);
    sprintf(format, "%%s/mh/%%s_lev%%d_p%%%d.%dd_hat.mh", ndigits, ndigits);
    char path[SLEN];
    sprintf(path, format, outdir, modname, ilev, ipoint);
    static int printed;
    PrintOnce(printed, "\nGenerating %s...\n", path);
    FILE* file = fopen(path, "wb");
    if (!file)
        Err("Cannot open %s", path);
    Fprintf(file, "// %s.mh: machine generated HAT descriptor matcher\n", Base(path));
    Fprintf(file, "//\n");
    Fprintf(file, "// Command: %s\n\n", cmdline);
    char path1[SLEN];
    sprintf(format, "stasm_%%s_lev%%d_p%%%d.%dd_hat_mh", ndigits, ndigits);
    sprintf(path1, format, modname, ilev, ipoint);
    ToUpper(path1);
    Fprintf(file, "#ifndef %s\n", path1);
    Fprintf(file, "#define %s\n\n", path1);
    Fprintf(file, "namespace stasm {\n\n");
    Fprintf(file, "// static const int EYEMOUTH_DIST = %d;\n", EYEMOUTH_DIST);
    Fprintf(file, "// static const double PYR_RATIO = %g;\n", PYR_RATIO);
    Fprintf(file, "\n");
    if (write_func)
    {
        Fprintf(file, "static double linmod(\n");
        Fprintf(file, "    const double* const x,\n");
        Fprintf(file, "    const double        intercept,\n");
        Fprintf(file, "    const double* const coef,\n");
        Fprintf(file, "    const int           n)\n");
        Fprintf(file, "{\n");
        Fprintf(file, "    double yhat = intercept;\n");
        Fprintf(file, "#if 0                               // unoptimized\n");
        Fprintf(file, "    for (int i = 0; i < n; i++)\n");
        Fprintf(file, "        yhat += coef[i] * x[i];\n");
        Fprintf(file, "#else                               // optimized\n");
        Fprintf(file, "    const int n4 = n - 4;\n");
        Fprintf(file, "    int i = 0;\n");
        Fprintf(file, "    if (n4 > 0)\n");
        Fprintf(file, "    {\n");
        Fprintf(file, "        for (; i < n4; i += 4)\n");
        Fprintf(file, "            yhat += coef[i]   * x[i]   +\n");
        Fprintf(file, "                    coef[i+1] * x[i+1] +\n");
        Fprintf(file, "                    coef[i+2] * x[i+2] +\n");
        Fprintf(file, "                    coef[i+3] * x[i+3];\n");
        Fprintf(file, "    }\n");
        Fprintf(file, "    for (; i < n; i++)\n");
        Fprintf(file, "        yhat += coef[i] * x[i];\n");
        Fprintf(file, "#endif\n");
        Fprintf(file, "    return yhat;\n");
        Fprintf(file, "}\n\n");
    }
    sprintf(format,
            "static double %%s_lev%%d_p%%%d.%dd_hatfit(const double* const d) "
            "// d has %%d elements\n",
            ndigits, ndigits);
    Fprintf(file, format,
            modname, ilev, ipoint, NSIZE(coeffs)-1);
    Fprintf(file, "{\n");
    Fprintf(file, "const double intercept = %g;\n\n", coeffs(0));
    VEC coeffs1(1, NSIZE(coeffs)-1); // coeffs but without the intercept
    for (int i = 0; i < NSIZE(coeffs)-1; i++)
        coeffs1(i) = coeffs(i+1);
    WriteMatAsArray(coeffs1, "coef", "linear regression coefficients", file);
    Fprintf(file, "// negative below because lowest distance is best fit\n");
    Fprintf(file, "return -linmod(d, intercept, coef, %d);\n", NSIZE(coeffs)-1);
    Fprintf(file, "}\n");
    Fprintf(file, "\n");
    sprintf(format,
            "static const HatDescMod %%s_lev%%d_p%%%d.%dd_hat(%%s_lev%%d_p%%%d.%dd_hatfit);\n",
            ndigits, ndigits, ndigits, ndigits);
    Fprintf(file, format,
            modname, ilev, ipoint, modname, ilev, ipoint);
    Fprintf(file, "\n");
    Fprintf(file, "} // namespace stasm\n", path1);
    Fprintf(file, "#endif // %s\n", path1);
    fclose(file);
}

// Example of conversion by ConvertDescs with constants as follows:
//      TASM_NNEG_TRAIN_DESCS = 3
//      TASM_BALANCE_NUMBER_OF_CASES = true
//
// dists 40x1 (there are 3 neg descriptors per pos descriptor because TASM_NNEG_TRAIN_DESCS = 3)
//      0       pos (distance is zero)
//      2.2     neg (distance is non zero)
//      7.2     neg
//      2.2     neg
//      0       pos
//      4.1     neg
//      4.2     neg
//      ...
//
// descs 40x160
//      0.3  0.1  0.0  0.0  0.1  0.2  1.3  1.1 ...    pos case
//      0.9  0.2  0.1  0.0  0.1  0.2  0.7  1.0 ...    neg case
//      0.6  0.4  0.3  0.0  0.1  0.4  0.5  0.6 ...    neg case
//      0.1  0.0  0.0  0.0  0.0  0.4  1.6  1.0 ...    neg case
//      0.2  0.1  0.0  0.0  0.1  0.4  1.2  0.7 ...    pos case
//      0.3  0.1  0.0  0.0  0.0  0.2  1.4  1.1 ...    neg case
//      0.5  0.3  0.0  0.1  0.1  0.1  0.6  0.6 ...    neg case
//      ...
//
// b 60x1 (there are 3 pos descriptors for each set of 3 neg descriptors, balanced)
//      0.0     pos case
//      0.0     pos case (duplicate of above)
//      0.0     pos case (duplicate of above)
//      2.2     neg case
//      7.2     neg case
//      2.2     neg case
//      0.0     pos case
//      0.0     pos case (duplicate of above)
//      0.0     pos case (duplicate of above)
//      4.1     neg case
//      4.2     neg case
//      ...
//
// A 60x161 (first column is intercept)
//      1.0  0.3  0.1  0.0  0.0  0.1  0.2  1.3  1.1    ... pos case
//      1.0  0.3  0.1  0.0  0.0  0.1  0.2  1.3  1.1    ... pos case (duplicate of above)
//      1.0  0.3  0.1  0.0  0.0  0.1  0.2  1.3  1.1    ... pos case (duplicate of above)
//      1.0  0.9  0.2  0.1  0.0  0.1  0.2  0.7  1.0    ... neg case
//      1.0  0.6  0.4  0.3  0.0  0.1  0.4  0.5  0.6    ... neg case
//      1.0  0.1  0.0  0.0  0.0  0.0  0.4  1.6  1.0    ... neg case
//      1.0  0.2  0.1  0.0  0.0  0.1  0.4  1.2  0.7    ... pos case
//      1.0  0.2  0.1  0.0  0.0  0.1  0.4  1.2  0.7    ... pos case (duplicate of above)
//      1.0  0.2  0.1  0.0  0.0  0.1  0.4  1.2  0.7    ... pos case (duplicate of above)
//      1.0  0.3  0.1  0.0  0.0  0.0  0.2  1.4  1.1    ... neg case
//      1.0  0.5  0.3  0.0  0.1  0.1  0.1  0.6  0.6    ... neg case
//      ...

static void ConvertDescs(    // type convert, add intercept, posssibly dup pos cases
    MAT&              A,     // out: nbr of shapes x nbr of desc elems plus intercept
    MAT&              b,     // out: nbr of shapes
    const vec_double& dists, // in: distance of each descriptor from landmark
    const vec_VEC&    descs) // in: nbr of shapes x nbr of desc elems
{
    CV_Assert(NSIZE(descs) == NSIZE(dists));

    // How many rows must we generate?  We need 1 row for each
    // negative case and npos rows for each positive case.

    const int npos = TASM_BALANCE_NUMBER_OF_CASES? TASM_NNEG_TRAIN_DESCS: 1;
    int nnewrows = 0;
    for (int row = 0; row < NSIZE(descs); row++)
        nnewrows += dists[row]? 1: npos;

    const int cols = NSIZE(descs[0]);
    A = MAT(nnewrows, 1+cols); // extra 1 for the intercept column
    b = VEC(nnewrows, 1);
    int newrow = 0;
    for (int idesc = 0; idesc < NSIZE(descs); idesc++)
    {
        for (int idup = 0; idup < (dists[idesc]? 1: npos); idup++)
        {
            A(newrow, 0) = 1; // intercept col
            for (int col = 0; col < cols; col++)
                A(newrow, col+1) = descs[idesc](col);
            b(newrow) = dists[idesc]? (TASM_BINARY_DISTANCE? 1: dists[idesc]): 0;
            newrow++;
        }
    }
}

void GenHatMod( // generate the HAT descriptor model for one point at the given pyr level
    int               ilev,    // in: pyramid level (0 is full scale)
    int               ipoint,  // in
    int               npoints, // in
    const vec_double& dists,   // in: [idesc] distance from true landmark of descriptors
    const vec_VEC     descs,   // in: [idesc] HAT descriptors for current point
    const char*       msg,     // in: appended to err msg if any e.g. "(level 1 point 2)"
    const char*       outdir,  // in: output directory (-d flag)
    const char*       modname, // in: e.g. "yaw00"
    const char*       cmdline) // in: command line used to invoke tasm
{
    MAT A; VEC b; ConvertDescs(A, b, dists, descs);
    const VEC coeffs = LinSolve(A, b, msg);
    static bool write_func = true;
    WriteHatMod(ilev, ipoint, npoints, coeffs, outdir, modname, cmdline, write_func);
    write_func = false; // write the function code only once
    CheckMem();
}

} // namespace stasm
