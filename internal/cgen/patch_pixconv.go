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

	bgrxHv11, err := extractCFunc(src, ""+
		"static void  //\n"+
		"wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__hv11(\n")
	if err != nil {
		return err
	}

	generalTriangleFilter, err := extractCFunc(src, ""+
		"static void  //\n"+
		"wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter(\n")
	if err != nil {
		return err
	}

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

	// ----

	const dstIterEtcSansX = "" +
		"size_t dst_stride = dst->private_impl.planes[0].stride;\n" +
		"uint8_t* dst_iter =\n" +
		"    dst->private_impl.planes[0].ptr + (dst_stride * ((size_t)y));\n"
	const dstIterWithX = "" +
		"size_t dst_stride = dst->private_impl.planes[0].stride;\n" +
		"uint8_t* dst_iter =\n" +
		"    dst->private_impl.planes[0].ptr + (dst_stride * ((size_t)y)) + (4 * ((size_t)x));\n"
	const dstIterPlusEq4 = "" +
		"dst_iter += 4;\n"
	const setColorU32At = "" +
		"wuffs_base__poke_u32le__no_bounds_check(dst_iter, color);\n"

	// ----

	generalHv11Patches := []struct {
		newFuncNameLine string
		patchMap        map[string]string
	}{{

		"wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__hv11(\n",
		map[string]string{},
	}}

	for _, p := range generalHv11Patches {
		const oldFuncNameLine = "wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__hv11(\n"
		p.patchMap[oldFuncNameLine] = p.newFuncNameLine
		if err := patchCFunc(buf, bgrxHv11, p.newFuncNameLine, p.patchMap); err != nil {
			return err
		}
	}

	// ----

	generalTriangleFilterPatches := []struct {
		newFuncNameLine string
		patchMap        map[string]string
	}{{

		"wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__triangle_filter(\n",
		map[string]string{
			"// ¡ dst_iter = etc\n":         dstIterWithX,
			"// ¡ dst_iter += 4\n":          dstIterPlusEq4,
			"// ¡ BEGIN set_color_u32_at\n": setColorU32At,
		},
	}, {

		"wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__triangle_filter(\n",
		map[string]string{
			"// ¡ dst_iter = etc\n":         dstIterWithX,
			"// ¡ dst_iter += 4\n":          dstIterPlusEq4,
			"// ¡ BEGIN set_color_u32_at\n": setColorU32At,
		},
	}}

	for _, p := range generalTriangleFilterPatches {
		const oldFuncNameLine = "wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter(\n"
		p.patchMap[oldFuncNameLine] = p.newFuncNameLine
		if err := patchCFunc(buf, generalTriangleFilter, p.newFuncNameLine, p.patchMap); err != nil {
			return err
		}
	}

	// ----

	generalBoxFilterPatches := []struct {
		newFuncNameLine string
		patchMap        map[string]string
	}{{

		"wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__box_filter(\n",
		map[string]string{
			"// ¡ dst_iter = etc\n":         dstIterEtcSansX,
			"// ¡ dst_iter += 4\n":          dstIterPlusEq4,
			"// ¡ BEGIN set_color_u32_at\n": setColorU32At,
		},
	}, {

		"wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__box_filter(\n",
		map[string]string{
			"// ¡ dst_iter = etc\n":         dstIterEtcSansX,
			"// ¡ dst_iter += 4\n":          dstIterPlusEq4,
			"// ¡ BEGIN set_color_u32_at\n": setColorU32At,
		},
	}}

	for _, p := range generalBoxFilterPatches {
		const oldFuncNameLine = "wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(\n"
		p.patchMap[oldFuncNameLine] = p.newFuncNameLine
		if err := patchCFunc(buf, generalBoxFilter, p.newFuncNameLine, p.patchMap); err != nil {
			return err
		}
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

func patchCFunc(buf *buffer, src string, newFuncNameLine string, patchMap map[string]string) error {
	if strings.Contains(newFuncNameLine, "swizzle_ycc__rgbx__") {
		patchMap["wuffs_base__color_ycc__as__color_u32(  //\n"] =
			"wuffs_base__color_ycc__as__color_u32_abgr(\n"
	}

	buf.writes("// Generated by internal/cgen/patch_pixconv.go\n")
	for src != "" {
		line := ""
		if i := strings.IndexByte(src, '\n'); i < 0 {
			return fmt.Errorf("internal error: patchCFunc failed to find '\n'")
		} else {
			line, src = src[:i+1], src[i+1:]
		}

		key, indent := line, ""
		for i := 0; ; i++ {
			if i == len(line) {
				key, indent = "", line
				break
			} else if line[i] != ' ' {
				key, indent = line[i:], line[:i]
				break
			}
		}
		val, ok := patchMap[key]
		if !ok {
			if !isCFuncComment(line) {
				buf.writes(line)
			}
			continue
		}
		indentCFunc(buf, indent, val)
		if !strings.Contains(key, "¡ BEGIN") {
			continue
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
	}
	buf.writes("\n")
	return nil
}

func isCFuncComment(s string) bool {
	for ; (len(s) > 0) && (s[0] <= ' '); s = s[1:] {
	}
	return (len(s) >= 2) && (s[0] == '/') && (s[1] == '/')
}

func indentCFunc(buf *buffer, indent string, s string) {
	for s != "" {
		line := ""
		if i := strings.IndexByte(s, '\n'); i < 0 {
			line, s = s, ""
		} else {
			line, s = s[:i+1], s[i+1:]
		}
		buf.writes(indent)
		buf.writes(line)
	}
}
