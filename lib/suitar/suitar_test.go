// Copyright 2026 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package suitar

import (
	"archive/tar"
	"bytes"
	"fmt"
	"hash/crc32"
	"io"
	"os"
	"testing"
)

type crcWriter uint32

func (c *crcWriter) Write(b []byte) (int, error) {
	state := uint32(*c)
	state = crc32.Update(state, crc32.IEEETable, b)
	*c = crcWriter(state)
	return len(b), nil
}

func testWriter(tt *testing.T, sparse bool) {
	f, err := os.Open("../../test/data/archive.tar")
	if err != nil {
		tt.Fatalf("os.Open: %v", err)
	}
	defer f.Close()

	// Convert from t (tar, using the standard library) to s (suitar, using
	// this package).
	buf := bytes.Buffer{}
	sWriter := NewWriter(&buf)
	for tReader := tar.NewReader(f); ; {
		tHeader, err := tReader.Next()
		if err == io.EOF {
			break
		} else if err != nil {
			tt.Fatalf("Next: %v", err)
		}

		sHeader := &Header{
			Typeflag: tHeader.Typeflag,
			Name:     tHeader.Name,
			Size:     tHeader.Size,
			Mode:     tHeader.Mode,
			ModTime:  tHeader.ModTime,
		}
		if sparse && (sHeader.Typeflag == TypeReg) {
			sHeader.Typeflag = TypeGNUSparse
		}
		if err := sWriter.WriteHeader(sHeader); err != nil {
			tt.Fatalf("WriteHeader: %v", err)
		}

		dstWriter := (io.Writer)(sWriter)
		if sparse {
			dstWriter = io.Discard
		}
		if _, err := io.Copy(dstWriter, tReader); err != nil {
			tt.Fatalf("io.Copy: %v", err)
		}
	}

	if err := sWriter.Close(); err != nil {
		tt.Fatalf("Close: %v", err)
	}

	got := buf.Bytes()
	wantFilename := "../../test/data/archive"
	if sparse {
		wantFilename += ".sparse.suitar"
	} else {
		wantFilename += ".dense.suitar"
	}
	want, err := os.ReadFile(wantFilename)
	if err != nil {
		tt.Fatalf("os.ReadFile: %v", err)
	}

	if !bytes.Equal(got, want) {
		tt.Fatalf("did not recreate golden test file")
	}
}

func testReader(tt *testing.T, sparse bool, ignore bool) {
	filename, wantTypeflag := "../../test/data/archive.dense.suitar", " T:'0'"
	if sparse {
		filename, wantTypeflag = "../../test/data/archive.sparse.suitar", " T:'S'"
	}

	wantChecksums := []string(nil)
	if ignore {
		wantChecksums = []string{
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
			"C:0x00000000",
		}

	} else if sparse {
		wantChecksums = []string{
			"C:0x00000000",
			"C:0xB69F8E37",
			"C:0x73FF3CAE",
			"C:0xD71F022F",
			"C:0xC446EAB8",
			"C:0x5F228EB9",
			"C:0x7BA7D011",
			"C:0x48792A7F",
		}

	} else {
		wantChecksums = []string{
			"C:0x00000000",
			"C:0xFEDD8F35",
			"C:0x87EE5E05",
			"C:0x703E9270",
			"C:0xC37CB538",
			"C:0x2B0B23B0",
			"C:0xABE507EF",
			"C:0x67FABE9C",
		}
	}

	want := "" +
		wantChecksums[0] + wantTypeflag + " S:0x0000 M:644 MT:0x5E3A5C50 N:artificial/0.bytes\n" +
		wantChecksums[1] + wantTypeflag + " S:0x0355 M:644 MT:0x5F33F6E6 N:github-tags.json\n" +
		wantChecksums[2] + wantTypeflag + " S:0x02B5 M:755 MT:0x608F960B N:hello.sh\n" +
		wantChecksums[3] + wantTypeflag + " S:0x0068 M:644 MT:0x608F954D N:non-ascii/αβ.txt\n" +
		wantChecksums[4] + wantTypeflag + " S:0x0097 M:644 MT:0x608F96C7 N:non-ascii/😻.txt\n" +
		wantChecksums[5] + wantTypeflag + " S:0x00D0 M:644 MT:0x5E3A5C50 N:pjw-thumbnail.png\n" +
		wantChecksums[6] + wantTypeflag + " S:0x03AE M:644 MT:0x5E3A5C50 N:romeo.txt\n" +
		wantChecksums[7] + wantTypeflag + " S:0x022E M:644 MT:0x5E3A5C50 N:romeo.txt.gz\n" +
		""

	f, err := os.Open(filename)
	if err != nil {
		tt.Fatalf("os.Open: %v", err)
	}
	defer f.Close()

	buf := bytes.Buffer{}
	for r := NewReader(f); ; {
		h, err := r.Next()
		if err == io.EOF {
			break
		} else if err != nil {
			tt.Fatalf("Next: %v", err)
		}

		checksum := crcWriter(0)
		if !ignore {
			if _, err := io.Copy(&checksum, r); err != nil {
				tt.Fatalf("io.Copy: %v", err)
			}
		}

		fmt.Fprintf(&buf, "C:0x%08X T:'%c' S:0x%04X M:%3o MT:0x%08X N:%s\n",
			checksum, h.Typeflag, h.Size, h.Mode, h.ModTime.Unix(), h.Name)
	}

	if got := buf.String(); got != want {
		tt.Fatalf("\ngot:\n%s\nwant:\n%s", got, want)
	}
}

func TestWriterDense(tt *testing.T)        { testWriter(tt, false) }
func TestWriterSparse(tt *testing.T)       { testWriter(tt, true) }
func TestReaderDenseCheck(tt *testing.T)   { testReader(tt, false, false) }
func TestReaderDenseIgnore(tt *testing.T)  { testReader(tt, false, true) }
func TestReaderSparseCheck(tt *testing.T)  { testReader(tt, true, false) }
func TestReaderSparseIgnore(tt *testing.T) { testReader(tt, true, true) }
