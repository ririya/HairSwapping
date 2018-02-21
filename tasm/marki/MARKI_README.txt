This directory contains interim versions of Marki and Cshapes.

Marki issues
------------

Some of these are out of date.

Off-by-one error on y mouse position, rare but annoying.

image00849 has missing eyes because they are behind glasses, initial
position is hosed.  This is a situation where it would be nice
to be able to scroll the image.

If image is low res then increase its size for display (issues with
small images --- pixelation).

Cursors need tweaking?

QuickMsgBox time should be longer?

When get image name with save image dialog, need to add bmp to the
name before checking?

Could do sanity checks on landmarked face (check if it conforms
to a plausible to a real face).

Mark -C doesn't progress if the window is minimized.

When we update aflw.shape we drop the AFLW record and other info.
Not so cool.

Logic is not perfect when zooming when zoomed width is less than
MIN_ZOOMED_WIDTH or greater than ImgWidth.

Tweak sizes of text and points in displayed shape annotations.

After expanding a window you sometimes can't unexpand it easily.

The dialog window is a bit user unfriendly.

Marki crashed when an operator was using right arrow repeatedly on set04.shape

When paging forward using the keyboard autorepeat on the pagedown key,
most of dialog window updates correctly but the update of image name
and nbr is delayed until you lift the pagedown key.

JumpToLandmark doesn't seem to work properly with unused landmarks.

Usage questions
---------------

This list is out of date.

Is it better to manually landmark with or without the current landmark
annotations?

Is it better to manually landmark on colored or monochrome images?

What zoom is best for landmarking?  1.3 or 1.5 seems good for general work?

Users must be careful not to update right when they mean left eye if
original eye coords are not available and the missing landmark
position is deceiving.

Must think about closed eyes.  For now, landmarkers should estimate
the position of the pupil and later we will tag and exclude such
images when training models.

Discuss where to put pupil on closed eyes.  Suggest center of eye
between where the upper and lower eyelid meet.

Example of a bad AFLW eye pupil is left pupil of image04242.

If user's landmark is more than half the intereye distance far from
original we put a "!" after the "Mark" message.

The same marker should mark symmetric points, for consistency when
mirror the images during training to effictively double the number of
images.

Tips on tagging bad images
--------------------------

Start Marki in "tag mode", like this

    marki -t 80000000 set02.shape

If you right click, you will set the 80000000 tag bit
in the image's tag string.

If you left click you will clear the tag bit.  It usually easier just
to hit SPACE to move to the next image.  Also, set AutoNextImage for
speed.

When paging through the images it's best if you set the landmark
number to a central landmark (the tip of the nose) and set the zoom to
about 1.3 or 1.5.

When you are done, run marki like this, to select only
the good images (i.e. with tag bit 80000000 not set):

    marki -P 80000000 0 -o newshape.shape set02.shape

and click on the save button within Marki.  The good shapes will be
written to newshape.shape
