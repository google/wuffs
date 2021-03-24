// Copyright 2021 The Wuffs Authors.
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

// ----------------

// This program exercises the Rust PNG decoder at
// https://github.com/image-rs/image-png
// which is the top result for https://crates.io/search?q=png&sort=downloads
//
// Wuffs' C code doesn't depend on Rust per se, but this program gives some
// performance data for specific Rust PNG implementations. The equivalent Wuffs
// benchmarks (on the same test images) are run via:
//
// wuffs bench std/png
//
// To run this program, do "cargo run --release" from the parent directory (the
// directory containing the Cargo.toml file).

extern crate png;
extern crate rustc_version_runtime;

use std::time::Instant;

const ITERSCALE: u64 = 50;
const REPS: u64 = 5;

fn main() {
    let version = rustc_version_runtime::version();
    print!(
        "# Rust {}.{}.{}\n",
        version.major, version.minor, version.patch,
    );
    print!("#\n");
    print!("# The output format, including the \"Benchmark\" prefixes, is compatible with the\n");
    print!("# https://godoc.org/golang.org/x/perf/cmd/benchstat tool. To install it, first\n");
    print!("# install Go, then run \"go get golang.org/x/perf/cmd/benchstat\".\n");

    let mut dst = vec![0u8; 64 * 1024 * 1024];

    // The various magic constants below are copied from test/c/std/png.c
    for i in 0..(1 + REPS) {
        bench(
            "19k_8bpp",
            &mut dst[..],
            include_bytes!("../../../test/data/bricks-gray.no-ancillary.png"),
            i == 0,        // warm_up
            160 * 120 * 1, // want_num_bytes = 19_200
            50,            // iters_unscaled
        );

        bench(
            "40k_24bpp",
            &mut dst[..],
            include_bytes!("../../../test/data/hat.png"),
            i == 0,       // warm_up
            90 * 112 * 4, // want_num_bytes = 40_320
            30,           // iters_unscaled
        );

        bench(
            "77k_8bpp",
            &mut dst[..],
            include_bytes!("../../../test/data/bricks-dither.png"),
            i == 0,        // warm_up
            160 * 120 * 4, // want_num_bytes = 76_800
            30,            // iters_unscaled
        );

        bench(
            "552k_32bpp_verify_checksum",
            &mut dst[..],
            include_bytes!("../../../test/data/hibiscus.primitive.png"),
            i == 0,        // warm_up
            312 * 442 * 4, // want_num_bytes = 551_616
            4,             // iters_unscaled
        );

        bench(
            "4002k_24bpp",
            &mut dst[..],
            include_bytes!("../../../test/data/harvesters.png"),
            i == 0,         // warm_up
            1165 * 859 * 4, // want_num_bytes = 4_002_940
            1,              // iters_unscaled
        );
    }
}

fn bench(
    name: &str,          // Benchmark name.
    dst: &mut [u8],      // Destination buffer.
    src: &[u8],          // Source data.
    warm_up: bool,       // Whether this is a warm up rep.
    want_num_bytes: u64, // Expected num_bytes per iteration.
    iters_unscaled: u64, // Base number of iterations.
) {
    let iters = iters_unscaled * ITERSCALE;
    let mut total_num_bytes = 0u64;

    let start = Instant::now();
    for _ in 0..iters {
        let n = decode(&mut dst[..], src);
        if n != want_num_bytes {
            panic!("num_bytes: got {}, want {}", n, want_num_bytes);
        }
        total_num_bytes += n;
    }
    let elapsed = start.elapsed();

    let elapsed_nanos = (elapsed.as_secs() * 1_000_000_000) + (elapsed.subsec_nanos() as u64);
    let kb_per_s: u64 = total_num_bytes * 1_000_000 / elapsed_nanos;

    if warm_up {
        return;
    }

    print!(
        "Benchmarkrust_png_decode_image_{:16}   {:8}   {:12} ns/op   {:3}.{:03} MB/s\n",
        name,
        iters,
        elapsed_nanos / iters,
        kb_per_s / 1_000,
        kb_per_s % 1_000
    );
}

// decode returns the number of bytes processed.
fn decode(dst: &mut [u8], src: &[u8]) -> u64 {
    let decoder = png::Decoder::new(src);
    let (info, mut reader) = decoder.read_info().unwrap();
    let num_bytes = info.buffer_size() as u64;
    reader.next_frame(dst).unwrap();
    if info.color_type == png::ColorType::RGB {
        // If the PNG image is RGB (not RGBA) then Rust's png crate will decode
        // to 3 bytes per pixel. Wuffs' std/png benchmarks decode to 4 bytes
        // per pixel (and in BGRA order, not RGB or RGBA). We'll hand-wave the
        // difference away and say that we decoded 33% more pixels than we did.
        return (num_bytes / 3) * 4;
    }
    num_bytes
}
