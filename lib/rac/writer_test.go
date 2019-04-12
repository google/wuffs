// Copyright 2019 The Wuffs Authors.
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

package rac

import (
	"bytes"
	"encoding/hex"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strings"
	"testing"
)

const bytesPerHexDumpLine = 79

const fakeCodec = Codec(0xEE)

// Note that these exact bytes depends on the zlib encoder's algorithm, but
// there is more than one valid zlib encoding of any given input. This "compare
// to golden output" test is admittedly brittle, as the standard library's zlib
// package's output isn't necessarily stable across Go releases.

const writerWantEmpty = "" +
	"00000000  72 c3 63 01 30 14 00 ff  00 00 00 00 00 00 00 ee  |r.c.0...........|\n" +
	"00000010  20 00 00 00 00 00 01 ff  20 00 00 00 00 00 01 01  | ....... .......|\n"

const writerWantILAEnd = "" +
	"00000000  72 c3 63 00 52 72 72 53  73 41 61 61 42 62 62 62  |r.c.RrrSsAaaBbbb|\n" +
	"00000010  43 63 63 63 63 63 63 63  63 63 31 32 72 c3 63 05  |Cccccccccc12r.c.|\n" +
	"00000020  1d 45 00 ff 00 00 00 00  00 00 00 ff 00 00 00 00  |.E..............|\n" +
	"00000030  00 00 00 ff 11 00 00 00  00 00 00 ff 33 00 00 00  |............3...|\n" +
	"00000040  00 00 00 01 77 00 00 00  00 00 00 ee 04 00 00 00  |....w...........|\n" +
	"00000050  00 00 01 ff 07 00 00 00  00 00 01 ff 09 00 00 00  |................|\n" +
	"00000060  00 00 01 ff 0c 00 00 00  00 00 01 00 10 00 00 00  |................|\n" +
	"00000070  00 00 01 00 7c 00 00 00  00 00 01 05              |....|.......|\n"

const writerWantILAEndCPageSize8 = "" +
	"00000000  72 c3 63 00 52 72 72 00  53 73 41 61 61 00 00 00  |r.c.Rrr.SsAaa...|\n" +
	"00000010  42 62 62 62 43 63 63 63  63 63 63 63 63 63 31 32  |BbbbCccccccccc12|\n" +
	"00000020  72 c3 63 05 90 5e 00 ff  00 00 00 00 00 00 00 ff  |r.c..^..........|\n" +
	"00000030  00 00 00 00 00 00 00 ff  11 00 00 00 00 00 00 ff  |................|\n" +
	"00000040  33 00 00 00 00 00 00 01  77 00 00 00 00 00 00 ee  |3.......w.......|\n" +
	"00000050  04 00 00 00 00 00 01 ff  08 00 00 00 00 00 01 ff  |................|\n" +
	"00000060  0a 00 00 00 00 00 01 ff  10 00 00 00 00 00 01 00  |................|\n" +
	"00000070  14 00 00 00 00 00 01 00  80 00 00 00 00 00 01 05  |................|\n"

const writerWantILAStart = "" +
	"00000000  72 c3 63 05 bc dc 00 ff  00 00 00 00 00 00 00 ff  |r.c.............|\n" +
	"00000010  00 00 00 00 00 00 00 ff  11 00 00 00 00 00 00 ff  |................|\n" +
	"00000020  33 00 00 00 00 00 00 01  77 00 00 00 00 00 00 ee  |3.......w.......|\n" +
	"00000030  60 00 00 00 00 00 01 ff  63 00 00 00 00 00 01 ff  |`.......c.......|\n" +
	"00000040  65 00 00 00 00 00 01 ff  68 00 00 00 00 00 01 00  |e.......h.......|\n" +
	"00000050  6c 00 00 00 00 00 01 00  78 00 00 00 00 00 01 05  |l.......x.......|\n" +
	"00000060  52 72 72 53 73 41 61 61  42 62 62 62 43 63 63 63  |RrrSsAaaBbbbCccc|\n" +
	"00000070  63 63 63 63 63 63 31 32                           |cccccc12|\n"

const writerWantILAStartCPageSize4 = "" +
	"00000000  72 c3 63 05 fc 4c 00 ff  00 00 00 00 00 00 00 ff  |r.c..L..........|\n" +
	"00000010  00 00 00 00 00 00 00 ff  11 00 00 00 00 00 00 ff  |................|\n" +
	"00000020  33 00 00 00 00 00 00 01  77 00 00 00 00 00 00 ee  |3.......w.......|\n" +
	"00000030  60 00 00 00 00 00 01 ff  64 00 00 00 00 00 01 ff  |`.......d.......|\n" +
	"00000040  68 00 00 00 00 00 01 ff  6c 00 00 00 00 00 01 00  |h.......l.......|\n" +
	"00000050  70 00 00 00 00 00 01 00  7c 00 00 00 00 00 01 05  |p.......|.......|\n" +
	"00000060  52 72 72 00 53 73 00 00  41 61 61 00 42 62 62 62  |Rrr.Ss..Aaa.Bbbb|\n" +
	"00000070  43 63 63 63 63 63 63 63  63 63 31 32              |Cccccccccc12|\n"

const writerWantILAStartCPageSize128 = "" +
	"00000000  72 c3 63 05 d8 df 00 ff  00 00 00 00 00 00 00 ff  |r.c.............|\n" +
	"00000010  00 00 00 00 00 00 00 ff  11 00 00 00 00 00 00 ff  |................|\n" +
	"00000020  33 00 00 00 00 00 00 01  77 00 00 00 00 00 00 ee  |3.......w.......|\n" +
	"00000030  80 00 00 00 00 00 01 ff  83 00 00 00 00 00 01 ff  |................|\n" +
	"00000040  85 00 00 00 00 00 01 ff  88 00 00 00 00 00 01 00  |................|\n" +
	"00000050  8c 00 00 00 00 00 01 00  98 00 00 00 00 00 01 05  |................|\n" +
	"00000060  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|\n" +
	"00000070  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|\n" +
	"00000080  52 72 72 53 73 41 61 61  42 62 62 62 43 63 63 63  |RrrSsAaaBbbbCccc|\n" +
	"00000090  63 63 63 63 63 63 31 32                           |cccccc12|\n"

func TestWriterILAEndEmpty(t *testing.T) {
	if err := testWriter(IndexLocationAtEnd, nil, 0, true); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAStartEmpty(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtStart, tempFile, 0, true); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAEndNoTempFile(t *testing.T) {
	if err := testWriter(IndexLocationAtEnd, nil, 0, false); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAEndMemTempFile(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtEnd, tempFile, 0, false); err == nil {
		t.Fatal("err: got nil, want non-nil")
	} else if !strings.HasPrefix(err.Error(), "rac: IndexLocationAtEnd requires") {
		t.Fatal(err)
	}
}

func TestWriterILAStartNoTempFile(t *testing.T) {
	if err := testWriter(IndexLocationAtStart, nil, 0, false); err == nil {
		t.Fatal("err: got nil, want non-nil")
	} else if !strings.HasPrefix(err.Error(), "rac: IndexLocationAtStart requires") {
		t.Fatal(err)
	}
}

func TestWriterILAStartMemTempFile(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtStart, tempFile, 0, false); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAStartRealTempFile(t *testing.T) {
	f, err := ioutil.TempFile("", "rac_test")
	if err != nil {
		t.Fatalf("TempFile: %v", err)
	}
	defer os.Remove(f.Name())
	defer f.Close()

	if err := testWriter(IndexLocationAtStart, f, 0, false); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAEndCPageSize8(t *testing.T) {
	if err := testWriter(IndexLocationAtEnd, nil, 8, false); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAStartCPageSize4(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtStart, tempFile, 4, false); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAStartCPageSize128(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtStart, tempFile, 128, false); err != nil {
		t.Fatal(err)
	}
}

func testWriter(iloc IndexLocation, tempFile io.ReadWriter, cPageSize uint64, empty bool) error {
	buf := &bytes.Buffer{}
	w := &Writer{
		Writer:        buf,
		Codec:         fakeCodec,
		IndexLocation: iloc,
		TempFile:      tempFile,
		CPageSize:     cPageSize,
	}

	if !empty {
		// We ignore errors (assigning them to _) from the AddXxx calls. Any
		// non-nil errors are sticky, and should be returned by Close.
		res0, _ := w.AddResource([]byte("Rrr"))
		res1, _ := w.AddResource([]byte("Ss"))
		_ = w.AddChunk(0x11, []byte("Aaa"), 0, 0)
		_ = w.AddChunk(0x22, []byte("Bbbb"), res0, 0)
		_ = w.AddChunk(0x44, []byte("Cccccccccc12"), res0, res1)
	}

	if err := w.Close(); err != nil {
		return err
	}
	got := hex.Dump(buf.Bytes())

	want := ""
	switch {
	case empty:
		want = writerWantEmpty
	case (iloc == IndexLocationAtEnd) && (cPageSize == 0):
		want = writerWantILAEnd
	case (iloc == IndexLocationAtEnd) && (cPageSize == 8):
		want = writerWantILAEndCPageSize8
	case (iloc == IndexLocationAtStart) && (cPageSize == 0):
		want = writerWantILAStart
	case (iloc == IndexLocationAtStart) && (cPageSize == 4):
		want = writerWantILAStartCPageSize4
	case (iloc == IndexLocationAtStart) && (cPageSize == 128):
		want = writerWantILAStartCPageSize128
	default:
		return fmt.Errorf("unsupported iloc/cPageSize combination")
	}

	if got != want {
		return fmt.Errorf("\ngot:\n%s\nwant:\n%s", got, want)
	}
	return nil
}

func TestWriterMultiLevelIndex(t *testing.T) {
	buf := &bytes.Buffer{}
	w := &Writer{
		Writer:        buf,
		Codec:         fakeCodec,
		IndexLocation: IndexLocationAtStart,
		TempFile:      &bytes.Buffer{},
	}

	// Write 260 chunks with 3 resources. With the current "func gather"
	// algorithm, this results in a root node with two children, both of which
	// are branch nodes. The first branch contains 252 chunks and refers to 3
	// resources (so that its arity is 255). The second branch contains 8
	// chunks and refers to 1 resource (so that its arity is 9).
	xRes := OptResource(0)
	yRes := OptResource(0)
	zRes := OptResource(0)
	for i := 0; i < 260; i++ {
		secondary := OptResource(0)
		tertiary := OptResource(0)

		switch i {
		case 3:
			xRes, _ = w.AddResource([]byte("XX"))
			yRes, _ = w.AddResource([]byte("YY"))
			secondary = xRes
			tertiary = yRes

		case 4:
			zRes, _ = w.AddResource([]byte("ZZ"))
			secondary = yRes
			tertiary = zRes

		case 259:
			secondary = yRes
		}

		primary := []byte(fmt.Sprintf("p%02x", i&0xFF))
		_ = w.AddChunk(0x10000, primary, secondary, tertiary)
	}

	if err := w.Close(); err != nil {
		t.Fatalf("Close: %v", err)
	}

	encoded := buf.Bytes()
	if got, want := len(encoded), 0x13E2; got != want {
		t.Fatalf("len(encoded): got 0x%X, want 0x%X", got, want)
	}

	got := hex.Dump(encoded)
	got = "" +
		got[0x000*bytesPerHexDumpLine:0x008*bytesPerHexDumpLine] +
		"...\n" +
		got[0x080*bytesPerHexDumpLine:0x088*bytesPerHexDumpLine] +
		"...\n" +
		got[0x100*bytesPerHexDumpLine:0x110*bytesPerHexDumpLine] +
		"...\n" +
		got[0x13C*bytesPerHexDumpLine:]

	const want = "" +
		"00000000  72 c3 63 02 a0 24 00 ff  00 00 fc 00 00 00 00 ff  |r.c..$..........|\n" +
		"00000010  00 00 04 01 00 00 00 ee  30 00 00 00 00 00 04 ff  |........0.......|\n" +
		"00000020  30 10 00 00 00 00 01 ff  e2 13 00 00 00 00 01 02  |0...............|\n" +
		"00000030  72 c3 63 ff 4c be 00 ff  00 00 00 00 00 00 00 ff  |r.c.L...........|\n" +
		"00000040  00 00 00 00 00 00 00 ff  00 00 00 00 00 00 00 ff  |................|\n" +
		"00000050  00 00 01 00 00 00 00 ff  00 00 02 00 00 00 00 ff  |................|\n" +
		"00000060  00 00 03 00 00 00 00 01  00 00 04 00 00 00 00 02  |................|\n" +
		"00000070  00 00 05 00 00 00 00 ff  00 00 06 00 00 00 00 ff  |................|\n" +
		"...\n" +
		"00000800  00 00 f7 00 00 00 00 ff  00 00 f8 00 00 00 00 ff  |................|\n" +
		"00000810  00 00 f9 00 00 00 00 ff  00 00 fa 00 00 00 00 ff  |................|\n" +
		"00000820  00 00 fb 00 00 00 00 ff  00 00 fc 00 00 00 00 ee  |................|\n" +
		"00000830  d9 10 00 00 00 00 01 ff  db 10 00 00 00 00 01 ff  |................|\n" +
		"00000840  e0 10 00 00 00 00 01 ff  d0 10 00 00 00 00 01 ff  |................|\n" +
		"00000850  d3 10 00 00 00 00 01 ff  d6 10 00 00 00 00 01 ff  |................|\n" +
		"00000860  dd 10 00 00 00 00 01 00  e2 10 00 00 00 00 01 01  |................|\n" +
		"00000870  e5 10 00 00 00 00 01 ff  e8 10 00 00 00 00 01 ff  |................|\n" +
		"...\n" +
		"00001000  bb 13 00 00 00 00 01 ff  be 13 00 00 00 00 01 ff  |................|\n" +
		"00001010  c1 13 00 00 00 00 01 ff  c4 13 00 00 00 00 01 ff  |................|\n" +
		"00001020  c7 13 00 00 00 00 01 ff  e2 13 00 00 00 00 01 ff  |................|\n" +
		"00001030  72 c3 63 09 f0 09 00 ff  00 00 00 00 00 00 00 ff  |r.c.............|\n" +
		"00001040  00 00 01 00 00 00 00 ff  00 00 02 00 00 00 00 ff  |................|\n" +
		"00001050  00 00 03 00 00 00 00 ff  00 00 04 00 00 00 00 ff  |................|\n" +
		"00001060  00 00 05 00 00 00 00 ff  00 00 06 00 00 00 00 ff  |................|\n" +
		"00001070  00 00 07 00 00 00 00 ff  00 00 08 00 00 00 00 ee  |................|\n" +
		"00001080  db 10 00 00 00 00 01 ff  ca 13 00 00 00 00 01 ff  |................|\n" +
		"00001090  cd 13 00 00 00 00 01 ff  d0 13 00 00 00 00 01 ff  |................|\n" +
		"000010a0  d3 13 00 00 00 00 01 ff  d6 13 00 00 00 00 01 ff  |................|\n" +
		"000010b0  d9 13 00 00 00 00 01 ff  dc 13 00 00 00 00 01 ff  |................|\n" +
		"000010c0  df 13 00 00 00 00 01 00  e2 13 00 00 00 00 01 09  |................|\n" +
		"000010d0  70 30 30 70 30 31 70 30  32 58 58 59 59 70 30 33  |p00p01p02XXYYp03|\n" +
		"000010e0  5a 5a 70 30 34 70 30 35  70 30 36 70 30 37 70 30  |ZZp04p05p06p07p0|\n" +
		"000010f0  38 70 30 39 70 30 61 70  30 62 70 30 63 70 30 64  |8p09p0ap0bp0cp0d|\n" +
		"...\n" +
		"000013c0  38 70 66 39 70 66 61 70  66 62 70 66 63 70 66 64  |8pf9pfapfbpfcpfd|\n" +
		"000013d0  70 66 65 70 66 66 70 30  30 70 30 31 70 30 32 70  |pfepffp00p01p02p|\n" +
		"000013e0  30 33                                             |03|\n"

	if got != want {
		t.Fatalf("\ngot:\n%s\nwant:\n%s", got, want)
	}
}
