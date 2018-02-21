// tasmdraw.cpp: draw the landmark table etc., for debugging purposes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
const double SCALE_NBRS = .3; // size of landmark numbers in images showing landmarks

static void DrawLandmarks(     // draw an image showing all landmarks, numbered
    const Shape&  shape,       // in
    const CImage& orgimg,      // in
    const char*   basename,    // in
    const Shape&  cropshape,   // in: used only to crop the image
    const char*   outdir,      // in: output directory (-d flag)
    double        scale,       // in
    int           circle_size, // in
    bool          dots)        // in
{
    CImage scaled_img(orgimg.clone()); // local copy we can write to
    cv::resize(scaled_img, scaled_img, cv::Size(), scale, scale, cv::INTER_LINEAR);
    const Shape scaled_shape(shape * scale);
    DrawShape(scaled_img, scaled_shape, 0xffffff, dots);
    if (dots)
        for (int ipoint = 0; ipoint < scaled_shape.rows; ipoint++)
            ImgPrintf(scaled_img,
                      cvRound(scaled_shape(ipoint, IX)) + 2 * circle_size,
                      cvRound(scaled_shape(ipoint, IY)),
                      0xa0a0a0, SCALE_NBRS, "%d", ipoint); // landmark number in gray
    char path[SLEN];
    sprintf(path, "%s/log/%s.bmp", outdir, basename);
    CropCimgToShapeWithMargin(scaled_img, cropshape * scale);
    ImgPrintf(scaled_img, 5, MAX(10, .05 * scaled_img.rows), C_YELLOW, 1.2,
              "%s", Base(path));
    lprintf("    %s\n", path);
    if (!cv::imwrite(path, scaled_img))
        Err("Cannot write %s", path);
}

static void Circle(           // draw a circle and number at a landmark
    CImage       img,         // io
    const Shape& shape,       // in
    int          ipoint,      // in
    unsigned     color,       // in
    int          circle_size) // in
{
    int x = cvRound(shape(ipoint, IX));
    int y = cvRound(shape(ipoint, IY));
    CV_Assert(PointUsed(shape, ipoint));
    CV_Assert(x >= 0 && x < img.cols);
    CV_Assert(y >= 0 && y < img.rows);
    ImgPrintf(img, x + 2 * circle_size, y, 0xa0a0a0, // landmark number in gray
              SCALE_NBRS, "%d", ipoint);
    cv::circle(img, cv::Point(x, y), MAX(1, circle_size), ToCvColor(color), 2);
}

static void Line(          // draw a line from point1 to point2
    CImage       img,      // io
    const Shape& shape,    // in
    int          ipoint1,  // in
    int          ipoint2,  // in
    unsigned     color,    // in
    int          linesize) // in
{
    int x1 = cvRound(shape(ipoint1, IX));
    int y1 = cvRound(shape(ipoint1, IY));
    int x2 = cvRound(shape(ipoint2, IX));
    int y2 = cvRound(shape(ipoint2, IY));
    CV_Assert(PointUsed(shape, ipoint1));
    CV_Assert(x1 >= 0 && x1 < img.cols);
    CV_Assert(y1 >= 0 && y1 < img.rows);
    CV_Assert(x2 >= 0 && x2 < img.cols);
    CV_Assert(y2 >= 0 && y2 < img.rows);
    cv::line(img,
             cv::Point(x1, y1), cv::Point(x2, y2), ToCvColor(color), MAX(1, linesize));
}

static unsigned Dark(unsigned color) // return a dark version of color
{
    const double scale = .5;
    const unsigned Red   = unsigned(scale * ((color & 0xff0000) >> 16));
    const unsigned Green = unsigned(scale * ((color & 0x00ff00) >>  8));
    const unsigned Blue  = unsigned(scale * ((color & 0x0000ff) >>  0));
    return
        ((Red   << 16) & 0xff0000) |
        ((Green <<  8) & 0x00ff00) |
        ((Blue  <<  0) & 0x0000ff);
}

// print the point name and the relevant info.bits (bits are relevant if
// they are point specific, as opposed to applying to the whole image)

static void PrintLandmarkInfo(
    CImage& img,               // io: will print to this image
    int     ipoint)            // in
{
    const LANDMARK_INFO info = LANDMARK_INFO_TAB[ipoint];
    char s[SLEN]; sprintf(s, "Landmark %d", ipoint);
    if (info.partner < 0)
        strcat(s, "  NoPartner");
    else
        strcat(s, ssprintf("  Partner%d", info.partner));
    AttrBitsToString(s, info.bits, AT_Glasses|AT_Beard|AT_Mustache|AT_Eye|AT_Hat, "  ");
    ImgPrintf(img, 5, MAX(10, .05 * img.rows), C_YELLOW, 1.2, "%s", s);
}

static void DrawLandmark(      // draw one landmark with its associated info
    const Shape&  refshape,    // in
    int           ipoint,      // in
    const CImage& orgimg,      // in
    const char*   outdir,      // in: output directory (-d flag)
    double        scale,       // in
    int           circle_size) // in
{
    const Shape shape(refshape * scale);
    const int ndigits = NumDigits(shape.rows);
    char format[SLEN];
    sprintf(format, "%%s/log/landmark%%%d.%dd.bmp", ndigits, ndigits);
    char path[SLEN];
    sprintf(path, format, outdir, ipoint);
    static int printed;
    PrintOnce(printed, "    %s...\n", path);
    CImage img(orgimg.clone()); // local copy we can write to
    cv::resize(img, img, cv::Size(), scale, scale, cv::INTER_LINEAR);
    DrawShape(img, shape, 0xffffff, true);

    const LANDMARK_INFO info = LANDMARK_INFO_TAB[ipoint];

    // plot in "reverse order" so important stuff gets plotted last so not overplotted
    int prev, next;
    if (info.partner >= 0)
    {
        // plot information of ipoint's partner

        PrevAndNextLandmarks(prev, next, info.partner, shape);

        Line(img, shape, info.partner, next, Dark(C_RED), 1);
        Circle(img, shape, next, Dark(C_RED), circle_size);

        Line(img, shape, info.partner, prev, Dark(C_GREEN), 1);
        Circle(img, shape, prev, Dark(C_GREEN), circle_size);

        Circle(img, shape, info.partner, C_YELLOW, circle_size);
    }
    PrevAndNextLandmarks(prev, next, ipoint, shape);

    Line(img, shape, ipoint, next, C_RED, 1);
    Circle(img, shape, next, C_RED, circle_size);

    Line(img, shape, ipoint, prev, C_GREEN, 1);
    Circle(img, shape, prev, C_GREEN, circle_size);

    Circle(img, shape, ipoint, C_YELLOW, circle_size);

    CropCimgToShapeWithMargin(img, shape);

    PrintLandmarkInfo(img, ipoint);

    if (!cv::imwrite(path, img))
        Err("Cannot write %s", path);
}

static void ConvertLandmarksAndDraw(
    const Shape&  refshape,    // in
    const CImage& orgimg,      // in
    const char*   outdir,      // in: output directory (-d flag)
    double        scale,       // in
    int           circle_size) // in
{
    const int max_npoints = MAX(99, refshape.rows);
    const int ndigits = NumDigits(max_npoints);
    char format[SLEN];
    sprintf(format, "shape%%%d.%dd", ndigits, ndigits);
    for (int npoints = 2; npoints < max_npoints; npoints++)
    {
        // draw the shape converted to a different number of points, if able to convert
        Shape shapen(ConvertShape(refshape, npoints));
        if (shapen.rows) // successfully converted to npoints?
        {
            char basename[SLEN];
            if (shapen.rows == refshape.rows)
                sprintf(basename, "shape_all%d", refshape.rows);
            else
                sprintf(basename, format, shapen.rows);
            DrawLandmarks(shapen, orgimg, basename,
                          refshape, outdir, scale, circle_size, true /*dots*/);
        }
    }
}

void TasmDraw( // draw images for checking tasm parameters and landmark table
    const ShapeFile& sh,             // in
    int              irefshape,      // in
    const Shape&     meanshape,      // in
    const char*      outdir)         // in: output directory (-d flag)
{
    const Shape refshape(sh.shapes_[irefshape]);
    // want a rescaled shape width of at least 400 so can see numbers etc.
    const double scale = MAX(1, int(.5 + 400 / ShapeWidth(refshape)));
    // load the image from disk (must first get its full path)
    const char* imgpath = PathGivenDirs(sh.bases_[irefshape], sh.dirs_, sh.shapepath_);
    CImage orgimg(cv::imread(imgpath, CV_LOAD_IMAGE_COLOR));
    if (!orgimg.data)
        Err("Cannot load", imgpath);
    DesaturateImg(orgimg);
    DarkenImg(orgimg);
    const double width = ShapeWidth(refshape);
    const int circle_size = width > 500? 4: width > 220? 3: 1; // arb

    // draw all landmarks in LANDMARK_INFO_TAB, an image per landmark

    lprintf("--- Generating images for manual checking the %d point landmark table ---\n",
        NELEMS(LANDMARK_INFO_TAB));
    for (int ipoint = 0; ipoint < refshape.rows; ipoint++)
        DrawLandmark(refshape, ipoint, orgimg, outdir, scale, circle_size);

    // draw mean shape aligned to refshape and superimposed on the ref image

    DrawLandmarks(TransformShape(meanshape, AlignmentMat(meanshape, refshape)),
                  orgimg, "meanshape",
                  refshape, outdir, scale, circle_size, false /*dots*/);

    // convert shape to different numbers of points and draw that

    ConvertLandmarksAndDraw(refshape, orgimg, outdir, scale, circle_size);
}

} // namespace stasm
