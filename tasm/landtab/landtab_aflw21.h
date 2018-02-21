// landtab_aflw21.h: definitions for AFLW 21 point shapes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef STASM_LANDTAB_AFLW21_H
#define STASM_LANDTAB_AFLW21_H

#if 0
enum LANDTAB_21      // the 21 points that make up an AFLW shape
{
    L21_LEyebrowOuter, // 00 left eyebrow outer
    L21_LEyebrowMid,   // 01 left eyebrow mid
    L21_LEyebrowInner, // 02 left eyebrow inner
    L21_REyebrowInner, // 03 right eyebrow inner
    L21_REyebrowMid,   // 04 right eyebrow mid
    L21_REyebrowOuter, // 05 right eyebrow outer
    L21_LEyeOuter,     // 06 left eye outer
    L21_LPupil,        // 07 left pupil
    L21_LEyeInner,     // 08 left eye inner
    L21_REyeInner,     // 09 right eye inner
    L21_RPupil,        // 10 right pupil
    L21_REyeOuter,     // 11 right eye outer
    L21_LJaw,          // 12 left jaw
    L21_LNose,         // 13 left nose
    L21_CNoseTip,      // 14 nose tip
    L21_RNose,         // 15 right nose
    L21_RJaw,          // 16 right jaw
    L21_LMouthCorner,  // 17 left mouth corner
    L21_CMouthMid,     // 18 mouth mid
    L21_RMouthCorner,  // 19 right mouth corner
    L21_CChin          // 20 chin
};
#endif

// Note that the AT_Hat bit is ignored if pyr lev > HAT_START_LEV

static const LANDMARK_INFO LANDMARK_INFO_TAB[21] = // 21 points
{ // par pre next weight bits
    {  5, 12,  1, 1., AT_Glasses|AT_Hat          }, // 00 L21_LEyebrowOuter
    {  4,  0,  2, 1., AT_Glasses|AT_Hat          }, // 01 L21_LEyebrowMid
    {  3,  1,  3, 1., AT_Glasses|AT_Hat          }, // 02 L21_LEyebrowInner

    {  2,  2,  4, 1., AT_Glasses|AT_Hat          }, // 03 L21_REyebrowInner
    {  1,  3,  5, 1., AT_Glasses|AT_Hat          }, // 04 L21_REyebrowMid
    {  0, 16,  4, 1., AT_Glasses|AT_Hat          }, // 05 L21_REyebrowOuter

    { 11,  0,  8, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 06 L21_LEyeOuter
    { 10,  6,  8, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 07 L21_LPupil
    {  9,  6,  9, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 08 L21_LEyeInner

    {  8, 11,  8, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 09 L21_REyeInner
    {  7, 11,  9, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 10 L21_RPupil
    {  6,  5,  9, 1., AT_Glasses|AT_Eye|AT_Hat   }, // 11 L21_REyeOuter

    { 16,  6, 20, 1., AT_Beard|AT_Glasses|AT_Hat }, // 12 L21_LJaw
    { 15, 12, 16, 1., AT_Hat                     }, // 13 L21_LNose

    { -1, 12, 16, 1., AT_Hat                     }, // 14 L21_CNoseTip

    { 13, 16, 12, 1., AT_Hat                     }, // 15 L21_RNose
    { 12, 11, 20, 1., AT_Beard|AT_Glasses|AT_Hat }, // 16 L21_RJaw

    { 19,  6, 20, 1., AT_Mustache|AT_Hat         }, // 17 L21_LMouthCorner
    { -1, 17, 19, 1., AT_Mustache|AT_Hat         }, // 18 L21_CMouthMid
    { 17, 20,  9, 1., AT_Mustache|AT_Hat         }, // 19 L21_RMouthCorner

    { -1, 12, 16, 1., AT_Beard|AT_Hat            }  // 20 L21_CChin
};

#endif // STASM_LANDTAB_AFLW21_H
