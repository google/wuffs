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
// wuffs bench -mimic -focus=wuffs_gif_decode_1000k,mimic_gif_decode_1000k std/gif
//
// The "1000k" is because the test image (harvesters.gif) has approximately 1
// million pixels.
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

// These constants are hard-coded to the harvesters.gif test image.
const WIDTH: usize = 1165;
const HEIGHT: usize = 859;
const BYTES_PER_PIXEL: usize = 1; // Palette index.
const NUM_BYTES: usize = WIDTH * HEIGHT * BYTES_PER_PIXEL;
const FIRST_PIXEL: u8 = 0; // Top left pixel's palette index is 0x00.
const LAST_PIXEL: u8 = 1; // Bottom right pixel's palette index is 0x01.

fn main() {
    let mut dst = [0; NUM_BYTES];
    let src = include_bytes!("../../../test/data/harvesters.gif");

    let start = Instant::now();

    const REPS: u32 = 50;
    for _ in 0..REPS {
        decode(&mut dst[..], src);
    }

    let elapsed = start.elapsed();
    let elapsed_nanos = elapsed.as_secs() * 1_000_000_000 + (elapsed.subsec_nanos() as u64);

    let total_pixels: u64 = ((WIDTH * HEIGHT) as u64) * (REPS as u64);
    let kp_per_s: u64 = total_pixels * 1_000_000 / elapsed_nanos;

    print!(
        "gif     {:3}.{:03} megapixels/second  github.com/PistonDevelopers/image-gif\n",
        kp_per_s / 1_000,
        kp_per_s % 1_000
    );
}

fn decode(dst: &mut [u8], src: &[u8]) {
    // Set up the hard-coded sanity check, executed below.
    dst[0] = 0xFE;
    dst[NUM_BYTES - 1] = 0xFE;

    let mut reader = gif::Decoder::new(src).read_info().unwrap();
    reader.next_frame_info().unwrap().unwrap();

    if reader.buffer_size() != NUM_BYTES {
        panic!("wrong num_bytes")
    }

    reader.read_into_buffer(dst).unwrap();

    // A hard-coded sanity check that we decoded the pixel data correctly.
    if (dst[0] != FIRST_PIXEL) || (dst[NUM_BYTES - 1] != LAST_PIXEL) {
        panic!("wrong dst pixels")
    }
}
