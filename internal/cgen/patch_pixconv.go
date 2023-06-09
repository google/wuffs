// Copyright 2023 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package cgen

import (
	"fmt"
	"strings"
)

func insertBasePixConvSubmoduleYcckC(buf *buffer) error {
	src := embedBasePixConvSubmoduleYcckC.Trim()

	generalBoxFilter, err := extractCFunc(src, ""+
		"static void  //\n"+
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n")
	if err != nil {
		return err
	}

	prefix, suffix := "", ""
	const insertPatchPixconv = "// ¡ INSERT patch_pixconv\n\n"
	if i := strings.Index(src, insertPatchPixconv); i < 0 {
		return fmt.Errorf("internal error: could not find %q", insertPatchPixconv)
	} else {
		prefix, suffix = src[:i], src[i+len(insertPatchPixconv):]
	}
	stripCFuncBangComments(buf, prefix)

	const dstIterEtc = "" +
		"    size_t dst_stride = dst->private_impl.planes[0].stride;\n" +
		"    uint8_t* dst_iter =\n" +
		"        dst->private_impl.planes[0].ptr + (dst_stride * ((size_t)y));\n"
	const setColorU32At = "" +
		"      wuffs_base__poke_u32le__no_bounds_check(\n" +
		"          dst_iter, wuffs_base__color_ycc__as__color_u32_FLAVOR(\n" +
		"                        *src_iter0, *src_iter1, *src_iter2));\n" +
		"      dst_iter += 4;\n"
	const useIxHv11 = "" +
		"      src_iter0++;\n" +
		"      src_iter1++;\n" +
		"      src_iter2++;\n"
	const useIyHv11 = "" +
		"    src_ptr0 += stride0;\n" +
		"    src_ptr1 += stride1;\n" +
		"    src_ptr2 += stride2;\n"

	// ----

	if err := patchCFunc(buf, generalBoxFilter, [][2]string{{
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n",
		"wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__box_filter(\n",
	}, {
		"    // ¡ dst_iter = etc\n",
		dstIterEtc,
	}, {
		"      // ¡ BEGIN set_color_u32_at\n",
		strings.Replace(setColorU32At, "_FLAVOR", "", 1),
	}}); err != nil {
		return err
	}

	// ----

	if err := patchCFunc(buf, generalBoxFilter, [][2]string{{
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n",
		"wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__box_filter(\n",
	}, {
		"    // ¡ dst_iter = etc\n",
		dstIterEtc,
	}, {
		"      // ¡ BEGIN set_color_u32_at\n",
		strings.Replace(setColorU32At, "_FLAVOR", "_abgr", 1),
	}}); err != nil {
		return err
	}

	// ----

	if err := patchCFunc(buf, generalBoxFilter, [][2]string{{
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n",
		"wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__hv11(\n",
	}, {
		"    // ¡ dst_iter = etc\n",
		dstIterEtc,
	}, {
		"      // ¡ BEGIN set_color_u32_at\n",
		strings.Replace(setColorU32At, "_FLAVOR", "", 1),
	}, {
		"  // ¡ BEGIN declare iy\n",
		"",
	}, {
		"    // ¡ BEGIN declare ix\n",
		"",
	}, {
		"      // ¡ BEGIN use ix\n",
		useIxHv11,
	}, {
		"    // ¡ BEGIN use iy\n",
		useIyHv11,
	}}); err != nil {
		return err
	}

	// ----

	if err := patchCFunc(buf, generalBoxFilter, [][2]string{{
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n",
		"wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__hv11(\n",
	}, {
		"    // ¡ dst_iter = etc\n",
		dstIterEtc,
	}, {
		"      // ¡ BEGIN set_color_u32_at\n",
		strings.Replace(setColorU32At, "_FLAVOR", "_abgr", 1),
	}, {
		"  // ¡ BEGIN declare iy\n",
		"",
	}, {
		"    // ¡ BEGIN declare ix\n",
		"",
	}, {
		"      // ¡ BEGIN use ix\n",
		useIxHv11,
	}, {
		"    // ¡ BEGIN use iy\n",
		useIyHv11,
	}}); err != nil {
		return err
	}

	// ----

	stripCFuncBangComments(buf, suffix)
	return nil
}

func extractCFunc(src string, start string) (string, error) {
	if i := strings.Index(src, start); i < 0 {
		goto fail
	} else {
		src = src[i:]
	}

	if i := strings.Index(src, "\n}\n"); i < 0 {
		goto fail
	} else {
		return src[:i+3], nil
	}

fail:
	return "", fmt.Errorf("internal error: could not extractCFunc %q", start)
}

func stripCFuncBangComments(buf *buffer, s string) {
	for {
		const slashEtc = "// ¡"
		i := strings.Index(s, slashEtc)
		if i < 0 {
			break
		}

		// Extend forwards past (including) the next '\n'.
		j := i + len(slashEtc)
		for ; (j < len(s)) && (s[j] != '\n'); j++ {
		}
		if j < len(s) {
			j++
		}

		// Extend backwards up to (but excluding) the previous '\n'.
		for ; (i > 0) && (s[i-1] != '\n'); i-- {
		}

		// Collapse multiple '\n's.
		if (i >= 2) && (s[i-2] == '\n') {
			for ; (j < len(s)) && (s[j] == '\n'); j++ {
			}
		}

		buf.writes(s[:i])
		s = s[j:]
	}
	buf.writes(s)
}

func patchCFunc(buf *buffer, src string, patches [][2]string) error {
	srcFuncNameParenNewline := patches[0][0]
	buf.writes("// Specializes ")
	buf.writes(srcFuncNameParenNewline[:len(srcFuncNameParenNewline)-2])
	buf.writes(".\n")

loop:
	for src != "" {
		line := ""
		if i := strings.IndexByte(src, '\n'); i < 0 {
			return fmt.Errorf("internal error: patchCFunc failed to find '\n'")
		} else {
			line, src = src[:i+1], src[i+1:]
		}

		for _, patch := range patches {
			if line != patch[0] {
				continue
			} else if !strings.Contains(line, "¡ BEGIN") {
				buf.writes(patch[1])
				continue loop
			}

			// Extend forwards past (including) the next '\n'.
			j := strings.Index(src, "¡ END  ")
			if j < 0 {
				return fmt.Errorf("internal error: patchCFunc failed to find \"¡ END\"")
			}
			for ; (j < len(src)) && (src[j] != '\n'); j++ {
			}
			if j < len(src) {
				j++
			}
			src = src[j:]

			buf.writes(patch[1])
			continue loop
		}

		if !isCFuncComment(line) {
			buf.writes(line)
		}
	}
	buf.writes("\n")
	return nil
}

func isCFuncComment(s string) bool {
	for ; (len(s) > 0) && (s[0] <= ' '); s = s[1:] {
	}
	return (len(s) >= 2) && (s[0] == '/') && (s[1] == '/')
}
