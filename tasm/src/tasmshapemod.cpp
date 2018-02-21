// tasmshapemod.cpp: generate the shape model
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
// Clone oldshapes to newshapes.
// Needed because a simple assignment (newshapes = oldshapes) does
// not copy the buffers (it just copies the buffer pointers).

void CopyShapeVec(
    vec_Shape&       newshapes, // out
    const vec_Shape& oldshapes) // in
{
    newshapes.resize(NSIZE(oldshapes));
    for (int i = 0; i < NSIZE(oldshapes); i++)
        oldshapes[i].copyTo(newshapes[i]);
}

// This function is like CV_Assert(mat == mat.t()) but tells you where the
// difference is, if any.  It's also quicker than testing mat == mat.t().

#ifdef _DEBUG
static bool IsSymmetric( // true if mat is symmetric
    const MAT& mat,      // in
    double     min)      // in
{
    bool exact = (min == 0.0);
    if (mat.rows != mat.cols)
    {
        lprintf("\nmatrix not square\n");
        return false; // note return
    }
    if (exact)
    {
        const int nrows = mat.rows;
        for (int row = 0; row < nrows; row++)
            for (int col = row+1; col < mat.cols; col++)
                if (mat(row, col) != mat(col, row))
                {
                    lprintf("\nmatrix not exactly symmetric: "
                        "mat(%d,%d)=%g != mat(%d,%d)=%g   "
                        "mat(%d,%d)-mat(%d,%d)=%g\n",
                        row, col, mat(row, col), col, row, mat(col, row),
                        row, col, row, col, mat(row, col)-mat(col, row));
                    return false; // note return
                }
    }
    else
    {
        min = ABS(min);
        const int nrows = mat.rows;
        for (int row = 0; row < nrows; row++)
            for (int col = row+1; col < mat.cols; col++)
                if (!Equal(mat(row, col), mat(col, row), min))
                {
                    lprintf("\nmatrix not symmetric: "
                        "mat(%d,%d)=%g != mat(%d,%d)=%g   "
                        "mat(%d,%d)-mat(%d,%d)=%g   min=%g\n",
                        row, col, mat(row, col), col, row, mat(col, row),
                        row, col, row, col, mat(row, col)-mat(col, row), min);
                    return false; // note return
                }
    }
    return true;
}
#endif // _DEBUG

int NbrUsedPoints(      // return the number of used points in shape
    const Shape& shape) // in
{
    int nused = 0;

    for (int ipoint = 0; ipoint < shape.rows; ipoint++)
        if (PointUsed(shape, ipoint))
            nused++;

    return nused;
}

static void NbrUsedPointsVec(   // nbr of used points in each shape
    vec_int&         nused_vec, // out: vec [nshapes] of int
    const vec_Shape& shapes)    // in
{
    nused_vec.resize(NSIZE(shapes));
    for (int ishape = 0; ishape < NSIZE(shapes); ishape++)
        nused_vec[ishape] = NbrUsedPoints(shapes[ishape]);
}

// Return the index of the first shape in tags which has no missing points.
// Issue error message if can't do that.

static int FirstShapeWithAllPoints(
    const ShapeFile& sh)
{
    for (int ishape = 0; ishape < sh.nshapes_; ishape++)
        if (NbrUsedPoints(sh.shapes_[ishape]) == sh.shapes_[ishape].rows)
        {
            lprintf("Reference shape %s is at index %d and has %d landmarks\n",
                    sh.bases_[ishape].c_str(), ishape, sh.shapes_[ishape].rows);
            return ishape; // note: return
        }

    Err("%s: all shapes have unused points", sh.shapepath_);
    return 0; // never get here
}

static int NbrShapesWithMissingPoints( // number of shapes with a missing point
    const vec_Shape& shapes,           // in
    const vec_int&   nused_vec)        // in: vec [nshapes] of int
{
    int nshapes_with_missing_points = 0;

    for (int ishape = 0; ishape < NSIZE(shapes); ishape++)
        if (nused_vec[ishape] != shapes[0].rows)
            nshapes_with_missing_points++;

    return nshapes_with_missing_points;
}

static void ShowPercentagePointsMissing(
    int              nshapes_with_missing_points, // in
    const vec_Shape& shapes)                      // in
{
    const int nshapes = NSIZE(shapes);
    const int npoints = shapes[0].rows;
    int ipoint_min = -1, min = nshapes;

    lprintf("%d (%.0f%%) of %d shapes have all %d points\n",
        nshapes - nshapes_with_missing_points,
        100 * double(nshapes - nshapes_with_missing_points) / nshapes,
        nshapes, npoints);

    lprintf("Percentage of points unused over all shapes:\n");
    lprintf("         0      1      2      3      4      5      "
            "6      7      8      9");
    for (int ipoint = 0; ipoint < npoints; ipoint++)
    {
        if (ipoint % 10 == 0)
            lprintf("\n  %2d ", ipoint);
        int nused = 0; // for current point, nbr used over all shapes
        for (int ishape = 0; ishape < nshapes; ishape++)
            if (PointUsed(shapes[ishape], ipoint))
                nused++;
        if (nused == nshapes) // all used?
            lprintf("    .  ");
        else
            lprintf(" %6.2f", 100. * (nshapes - nused) / nshapes);
        if (nused < min)
        {
            min = nused;
            ipoint_min = ipoint;
        }
    }
    lprintf("\n");
    if (min == 0)
        Err("Point %d is unused in all shapes",
            ipoint_min, 100. * min/nshapes);
    else if (min < .333 * nshapes) // .333 is arb
    {
        // error message is issued because we need more used points
        // to build a shape model, even when imputing points
        Err("Point %d is unused in %.0f%% of the shapes ",
            ipoint_min, 100. * (nshapes - min) / double(nshapes));
    }
}

// Calculates the covariance matrix for all x and y values in shapes.
// It uses all shapes, even those with unused points.  The unused points
// are ignored when calculating the covariance matrix, but the other points
// in the shape are used.

static MAT CalcShapeCov(
    const vec_Shape& shapes,    // in
    const Shape&     meanshape) // in
{
    int npoints = meanshape.rows;
    int i, j;

    MAT cov(2 * npoints, 2 * npoints, 0.); // covariance of every x and y coord
    MAT nused(npoints, npoints, 0.);

    // Because of possible unused points, we have to iterate by
    // hand (we can't use standard matrix multiplies).

    for (int ishape = 0; ishape < NSIZE(shapes); ishape++)
    {
        Shape shape(shapes[ishape]);
        for (i = 0; i < npoints; i++)
            if (PointUsed(shape, i))
                for (j = 0; j < npoints; j++)
                    if (PointUsed(shape, j))
                    {
                        cov(2*i,   2*j)   +=  // x * x
                            (shape(i, IX) - meanshape(i, IX)) *
                            (shape(j, IX) - meanshape(j, IX));

                        cov(2*i+1, 2*j)   +=  // y * x
                            (shape(i, IY) - meanshape(i, IY)) *
                            (shape(j, IX) - meanshape(j, IX));

                        cov(2*i,   2*j+1) +=  // x * y
                            (shape(i, IX) - meanshape(i, IX)) *
                            (shape(j, IY) - meanshape(j, IY));

                        cov(2*i+1, 2*j+1) +=  // y * y
                            (shape(i, IY) - meanshape(i, IY)) *
                            (shape(j, IY) - meanshape(j, IY));

                        nused(i, j)++;
                    }
       }

    for (i = 0; i < npoints; i++)
        for (j = 0; j < npoints; j++)
        {
            const double n = nused(i, j);
            if (n < 3) // 3 is somewhat arb
                Err("Cannot calculate covariance of %g shape%s (need more shapes)",
                    n, plural(int(n)));
            cov(2*i,   2*j)   /=  n;
            cov(2*i+1, 2*j)   /=  n;
            cov(2*i,   2*j+1) /=  n;
            cov(2*i+1, 2*j+1) /=  n;
        }

    return cov;
}

// Mathematically the sign of an eigenvector is not uniquely determined.
// Therefore, for consistency across software versions, we flip signs if
// necessary so the largest (in magnitude) element of each eigvec is positive.

static void FixEigSigns(
    MAT& eigvecs)        // io: one row per eigvec
{
    for (int ivec = 0; ivec < eigvecs.rows; ivec++) // ivec is the eigenvec number
    {
        double absmax = -FLT_MAX; int imax = 0;
        for (int i = 0; i < eigvecs.cols; i++)      // i is the elem in eigenvec
            if (ABS(eigvecs(ivec, i)) > absmax)
            {
                absmax = ABS(eigvecs(ivec, i));
                imax = i;
            }
        if (eigvecs(ivec, imax) < 0) // largest elem in eigenvec is negative?
            for (int i = 0; i < eigvecs.cols; i++)  // flip signs
                eigvecs(ivec, i) = -eigvecs(ivec, i);
    }
}

// Returns a matrix of eigenvectors for the symmetric matrix mat.
//
// Each column of the returned matrix will be an eigenvector, normalized to
// unit length.  The eigvals are returned in eigvals, one row per eigenvec.
// mat must be a symmetric matrix.

static void EigsOfSymMat(
    MAT&       eigvecs,        // out: one row per eigvec
    MAT&       eigvals,        // out: sorted
    const MAT& mat,            // in: not modified
    bool       fix_signs=true) // in: see FixEigSigns
{
    CV_DbgAssert(IsSymmetric(mat, cv::trace(mat)[0] / double(1e16)));
    eigen(mat, eigvals, eigvecs);
    if (fix_signs)
        FixEigSigns(eigvecs);
}

// Set very small eig values and their corresponding eig vecs to 0.
// This gives improved consistency between _DEBUG and release versions and
// different compilers.  Differences can arise because of numerical errors.

static void ZapSmallEigs(
    MAT& eigvals,         // io
    MAT& eigvecs)         // io
{
    const double min = eigvals(0) / 1e6;
    CV_Assert(min > 0);
    for (int eig = 0; eig < NSIZE(eigvals); eig++)
        if (eigvals(eig) < min)
        {
            eigvals(eig) = 0;
            eigvecs.row(eig) = 0; // set all eigenvector elems to 0
        }
}

// Informational.  Print first n eig vals, up to and including the first
// small value.  But no more than 10 of them.

static void ShowEigs(const MAT& eigvals)
{
    double min_eig = eigvals(0) / 1000;
    double var_all = SumElems(eigvals);
    double var10 = 0;
    int neigs = NSIZE(eigvals);
    for (int i = 0; i < MIN(10, neigs); i++)
        var10 += eigvals(i);
    int iprint = 0;
    while (iprint < neigs && eigvals(iprint) > min_eig && iprint < 9)
        iprint++;
    if (iprint < neigs + 1) // show first small eig value, for context
        iprint++;
    lprintf("%.0f%% percent variance is explained by %s%d shape eigs:",
            100 * var10 / var_all, (iprint < neigs? "the first ": ""), iprint);
    const MAT eigvals_t(eigvals.t());
    for (int i = 0; i < iprint; i++)
        lprintf(" %.0f", 100 * eigvals_t(i) / var_all);
    lprintf("%%\n");
}

static void WriteShapeMod(  // write a shape model to a .mh file as C++ code
    const char*  modname,   // in
    const Shape& meanshape, // in: n x 2
    const VEC&   eigvals,   // in: 2n x 1
    const MAT&   eigvecs,   // in: 2n x 2n, inverse of eigs of cov mat
    const char*  outdir,    // in: output directory
    const char*  comment)   // in
{
    char path[SLEN];
    sprintf(path, "%s/mh/%s_shapemodel.mh", outdir, modname);
    lprintf("Generating %s\n", path);
    FILE *file = fopen(path, "wb");
    if (!file)
        Err("Cannot open %s", path);
    char s[SLEN];
    Fprintf(file, "// %s.mh: machine generated shape model\n", path);
    Fprintf(file, "//\n");
    Fprintf(file, "// Command: %s\n//\n\n", comment);
    char path1[SLEN]; sprintf(path1, "stasm_%s_shapemodel_mh", modname);
    ToUpper(path1);
    Fprintf(file, "#ifndef %s\n", path1);
    Fprintf(file, "#define %s\n\n", path1);
    Fprintf(file, "namespace stasm {\n\n");

    sprintf(s, "%s_meanshapedata", modname);
    WriteMatAsArray(meanshape, s, "mean shape", file);

    Fprintf(file,
        "\nstatic const cv::Mat %s_meanshape(%d, %d, CV_64FC1, "
        "(double *)%s_meanshapedata);\n\n",
        modname, meanshape.rows, meanshape.cols, modname);

    sprintf(s, "%s_eigvalsdata", modname);
    WriteMatAsArray(eigvals, s, "eigen values", file);

    Fprintf(file,
        "\nstatic const cv::Mat %s_eigvals(%d, %d, CV_64FC1, "
        "(double *)%s_eigvalsdata);\n\n",
        modname, eigvals.rows, eigvals.cols, modname);

    sprintf(s, "%s_eigvecsdata", modname);
    WriteMatAsArray(eigvecs, s, "eigen vectors", file);

    Fprintf(file,
        "\nstatic const cv::Mat %s_eigvecs(%d, %d, CV_64FC1, "
        "(double *)%s_eigvecsdata);\n\n",
        modname, eigvecs.rows, eigvecs.cols, modname);

    Fprintf(file, "} // namespace stasm\n", path1);
    Fprintf(file, "#endif // %s\n", path1);
    fclose(file);
}

static void GenShapeMod(
    Shape&           meanshape,      // out: n x 2
    VEC&             eigvals,        // out: 2n x 1
    MAT&             eigvecs,        // out: 2n x 2n, inverse of eigs of cov mat
    vec_Shape&       aligned_shapes, // out: shapes aligned to refshape
    const ShapeFile& sh,             // in
    int              irefshape,      // in: index of shape used to align other shapes
    const vec_int&   nused_vec,      // in: vec [nshapes] of int
    const char*      outdir,         // in: output directory (-d flag)
    const char*      modname,        // in: e.g. "yaw00"
    const char*      cmdline,        // in: command line used to invoke tasm
    bool             force_facedet,  // in: facedet the images themselves (ignore shapefile facedets)
    bool             write_detpars)  // in: write the image detpars to facedet.shape
{
    CopyShapeVec(aligned_shapes, sh.shapes_);

    // after this, mean shape and aligned_shapes will be aligned to ref shape
    AlignTrainingShapes(meanshape, aligned_shapes,
                        aligned_shapes[irefshape]);

    MAT cov(CalcShapeCov(aligned_shapes, meanshape));
    EigsOfSymMat(eigvecs, eigvals, cov);

    if (eigvals(0) < 0.1) // number is arbitrary
        lprintf("eigen data invalid "
                "(not enough variation in shapes, max eigenvalue is %g)", eigvals(0));

    eigvals *= 1000 / eigvals(0); // normalize so biggest eigval is 1000 (to match old Stasm)

    ZapSmallEigs(eigvals, eigvecs);

    eigvecs = eigvecs.t(); // invert (eigvecs is orthog so transpose is inverse)

    ShowEigs(eigvals);

    // now align mean shape to the face detector frame
    MeanShapeAlignedToFaceDets(meanshape,
        sh, nused_vec, outdir, modname, cmdline, force_facedet, write_detpars);
}

// write the imputed shapes so they can be manually checked later if necessary

static void WriteImputedShapes(
    const ShapeFile& sh,        // in
    const char*      outdir,    // in: output directory (-d flag)
    const char*      cmdline)   // in: command line used to invoke tasm
{
    clock_t start_time = clock();
    char path[SLEN]; sprintf(path, "%s/log/imputed.shape", outdir);
    lprintf("Writing %s ", path);
    sh.Write_(path, sh.dirs_, ssprintf("# Command: %s\n", cmdline));
    lprintf("[%.1f secs]\n", double(clock() - start_time) / CLOCKS_PER_SEC);
}

void GenAndWriteShapeMod(             // generate the shape model and write it to a .mh file
    ShapeFile&  sh,                   // io: shapes_ field modified if there are missing points
                                      //     entire sh changed if TASM_SUBSET_DESC_MODS
    const char* outdir,               // in: output directory (-d flag)
    const char* modname,              // in: e.g. "yaw00"
    const char* cmdline,              // in: command line used to invoke tasm
    bool        force_facedet,        // in: facedet the images themselves (ignore shapefile facedets)
    bool        write_detpars,        // in: write the image detpars to facedet.shape
    bool        write_imputed_shapes, // in: write imputed.shapes file if shapes are imputed
    bool        write_imgs)           // in: write images showing landmark tab values
{
    clock_t start_time = clock();
    lprintf("--- Generating the shape model ---\n");

    // the reference shape is the first shape with no missing
    // points and irefshape is its index
    int irefshape = FirstShapeWithAllPoints(sh);

    vec_int nused_vec; // vec [nshapes] of int, nbr of used points in each shape
    NbrUsedPointsVec(nused_vec, sh.shapes_);

    const int nshapes_with_missing_points =
        NbrShapesWithMissingPoints(sh.shapes_, nused_vec);

    if (nshapes_with_missing_points)
        ShowPercentagePointsMissing(
            nshapes_with_missing_points, sh.shapes_);

    Shape     meanshape;
    VEC       eigvals;
    MAT       eigvecs;
    vec_Shape aligned_shapes; // sh.shapes_ aligned to ref shape

    GenShapeMod(
        meanshape, eigvals, eigvecs, aligned_shapes,
        sh, irefshape, nused_vec, outdir, modname, cmdline,
        force_facedet, write_detpars);

    // shapemodel_sh is the shapes we used to build the shape model
    // (this will be sh unless TASM_REBUILD_SUBSET is not NULL)
    ShapeFile shapemodel_sh(sh); // copy sh to shapemodel_sh

    if (nshapes_with_missing_points)
    {
        // Points are missing in some shapes (typically because the points
        // were manually landmarked as missing because they are obscured).
        // Update sh.shape by imputing those points.
        // After this, all shapes in sh.shapes_ have all points.

        ImputeMissingPoints(sh.shapes_,
                            aligned_shapes, meanshape, eigvals, eigvecs);

        if (write_imputed_shapes)
            WriteImputedShapes(sh, outdir, cmdline); // write imputed.shape

        if (TASM_REBUILD_SUBSET)
        {
            lprintf("--- Regenerating the shape model "
                    "using the imputed shapes matching %s---\n",
                    TASM_REBUILD_SUBSET);
            lprintf("Reference shape is now the first shape "
                    "(since all shapes now all have all landmarks)\n");
            irefshape = 0;
            // copy sh from ImputeMissingPoints above to shapemodel_sh
            shapemodel_sh = sh;
            shapemodel_sh.Subset_(TASM_REBUILD_SUBSET);
            if (TASM_SUBSET_DESC_MODS)
                sh = shapemodel_sh;
            NbrUsedPointsVec(nused_vec, shapemodel_sh.shapes_);
            GenShapeMod(
                meanshape, eigvals, eigvecs, aligned_shapes,
                shapemodel_sh, irefshape, nused_vec, outdir,
                modname, cmdline,
                force_facedet, false /*write_detpars*/);
        }
    }
    WriteShapeMod(TASM_MODNAME, meanshape, eigvals, eigvecs, outdir, cmdline);

    const double totaltime = double(clock() - start_time) / CLOCKS_PER_SEC;
    lprintf("[%.1f secs%s to generate the shape model]\n",
            totaltime,
            totaltime > 180? ssprintf(" (%.1f minutes)", totaltime / 60): "");

    if (write_imgs)
        TasmDraw(shapemodel_sh, irefshape, meanshape, outdir);

    // sanity check that we convert to Shape17 correctly or not at all
    const Shape shape17(Shape17OrEmpty(sh.shapes_[irefshape]));
    if (shape17.rows) // converted to Shape17?
        SanityCheckShape17(shape17);
}

} // namespace stasm
