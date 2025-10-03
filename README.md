# 2025-fall-student3-uasika2
Student: Ugo
Worked solo (no partner)

Work Summary:

I implemented all the required transformations in c milestone 1, and assembly milestone 2, and for milestone 3, I implemented the ellipse and emboss transformations inc. The helper functions were written in c.Unit tests were added in imgproc_tests.c to cover these helper functions and to validate all API transformations.

Implementation Details: 

Complements: inverts the rgb values of each pixel while preservng alpha.
Transpose: Swaps rows and columns for square images, return 0 for non square.
Ellipse: keeps pixels within a centered ellipse; others remain opaque black.. Uses integer math with sscaling by 1000 to avoid floating point.
Emboss: Applies the emboss effect by comparing each pixel to its upper left neighbout. Handlles tie breaking by prioritising red over green and green over blue. Clamps values to between 0 and 255 inclusive.

Notes:
no memory errors.