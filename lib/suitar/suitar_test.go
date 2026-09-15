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
	"errors"
	"fmt"
	"hash/crc32"
	"io"
	"os"
	"reflect"
	"testing"
	"time"
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

// cappedWriter returns an error once the total number of bytes written would
// exceed limit, so that a buggy Writer (which would otherwise loop forever)
// instead fails the surrounding test.
type cappedWriter struct {
	buf   bytes.Buffer
	limit int
}

func (w *cappedWriter) Write(b []byte) (int, error) {
	if w.buf.Len()+len(b) > w.limit {
		return 0, errCappedWriter
	}
	return w.buf.Write(b)
}

var errCappedWriter = errors.New("suitar_test: capped writer")

// writeOneFileChunked returns a SUITAR archive holding a single file whose
// contents are size bytes long, written in chunks of chunk bytes (or all at
// once if chunk is non-positive).
func writeOneFileChunked(tt *testing.T, size int, chunk int) []byte {
	tt.Helper()

	cw := &cappedWriter{limit: 0x10000}
	w := NewWriter(cw)
	err := w.WriteHeader(&Header{
		Typeflag: TypeReg,
		Name:     "file.bin",
		Size:     int64(size),
		Mode:     Mode644,
		ModTime:  time.Unix(12345678, 0),
	})
	if err != nil {
		tt.Fatalf("size=%d chunk=%d: WriteHeader: %v", size, chunk, err)
	}

	contents := make([]byte, size)
	for i := range contents {
		contents[i] = byte(i)
	}

	if chunk <= 0 {
		if _, err := w.Write(contents); err != nil {
			tt.Fatalf("size=%d chunk=%d: Write: %v", size, chunk, err)
		}
	} else {
		for i := 0; i < size; i += chunk {
			j := min(i+chunk, size)
			if _, err := w.Write(contents[i:j]); err != nil {
				tt.Fatalf("size=%d chunk=%d: Write[%d:%d]: %v", size, chunk, i, j, err)
			}
		}
	}

	if err := w.Close(); err != nil {
		tt.Fatalf("size=%d chunk=%d: Close: %v", size, chunk, err)
	}
	return cw.buf.Bytes()
}

// TestWriterChunked checks that how the file contents are split up over Write
// calls doesn't matter, as long as the total is Header.Size bytes long.
func TestWriterChunked(tt *testing.T) {
	for _, size := range []int{0, 1, 300, 511, 512, 513, 700, 1000, 1024, 5000} {
		want := writeOneFileChunked(tt, size, 0)

		for _, chunk := range []int{1, 100, 300, 511, 512, 700, 1024} {
			if got := writeOneFileChunked(tt, size, chunk); !bytes.Equal(got, want) {
				tt.Fatalf("size=%d chunk=%d: chunk size changed the archive bytes", size, chunk)
			}
		}

		r := NewReader(bytes.NewReader(want))
		h, err := r.Next()
		if err != nil {
			tt.Fatalf("size=%d: Next: %v", size, err)
		} else if (h.Typeflag != TypeReg) || (h.Size != int64(size)) {
			tt.Fatalf("size=%d: got T:'%c' S:%d", size, h.Typeflag, h.Size)
		}
		got, err := io.ReadAll(r)
		if err != nil {
			tt.Fatalf("size=%d: ReadAll: %v", size, err)
		} else if len(got) != size {
			tt.Fatalf("size=%d: read back %d bytes", size, len(got))
		} else {
			for i, c := range got {
				if c != byte(i) {
					tt.Fatalf("size=%d: contents differ at %d", size, i)
					break
				}
			}
		}
		if _, err := r.Next(); err != io.EOF {
			tt.Fatalf("size=%d: Next after the last entry: %v", size, err)
		}
	}
}

// TestWriteNonRegular checks that entries (including TypeDir entries which
// have no contents and TypeGNUSparse entries which have all-NUL contents) can
// be written and read back.
func TestWriteNonRegular(tt *testing.T) {
	headers := []Header{{
		Typeflag: TypeDir,
		Name:     "a/b",
		Mode:     Mode755,
		ModTime:  time.Unix(12345601, 0),
	}, {
		Typeflag: TypeReg,
		Name:     "a/b/c.txt",
		Size:     5,
		Mode:     Mode644,
		ModTime:  time.Unix(12345602, 0),
	}, {
		Typeflag: TypeGNUSparse,
		Name:     "sparse0.bin",
		Size:     3,
		Mode:     Mode644,
		ModTime:  time.Unix(123000, 0),
	}, {
		Typeflag: TypeGNUSparse,
		Name:     "sparse1.bin",
		Size:     3,
		Mode:     Mode644,
		ModTime:  time.Unix(123001, 0),
	}, {
		Typeflag: TypeGNUSparse,
		Name:     "sparse3.bin",
		Size:     3,
		Mode:     Mode644,
		ModTime:  time.Unix(123002, 0),
	}}

	contents := map[string]string{
		"a/b/c.txt":   "hello",
		"sparse0.bin": "\x00\x00\x00",
		"sparse1.bin": "\x00\x00\x00",
		"sparse3.bin": "\x00\x00\x00",
	}

	buf := bytes.Buffer{}
	w := NewWriter(&buf)
	for _, h := range headers {
		if err := w.WriteHeader(&h); err != nil {
			tt.Fatalf("WriteHeader(%q): %v", h.Name, err)
		}

		content, ok := contents[h.Name]
		if !ok {
			continue
		} else if h.Name == "sparse0.bin" {
			// It's OK, for TypeGNUSparse, to Write no bytes.
			continue
		} else if h.Name == "sparse1.bin" {
			// It's OK, for TypeGNUSparse, to Write some but not all of the
			// Size bytes, provided that what you're writing are NUL bytes.
			content = content[:1]
		} else if h.Name == "sparse3.bin" {
			// No-op, going on to Write all 3 explicit NUL bytes.
		}

		if _, err := w.Write([]byte(content)); err != nil {
			tt.Fatalf("Write(%q): %v", h.Name, err)
		}
	}
	if err := w.Close(); err != nil {
		tt.Fatalf("Close: %v", err)
	}

	r := NewReader(&buf)
	for _, hWant := range headers {
		hGot, err := r.Next()
		if err != nil {
			tt.Fatalf("Next: %v", err)
		} else if !reflect.DeepEqual(hGot, hWant) {
			tt.Fatalf("got vs want\n%#v\n%#v", hGot, hWant)
		}

		if readAll, err := io.ReadAll(r); err != nil {
			tt.Fatalf("ReadAll: %v", err)
		} else if rGot, rWant := string(readAll), contents[hGot.Name]; rGot != rWant {
			tt.Fatalf("N:%s: got %q, want %q", hGot.Name, rGot, rWant)
		}
	}
	if _, err := r.Next(); err != io.EOF {
		tt.Fatalf("Next after the last entry: %v", err)
	}
}

func TestWriteANonNUL(tt *testing.T) {
	buf := bytes.Buffer{}
	w := NewWriter(&buf)

	if err := w.WriteHeader(&Header{
		Typeflag: TypeGNUSparse,
		Name:     "example.dat",
		Size:     5,
		Mode:     Mode644,
		ModTime:  time.Unix(0, 0),
	}); err != nil {
		tt.Fatalf("WriteHeader: %v", err)
	}

	if _, err := w.Write([]byte("Lorem")); err != errWriteANonNul {
		tt.Fatalf("Write: got %v, want %v", err, errWriteANonNul)
	}
}

func testWriteTooMuch(tt *testing.T, typeflag byte) {
	for i := 8; i <= 12; i++ {
		buf := bytes.Buffer{}
		w := NewWriter(&buf)

		if err := w.WriteHeader(&Header{
			Typeflag: typeflag,
			Name:     "example.dat",
			Size:     10,
			Mode:     Mode644,
			ModTime:  time.Unix(0, 0),
		}); err != nil {
			tt.Fatalf("WriteHeader: %v", err)
		}

		errWant := error(nil)
		if i > 10 {
			errWant = errHeaderSize
		}

		if _, errGot := w.Write(make([]byte, i)); errGot != errWant {
			tt.Fatalf("i=%d: Write: got %v, want %v", i, errGot, errWant)
		}
	}
}

func TestWriteTooMuchRegular(tt *testing.T) { testWriteTooMuch(tt, TypeReg) }
func TestWriteTooMuchSparse(tt *testing.T)  { testWriteTooMuch(tt, TypeGNUSparse) }
