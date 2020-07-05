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

// print-mpb-powers-of-10.go prints the medium-precision (128 bit mantissa)
// binary (base 2) wuffs_base__private_implementation__powers_of_10 tables.
//
// Usage: go run print-mpb-powers-of-10.go -detail

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
		fmt.Printf("≈ (0x%s ", e, hex)
		if n >= 0 {
			fmt.Printf("<< %4d)", +n)
		} else {
			fmt.Printf(">> %4d)", -n)
		}
	}

	fmt.Println()
	return nil
}
