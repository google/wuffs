# Roadmap

Short term:

- Do lots of TODOs.
- Implement coroutines, for streaming decoding.
- Finish the GIF decoder.
- Write the PNG decoder.
- Write the JPEG decoder.
- Write the PNG encoder.
- Design an image API that works with all these, including animated GIF.
- Write some example programs, maybe cpng a la cjpeg and cwebp.
- Add compatibility tests, benchmarks, code size comparison.
- Release as open source.

Medium term:

- Write the BMP/ICO decoder.
- Write the TIFF decoder.
- Write the WEBP decoder.
- Write the JPEG encoder.
- Get to 100% compatibility, on a large web crawl corpus, with libpng etc.
- Optimize the codecs; benchmark comparably to libpng etc. Might require SIMD.
- Ensure code size compares favorably to libpng etc.
- Fuzz it, both for crashers and for different output to libpng etc.

Long term:

- Ship with Google Chrome: safer code, smaller binaries, no regressions.
- Ship a version 1.0: stabilize the language and library APIs.
- Write a language spec.
- Generate Go code.
- Generate Rust code.

Very long term:

- Other file formats? Fonts? PDF? Audio? Video?
- Other image operations? Image scaling that's both safe and fast?


---

Updated on August 2017.
