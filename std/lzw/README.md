# LZW (Lempel Ziv Welch) Compression

LZW is a general purpose compression algorithm, not specific to GIF, PDF, TIFF
or even to graphics. In practice, there are two incompatible implementations,
LSB (Least Significant Bits) and MSB (Most Significant Bits) first.

The GIF format uses LSB first. The literal width can vary between 2 and 8 bits,
inclusive. There is no EarlyChange option.

The PDF and TIFF formats use MSB first. The literal width is fixed at 8 bits.
There is an EarlyChange option that PDF sometimes uses (see Table 3.7 in the
[PDF 1.4
spec](https://www.adobe.com/content/dam/acom/en/devnet/pdf/pdfs/pdf_reference_archives/PDFReference.pdf))
and TIFF always uses.

TODO: refactor this README.md file and std/gif's one.
