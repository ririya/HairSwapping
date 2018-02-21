// tasmclassic.cpp: generate classic 1D descriptor models
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
static bool IeeeNormal( // true if x is a "normal" IEEE number
    double x)
{
#if _MSC_VER // Microsoft Visual C++ 10 doesn't have isnormal()
    int c = _fpclass(x);
              // Negative normal |   -0 |        +0 | Positive normal
    return (c & (_FPCLASS_NN|_FPCLASS_NZ|_FPCLASS_PZ|_FPCLASS_PN)) != 0;
#else
    return std::isnormal(x);
#endif
}

static bool IeeeNormal( // true if all elems of mat are "normal" IEEE numbers
    const MAT& mat)
{
    CV_Assert(mat.isContinuous());
    double* data = Buf(mat);
    for (int i = 0; i < NSIZE(mat); i++)
        if (!IeeeNormal(data[i]))
            return false;
    return true;
}

static bool PosDef( // true if mat is positive definite
    const MAT& mat) // in
{
    CV_Assert(mat.rows == mat.cols && mat.isContinuous());
    MAT temp(mat.clone()); // don't want to destroy mat so make a copy

    // figure out if mat is pos def by doing the first part of a Cholesky decomposition.
    // following code lifted from OpenCV 2.3.1 lapack.cpp::CholImpl
    int n = temp.rows;
    double* data = (double *)temp.data;
    double* L = (double *)temp.data;

    for (int i = 0; i < n; i++)
    {
        double s;
        int j, k;
        for (j = 0; j < i; j++)
        {
            s = data[i*n + j];
            for (k = 0; k < j; k++)
                s -= L[i*n + k]*L[j*n + k];
            L[i*n + j] = s*L[j*n + j];
        }
        s = data[i*n + i];
        for (k = 0; k < j; k++)
        {
            double t = L[i*n + k];
            s -= t*t;
        }
        if (s < std::numeric_limits<double>::epsilon())
            return false; // note: return
        L[i*n + i] = (double)(1./std::sqrt(s));
    }
    return true;
}

static bool CheckCov(   // true if everything ok with cov, else issue message and false
    const MAT&  cov,    // in: the covariance matrix
    const char* prefix, // in: either "" or "inverted"
    const char* msg)    // in: appended to err msg if any e.g. "(level 1 point 2)"
{
    if (!IeeeNormal(cov))
    {
        static int printed;
        PrintOnce(printed,
                  "\n    %scov mat is not IEEE normal, "
                  "replacing with identity mat%s%s...\n",
                  prefix, msg && msg[0]? " ": "", msg);
        return false;
    }
    const double maxabs = MaxAbsElem(cov);
    if (maxabs > FLT_MAX) // FLT_MAX is a convenient big number
    {
        static int printed;
        PrintOnce(printed,
            "\n    an element %g of the %scov mat is too large, "
            "replacing with identity mat%s%s...\n",
            maxabs, prefix, msg && msg[0]? " ": "", msg);
        return false;
    }
    if (!PosDef(cov))
    {
        static int printed;
        PrintOnce(printed,
            "\n    %scov mat is not positive definite, "
            "replacing with identity mat%s%s...\n",
            prefix, msg && msg[0]? " ": "", msg);
        return false;
    }
    return true;
}

static void GetInvertedCovMat( // get the inverted cov mat from the descriptors
    MAT&           cov,        // out: the inverted covariance matrix
    const vec_VEC& descs,      // in: the descriptors
    const VEC&     mean,       // in: the mean of the descriptors
    const char*    msg)        // in: appended to err msg if any e.g. "(level 1 point 2)"
{
    CV_Assert(NSIZE(descs) > 0);
    const int proflen = NSIZE(descs[0]);
    if (NSIZE(descs) <= proflen)
    {
        // not enough data, covar mat would be singular, use ident mat instead
        static int printed;
        PrintOnce(printed,
            "\n    nbr of shapes %d <= "
            "profile length %d, using identity cov mat%s%s...\n",
            NSIZE(descs), proflen,
            msg && msg[0]? " ": "", msg);
        cov = MAT(cv::Mat::eye(proflen, proflen, CV_64F)); // identity matrix
    }
    else
    {
        // calculate covar mat
        for (int i = 0; i < NSIZE(descs); i++)
        {
            const VEC& desc = descs[i] - mean;
            cov += desc.t() * desc;
        }
        cov /= NSIZE(descs);
        bool good = CheckCov(cov, "", msg);
        if (good)
        {
            // Invert cov.  DECOMP_CHOLESKY is faster than DECOMP_LU
            // (and ok to use here because cov is pos def).
            cov = cov.inv(cv::DECOMP_CHOLESKY);
        }

        if (!good || !CheckCov(cov, "inverted ", msg))
        {
            // replace bad inverted cov mat with identity matrix
            cov = MAT(cv::Mat::eye(proflen, proflen, CV_64F)); // identity matrix
        }
    }
}

static void WriteClassicMod( // write a classical model to a .mh file as C++ code
    int         ilev,        // in: pyramid level (0 is full scale)
    int         ipoint,      // in
    int         npoints,     // in
    const VEC&  mean,        // in
    const MAT&  cov,         // in: inverted covariance matrix
    const char* outdir,      // in: output directory (-d flag)
    const char* modname,     // in: e.g. "yaw00"
    const char* cmdline)     // in: command line used to invoke tasm
{
    char format[SLEN]; // e.g. "%s/mh/%s_lev%d_p2.2%d_classic.mh" for npoints 10..99
    const int ndigits = NumDigits(npoints);
    sprintf(format, "%%s/mh/%%s_lev%%d_p%%%d.%dd_classic.mh", ndigits, ndigits);
    char path[SLEN];
    sprintf(path, format, outdir, modname, ilev, ipoint);
    static int printed;
    PrintOnce(printed, "\nGenerating %s...\n", path);
    FILE* file = fopen(path, "wb");
    if (!file)
        Err("Cannot open %s", path);
    Fprintf(file, "// %s.mh: machine generated classical descriptor matcher\n", Base(path));
    Fprintf(file, "//\n");
    Fprintf(file, "// Command: %s\n\n", cmdline);
    char path1[SLEN];
    sprintf(format, "stasm_%%s_lev%%d_p%%%d.%dd_classic_mh", ndigits, ndigits);
    sprintf(path1, format, modname, ilev, ipoint);
    ToUpper(path1);
    Fprintf(file, "#ifndef %s\n", path1);
    Fprintf(file, "#define %s\n\n", path1);
    Fprintf(file, "namespace stasm {\n\n");
    Fprintf(file, "// static const int EYEMOUTH_DIST = %d;\n", EYEMOUTH_DIST);
    Fprintf(file, "// static const double PYR_RATIO = %g;\n", PYR_RATIO);
    Fprintf(file, "\n");

    char s[SLEN];
    sprintf(format, "%%s_lev%%d_p%%%d.%dd_prof", ndigits, ndigits);
    sprintf(s, format, modname, ilev, ipoint);
    WriteMatAsArray(mean, s, "mean profile", file);

    sprintf(format, "%%s_cov_lev%%d_p%%%d.%dd", ndigits, ndigits);
    sprintf(s, format, modname, ilev, ipoint);
    WriteMatAsArray(cov, s, "covariance matrix", file);

    sprintf(format,
            "\nstatic const ClassicDescMod %%s_lev%%d_p%%%d.%dd_classic"
            "(%%d, %%s_lev%%d_p%%%d.%dd_prof, %%s_cov_lev%%d_p%%%d.%dd);\n\n",
            ndigits, ndigits,
            ndigits, ndigits,
            ndigits, ndigits);

    Fprintf(file, format,
            modname, ilev, ipoint,
            NSIZE(mean),
            modname, ilev, ipoint,
            modname, ilev, ipoint);

    Fprintf(file, "} // namespace stasm\n", path1);
    Fprintf(file, "#endif // %s\n", path1);
    fclose(file);
}

void GenClassicMod( // generate the classical descriptor model for one point at the given pyr level
    int           ilev,    // in: pyramid level (0 is full scale)
    int           ipoint,  // in
    int           npoints, // in
    const vec_VEC descs,   // in: [idesc] classical 1D descriptors for current point
    const char*   msg,     // in: appended to err msg if any e.g. "(level 1 point 2)"
    const char*   outdir,  // in: output directory (-d flag)
    const char*   modname, // in: e.g. "yaw00"
    const char*   cmdline) // in: command line used to invoke tasm
{
    const int proflen = NSIZE(descs[0]);

    MAT mean(1, proflen, 0.); // mean profile

    for (int idesc = 0; idesc < NSIZE(descs); idesc++)
        mean += descs[idesc];

    mean /= NSIZE(descs);

    MAT cov(proflen, proflen, 0.); //  inverted covariance matrix

    GetInvertedCovMat(cov, descs, mean, msg);

    WriteClassicMod(ilev, ipoint, npoints, mean, cov, outdir, modname, cmdline);

    CheckMem();
}

} // namespace stasm
