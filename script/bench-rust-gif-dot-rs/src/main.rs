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

// This program exercises the Rust GIF decoder in https://github.com/Geal/gif.rs
//
// Wuffs doesn't depend on Rust or Nom (a parser combinator library written in
// Rust) per se, but this program gives some comparitive data on how fast a
// Rust GIF decoder could perform.
//
// To run this program, do "cargo run --release > /dev/null" from the parent
// directory (the directory containing the Cargo.toml file). See the eprint
// comment below for why we redirect to /dev/null.

extern crate gif;

use std::time::Instant;

// These constants are hard-coded to the harvesters.gif test image. As of
// 2018-01-27, the https://github.com/Geal/gif.rs library's src/lib.rs' Decoder
// implementation is incomplete, and doesn't expose enough API to calculate
// these from the source GIF image.
const WIDTH: usize = 1165;
const HEIGHT: usize = 859;
const BYTES_PER_PIXEL: usize = 3; // Red, green, blue.
const NUM_BYTES: usize = WIDTH * HEIGHT * BYTES_PER_PIXEL;
const COLOR_TABLE_ELEMENT_COUNT: u16 = 256;
const COLOR_TABLE_OFFSET: usize = 13;
const GRAPHIC_BLOCK_OFFSET: usize = 782;
const FIRST_PIXEL: u8 = 1; // Top left pixel's red component is 0x01.
const LAST_PIXEL: u8 = 3; // Bottom right pixel's blue component is 0x03.

fn main() {
    let mut dst = [0; NUM_BYTES];
    let src = include_bytes!("../../../test/testdata/harvesters.gif");

    let start = Instant::now();

    const REPS: u32 = 50;
    for _ in 0..REPS {
        decode(&mut dst[..], src);
    }

    let elapsed = start.elapsed();
    let elapsed_nanos = elapsed.as_secs() * 1_000_000_000 + (elapsed.subsec_nanos() as u64);

    let total_pixels: u64 = ((WIDTH * HEIGHT) as u64) * (REPS as u64);
    let kp_per_s: u64 = total_pixels * 1_000_000 / elapsed_nanos;

    // Use eprint instead of print because the https://github.com/Geal/gif.rs
    // library can print its own debugging messages (this is filed as
    // https://github.com/Geal/gif.rs/issues/4). When running this program, it
    // can be useful to redirect stdout (but not stderr) to /dev/null.
    eprint!(
        "{}.{:03} megapixels/second\n",
        kp_per_s / 1_000,
        kp_per_s % 1_000
    );
}

fn decode(dst: &mut [u8], src: &[u8]) {
    // Set up the hard-coded sanity check, executed below.
    dst[0] = 0;
    dst[NUM_BYTES - 1] = 0;

    let (_, colors) =
        gif::parser::color_table(&src[COLOR_TABLE_OFFSET..], COLOR_TABLE_ELEMENT_COUNT).unwrap();

    let (_, block) = gif::parser::graphic_block(&src[GRAPHIC_BLOCK_OFFSET..]).unwrap();

    let rendering = match block {
        gif::parser::Block::GraphicBlock(_, x) => x,
        _ => panic!("not a graphic block"),
    };

    let (code_size, blocks) = match rendering {
        gif::parser::GraphicRenderingBlock::TableBasedImage(_, x, y) => (x, y),
        _ => panic!("not a table based image"),
    };

    let num_bytes = gif::lzw::decode_lzw(&colors, code_size as usize, blocks, dst).unwrap();

    if num_bytes != NUM_BYTES {
        panic!("wrong num_bytes")
    }

    // A hard-coded sanity check that we decoded the pixel data correctly.
    if (dst[0] != FIRST_PIXEL) || (dst[NUM_BYTES - 1] != LAST_PIXEL) {
        panic!("wrong dst pixels")
    }
}
