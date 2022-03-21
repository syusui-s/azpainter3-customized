/*$
 Copyright (C) 2013-2022 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/***************************************
 * フィルタ描画関数
 ***************************************/

/* filter_color_alpha.c */

mlkbool FilterDraw_color_brightcont(FilterDrawInfo *info);
mlkbool FilterDraw_color_gamma(FilterDrawInfo *info);
mlkbool FilterDraw_color_level(FilterDrawInfo *info);
mlkbool FilterDraw_color_rgb(FilterDrawInfo *info);
mlkbool FilterDraw_color_hsv(FilterDrawInfo *info);
mlkbool FilterDraw_color_hsl(FilterDrawInfo *info);
mlkbool FilterDraw_color_nega(FilterDrawInfo *info);
mlkbool FilterDraw_color_grayscale(FilterDrawInfo *info);
mlkbool FilterDraw_color_sepia(FilterDrawInfo *info);
mlkbool FilterDraw_color_gradmap(FilterDrawInfo *info);
mlkbool FilterDraw_color_threshold(FilterDrawInfo *info);
mlkbool FilterDraw_color_threshold_dither(FilterDrawInfo *info);
mlkbool FilterDraw_color_posterize(FilterDrawInfo *info);

mlkbool FilterDraw_color_replace_drawcol(FilterDrawInfo *info);
mlkbool FilterDraw_color_replace(FilterDrawInfo *info);

mlkbool FilterDraw_alpha_checked(FilterDrawInfo *info);
mlkbool FilterDraw_alpha_current(FilterDrawInfo *info);

/* filter_blur.c */

mlkbool FilterDraw_blur(FilterDrawInfo *info);
mlkbool FilterDraw_gaussblur(FilterDrawInfo *info);
mlkbool FilterDraw_motionblur(FilterDrawInfo *info);
mlkbool FilterDraw_radialblur(FilterDrawInfo *info);
mlkbool FilterDraw_lensblur(FilterDrawInfo *info);

/* filter_draw.c */

mlkbool FilterDraw_draw_cloud(FilterDrawInfo *info);
mlkbool FilterDraw_draw_amitone(FilterDrawInfo *info);
mlkbool FilterDraw_draw_randpoint(FilterDrawInfo *info);
mlkbool FilterDraw_draw_edgepoint(FilterDrawInfo *info);
mlkbool FilterDraw_draw_frame(FilterDrawInfo *info);
mlkbool FilterDraw_draw_horzvertLine(FilterDrawInfo *info);
mlkbool FilterDraw_draw_plaid(FilterDrawInfo *info);

/* filter_comic_tone.c */

mlkbool FilterDraw_comic_amitone_create(FilterDrawInfo *info);
mlkbool FilterDraw_comic_to_amitone(FilterDrawInfo *info);
mlkbool FilterDraw_comic_sand_tone(FilterDrawInfo *info);

/* filter_comic_draw.c */

mlkbool FilterDraw_comic_concline_flash(FilterDrawInfo *info);
mlkbool FilterDraw_comic_popupflash(FilterDrawInfo *info);
mlkbool FilterDraw_comic_uniflash(FilterDrawInfo *info);
mlkbool FilterDraw_comic_uniflash_wave(FilterDrawInfo *info);

/* filter_pixelate.c */

mlkbool FilterDraw_mozaic(FilterDrawInfo *info);
mlkbool FilterDraw_crystal(FilterDrawInfo *info);
mlkbool FilterDraw_halftone(FilterDrawInfo *info);

/* filter_edge.c */

mlkbool FilterDraw_sharp(FilterDrawInfo *info);
mlkbool FilterDraw_unsharpmask(FilterDrawInfo *info);
mlkbool FilterDraw_edge_sobel(FilterDrawInfo *info);
mlkbool FilterDraw_edge_laplacian(FilterDrawInfo *info);
mlkbool FilterDraw_highpass(FilterDrawInfo *info);

/* filter_effect.c */

mlkbool FilterDraw_effect_glow(FilterDrawInfo *info);
mlkbool FilterDraw_effect_rgbshift(FilterDrawInfo *info);
mlkbool FilterDraw_effect_oilpaint(FilterDrawInfo *info);
mlkbool FilterDraw_effect_emboss(FilterDrawInfo *info);
mlkbool FilterDraw_effect_noise(FilterDrawInfo *info);
mlkbool FilterDraw_effect_diffusion(FilterDrawInfo *info);
mlkbool FilterDraw_effect_scratch(FilterDrawInfo *info);
mlkbool FilterDraw_effect_median(FilterDrawInfo *info);
mlkbool FilterDraw_effect_blurring(FilterDrawInfo *info);

/* filter_transform.c */

mlkbool FilterDraw_trans_wave(FilterDrawInfo *info);
mlkbool FilterDraw_trans_ripple(FilterDrawInfo *info);
mlkbool FilterDraw_trans_polar(FilterDrawInfo *info);
mlkbool FilterDraw_trans_radial_shift(FilterDrawInfo *info);
mlkbool FilterDraw_trans_swirl(FilterDrawInfo *info);

/* filter_other.c */

mlkbool FilterDraw_lum_to_alpha(FilterDrawInfo *info);
mlkbool FilterDraw_dot_thinning(FilterDrawInfo *info);
mlkbool FilterDraw_hemming(FilterDrawInfo *info);
mlkbool FilterDraw_3dframe(FilterDrawInfo *info);
mlkbool FilterDraw_shift(FilterDrawInfo *info);

/* filter_antialiasing.c */

mlkbool FilterDraw_antialiasing(FilterDrawInfo *info);

