// Copyright 2018 The Wuffs Authors.
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

// This program exercises the Rust GIF decoder at
// https://github.com/PistonDevelopers/image-gif
//
// Wuffs' C code doesn't depend on Rust per se, but this program gives some
// performance data for specific Rust GIF implementations. The equivalent Wuffs
// benchmarks (on the same test image) are run via:
//
// wuffs bench std/gif
//
// The Wuffs benchmark reports megabytes per second. This program reports
// megapixels per second. The two concepts should be equivalent, since GIF
// images' pixel data are always 1 byte per pixel indices into a color palette.
//
// To run this program, do "cargo run --release" from the parent directory (the
// directory containing the Cargo.toml file).

// TODO: unify this program, bench-rust-gif-dot-rs, with bench-rust-gif, the
// other Rust GIF benchmark program. They are two separate programs because
// both libraries want to be named "gif", and the cargo package manager cannot
// handle duplicate names (https://github.com/rust-lang/cargo/issues/1311).

extern crate gif;

use std::time::Instant;

const ITERSCALE: u64 = 10;
const REPS: u64 = 5;

fn main() {
    let mut dst = vec![0u8; 64 * 1024 * 1024];

    for _ in 0..REPS {
        bench(
            "1k_bw",
            &mut dst[..32 * 32],
            include_bytes!("../../../test/data/pjw-thumbnail.gif"),
            2000, // iters_unscaled
            32, // width
            32, // height
            1, // first_pixel
            1, // last_pixel
        );

        bench(
            "1k_color",
            &mut dst[..36 * 28],
            include_bytes!("../../../test/data/hippopotamus.regular.gif"),
            1000, // iters_unscaled
            36, // width
            28, // height
            82, // first_pixel
            225, // last_pixel
        );

        bench(
            "10k_indexed",
            &mut dst[..90 * 112],
            include_bytes!("../../../test/data/hat.gif"),
            100, // iters_unscaled
            90, // width
            112, // height
            0, // first_pixel
            0, // last_pixel
        );

        bench(
            "20k",
            &mut dst[..160 * 120],
            include_bytes!("../../../test/data/bricks-gray.gif"),
            50, // iters_unscaled
            160, // width
            120, // height
            85, // first_pixel
            6, // last_pixel
        );

        bench(
            "100k_artificial",
            &mut dst[..312 * 442],
            include_bytes!("../../../test/data/hibiscus.primitive.gif"),
            15, // iters_unscaled
            312, // width
            442, // height
            102, // first_pixel
            102, // last_pixel
        );

        bench(
            "100k_realistic",
            &mut dst[..312 * 442],
            include_bytes!("../../../test/data/hibiscus.regular.gif"),
            10, // iters_unscaled
            312, // width
            442, // height
            0, // first_pixel
            0, // last_pixel
        );

        bench(
            "1000k",
            &mut dst[..1165 * 859],
            include_bytes!("../../../test/data/harvesters.gif"),
            1, // iters_unscaled
            1165, // width
            859, // height
            0, // first_pixel
            1, // last_pixel
        );
    }
}

fn bench(
    name: &str, // Benchmark name.
    dst: &mut [u8], // Destination buffer.
    src: &[u8], // Source data.
    iters_unscaled: u64, // Base number of iterations.
    width: u64, // Image width in pixels.
    height: u64, // Image height in pixels.
    first_pixel: u8, // Top left pixel's palette index.
    last_pixel: u8, // Bottom right pixel's palette index.
) {
    let start = Instant::now();

    let iters = iters_unscaled * ITERSCALE;
    for _ in 0..iters {
        decode(&mut dst[..], src, width, height, first_pixel, last_pixel);
    }

    let elapsed = start.elapsed();
    let elapsed_nanos = elapsed.as_secs() * 1_000_000_000 + (elapsed.subsec_nanos() as u64);

    let total_pixels: u64 = iters * width * height;
    let kp_per_s: u64 = total_pixels * 1_000_000 / elapsed_nanos;

    print!(
        "Benchmarkrust_gif_decode_{:16}   {:8}   {:12} ns/op   {:3}.{:03} MB/s\n",
        name,
        iters,
        elapsed_nanos / iters,
        kp_per_s / 1_000,
        kp_per_s % 1_000
    );
}

fn decode(dst: &mut [u8], src: &[u8], width: u64, height: u64, first_pixel: u8, last_pixel: u8) {
    // GIF images are 1 byte per pixel.
    let num_bytes = (width * height * 1) as usize;

    // Set up the hard-coded sanity check, executed below.
    dst[0] = 0xFE;
    dst[num_bytes - 1] = 0xFE;

    let mut reader = gif::Decoder::new(src).read_info().unwrap();
    reader.next_frame_info().unwrap().unwrap();

    if reader.buffer_size() != num_bytes {
        panic!("wrong num_bytes")
    }

    reader.read_into_buffer(dst).unwrap();

    // A hard-coded sanity check that we decoded the pixel data correctly.
    if (dst[0] != first_pixel) || (dst[num_bytes - 1] != last_pixel) {
        panic!("wrong dst pixels")
    }
}
