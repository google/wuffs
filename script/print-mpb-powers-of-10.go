// Copyright 2020 The Wuffs Authors.
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

// +build ignore

package main

// print-mpb-powers-of-10.go prints the medium-precision (128-bit mantissa)
// binary (base-2) wuffs_base__private_implementation__powers_of_10 tables.
//
// When the approximation to (10 ** N) is not exact, the mantissa is truncated,
// not rounded to nearest. The base-2 exponent is chosen so that the mantissa's
// most signficant bit (bit 127) is set.
//
// The final uint32_t entry in each row-of-5 is biased by 1214, also known as
// 0x04BE, which simplifies its use in f64conv-submodule.c.
//
// Usage: go run print-mpb-powers-of-10.go -detail
//
// With -detail set, its output should include:
//
// 0xF7604B57, 0x014BB630, 0xFE98746D, 0x84A57695, 0x0004,
//    // 1e-326 ≈ (0x84A57695FE98746D014BB630F7604B57 >> 1210)
//
// 0x35385E2D, 0x419EA3BD, 0x7E3E9188, 0xA5CED43B, 0x0007,
//    // 1e-325 ≈ (0xA5CED43B7E3E9188419EA3BD35385E2D >> 1207)
//
// ...
//
// 0x0A3D70A3, 0x3D70A3D7, 0x70A3D70A, 0xA3D70A3D, 0x0438,
//    // 1e-2   ≈ (0xA3D70A3D70A3D70A3D70A3D70A3D70A3 >>  134)
//
// 0xCCCCCCCC, 0xCCCCCCCC, 0xCCCCCCCC, 0xCCCCCCCC, 0x043B,
//    // 1e-1   ≈ (0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC >>  131)
//
// 0x00000000, 0x00000000, 0x00000000, 0x80000000, 0x043F,
//    // 1e0    ≈ (0x80000000000000000000000000000000 >>  127)
//
// 0x00000000, 0x00000000, 0x00000000, 0xA0000000, 0x0442,
//    // 1e1    ≈ (0xA0000000000000000000000000000000 >>  124)
//
// 0x00000000, 0x00000000, 0x00000000, 0xC8000000, 0x0445,
//    // 1e2    ≈ (0xC8000000000000000000000000000000 >>  121)
//
// ...
//
// 0x51E513DA, 0x2CD2CC65, 0x35D63F73, 0xB201833B, 0x0841,
//    // 1e309  ≈ (0xB201833B35D63F732CD2CC6551E513DA <<  899)
//
// 0xA65E58D1, 0xF8077F7E, 0x034BCF4F, 0xDE81E40A, 0x0844,
//    // 1e310  ≈ (0xDE81E40A034BCF4FF8077F7EA65E58D1 <<  902)

import (
	"flag"
	"fmt"
	"math/big"
	"os"
)

var (
	detail = flag.Bool("detail", false, "whether to print detailed comments")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()

	const count = 1 + (+310 - -326)
	fmt.Printf("static const uint32_t "+
		"wuffs_base__private_implementation__powers_of_10[%d] = {\n", 5*count)
	for e := -326; e <= +310; e++ {
		if err := do(e); err != nil {
			return err
		}
	}
	fmt.Printf("};\n\n")

	return nil
}

var (
	one    = big.NewInt(1)
	ten    = big.NewInt(10)
	two128 = big.NewInt(0).Lsh(one, 128)
)

// N is large enough so that (1<<N) is easily bigger than 1e310.
const N = 2048

// 1214 is 1023 + 191. 1023 is the bias for IEEE 754 double-precision floating
// point. 191 is ((3 * 64) - 1) and we work with multiples-of-64-bit mantissas.
const bias = 1214

func do(e int) error {
	z := big.NewInt(0).Lsh(one, N)
	if e >= 0 {
		exp := big.NewInt(0).Exp(ten, big.NewInt(int64(+e)), nil)
		z.Mul(z, exp)
	} else {
		exp := big.NewInt(0).Exp(ten, big.NewInt(int64(-e)), nil)
		z.Div(z, exp)
	}

	n := int32(-N)
	for z.Cmp(two128) >= 0 {
		z.Rsh(z, 1)
		n++
	}
	hex := fmt.Sprintf("%X", z)
	if len(hex) != 32 {
		return fmt.Errorf("invalid hexadecimal representation %q", hex)
	}

	fmt.Printf("    0x%s, 0x%s, 0x%s, 0x%s, 0x%04X,  // 1e%-04d",
		hex[24:], hex[16:24], hex[8:16], hex[:8], uint32(n)+bias, e)
	if *detail {
		fmt.Printf(" ≈ (0x%s ", hex)
		if n >= 0 {
			fmt.Printf("<< %4d)", +n)
		} else {
			fmt.Printf(">> %4d)", -n)
		}
	}

	fmt.Println()
	return nil
}
