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

// Note that these exact bytes depends on the zlib encoder's algorithm, but
// there is more than one valid zlib encoding of any given input. This "compare
// to golden output" test is admittedly brittle, as the standard library's zlib
// package's output isn't necessarily stable across Go releases.

const writerWantILAEnd = "" +
	"00000000  72 c3 63 00 52 72 72 53  73 41 61 61 42 62 62 62  |r.c.RrrSsAaaBbbb|\n" +
	"00000010  43 63 63 63 63 63 63 63  63 63 31 32 72 c3 63 05  |Cccccccccc12r.c.|\n" +
	"00000020  1d 45 00 ff 00 00 00 00  00 00 00 ff 00 00 00 00  |.E..............|\n" +
	"00000030  00 00 00 ff 11 00 00 00  00 00 00 ff 33 00 00 00  |............3...|\n" +
	"00000040  00 00 00 01 77 00 00 00  00 00 00 ee 04 00 00 00  |....w...........|\n" +
	"00000050  00 00 01 ff 07 00 00 00  00 00 01 ff 09 00 00 00  |................|\n" +
	"00000060  00 00 01 ff 0c 00 00 00  00 00 01 00 10 00 00 00  |................|\n" +
	"00000070  00 00 01 00 7c 00 00 00  00 00 01 05              |....|.......|\n"

const writerWantILAStart = "" +
	"00000000  72 c3 63 05 bc dc 00 ff  00 00 00 00 00 00 00 ff  |r.c.............|\n" +
	"00000010  00 00 00 00 00 00 00 ff  11 00 00 00 00 00 00 ff  |................|\n" +
	"00000020  33 00 00 00 00 00 00 01  77 00 00 00 00 00 00 ee  |3.......w.......|\n" +
	"00000030  60 00 00 00 00 00 01 ff  63 00 00 00 00 00 01 ff  |`.......c.......|\n" +
	"00000040  65 00 00 00 00 00 01 ff  68 00 00 00 00 00 01 00  |e.......h.......|\n" +
	"00000050  6c 00 00 00 00 00 01 00  78 00 00 00 00 00 01 05  |l.......x.......|\n" +
	"00000060  52 72 72 53 73 41 61 61  42 62 62 62 43 63 63 63  |RrrSsAaaBbbbCccc|\n" +
	"00000070  63 63 63 63 63 63 31 32                           |cccccc12|\n"

func TestWriterILAEndNoTempFile(t *testing.T) {
	if err := testWriter(IndexLocationAtEnd, nil); err != nil {
		t.Fatal(err)
	}
}

func TestWriterILAEndMemTempFile(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtEnd, tempFile); err == nil {
		t.Fatal("err: got nil, want non-nil")
	} else if !strings.HasPrefix(err.Error(), "rac: IndexLocationAtEnd requires") {
		t.Fatal(err)
	}
}

func TestWriterILAStartNoTempFile(t *testing.T) {
	if err := testWriter(IndexLocationAtStart, nil); err == nil {
		t.Fatal("err: got nil, want non-nil")
	} else if !strings.HasPrefix(err.Error(), "rac: IndexLocationAtStart requires") {
		t.Fatal(err)
	}
}

func TestWriterILAStartMemTempFile(t *testing.T) {
	tempFile := &bytes.Buffer{}
	if err := testWriter(IndexLocationAtStart, tempFile); err != nil {
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

	if err := testWriter(IndexLocationAtStart, f); err != nil {
		t.Fatal(err)
	}
}

func testWriter(iloc IndexLocation, tempFile io.ReadWriter) error {
	buf := &bytes.Buffer{}
	const fakeCodec = Codec(0xEE)
	w := &Writer{
		Writer:        buf,
		Codec:         fakeCodec,
		IndexLocation: iloc,
		TempFile:      tempFile,
	}

	// We ignore errors (assigning them to _) from the AddXxx calls. Any
	// non-nil errors are sticky, and should be returned by Close.
	res0, _ := w.AddResource([]byte("Rrr"))
	res1, _ := w.AddResource([]byte("Ss"))
	_ = w.AddChunk(0x11, []byte("Aaa"), 0, 0)
	_ = w.AddChunk(0x22, []byte("Bbbb"), res0, 0)
	_ = w.AddChunk(0x44, []byte("Cccccccccc12"), res0, res1)
	if err := w.Close(); err != nil {
		return err
	}
	got := hex.Dump(buf.Bytes())

	want := writerWantILAEnd
	if iloc == IndexLocationAtStart {
		want = writerWantILAStart
	}

	if got != want {
		return fmt.Errorf("\ngot:\n%s\nwant:\n%s", got, want)
	}
	return nil
}
