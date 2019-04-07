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
	"errors"
	"hash/crc32"
	"io"
	"sort"
)

func putU64LE(b []byte, v uint64) {
	_ = b[7] // Early bounds check to guarantee safety of writes below.
	b[0] = byte(v)
	b[1] = byte(v >> 8)
	b[2] = byte(v >> 16)
	b[3] = byte(v >> 24)
	b[4] = byte(v >> 32)
	b[5] = byte(v >> 40)
	b[6] = byte(v >> 48)
	b[7] = byte(v >> 56)
}

// Writer provides a relatively simple way to write a RAC file - one that is
// created starting from nothing, as opposed to incrementally modifying an
// existing RAC file.
//
// Other packages may provide a more flexible (and more complicated) way to
// write or append to RAC files, but that is out of scope of this package.
//
// Do not modify its exported fields after calling any of its methods.
type Writer struct {
	// Writer is where the RAC-encoded data is written to.
	//
	// Nil is an invalid value.
	Writer io.Writer

	// Codec is the compression codec for the RAC file.
	//
	// See the RAC specification for further discussion.
	//
	// Zero is an invalid value.
	Codec Codec

	// initialized is set true after the first AddXxx call.
	initialized bool

	// IndexLocation is whether the index is at the start or end of the RAC
	// file.
	//
	// See the RAC specification for further discussion.
	//
	// The zero value of this field is IndexLocationAtEnd: a one pass encoding.
	IndexLocation IndexLocation

	// TempFile is a temporary file to use for a two pass encoding. The field
	// name says "file" but it need not be a real file, in the operating system
	// sense.
	//
	// For IndexLocationAtEnd, this must be nil. For IndexLocationAtStart, this
	// must be non-nil.
	//
	// It does not need to implement io.Seeker, if it supports separate read
	// and write positions (e.g. it is a bytes.Buffer). If it does implement
	// io.Seeker (e.g. it is an os.File), its current position will be noted
	// (via SeekCurrent), then it will be written to (via the rac.Writer.AddXxx
	// methods), then its position will be reset (via SeekSet), then it will be
	// read from (via the rac.Writer.Close method).
	//
	// The rac.Writer does not call TempFile.Close even if that method exists
	// (e.g. the TempFile is an os.File). In that case, closing the temporary
	// file (and deleting it) after the rac.Writer is closed is the
	// responsibility of the rac.Writer caller, not the rac.Writer itself.
	TempFile io.ReadWriter

	// err is the first error encountered. It is sticky: once a non-nil error
	// occurs, all public methods will return that error.
	err error

	// tempFileSeekStart is the TempFile's starting position, if the TempFile
	// is an io.Seeker.
	tempFileSeekStart int64

	// dataSize is the total size (so far) of the data portion (as opposed to
	// the index portion) of the compressed file. For a two pass encoding, this
	// should equal the size of the TempFile.
	dataSize uint64

	// dFileSize is the total size (so far) of the decompressed file. It is the
	// sum of all the dRangeSize arguments passed to AddChunk.
	dFileSize uint64

	// resourcesCOffsets is the file offsets of each shared resource. Those
	// offsets are for the compressed file (what the RAC spec calls CSpace).
	//
	// The first element (if it exists) is an unused placeholder, as a zero
	// OptResource means unused.
	resourcesCOffsets []uint64

	// leafNodes are the non-resource leaf nodes of the hierarchical index.
	leafNodes []node
}

func (w *Writer) initialize() error {
	if w.err != nil {
		return w.err
	}
	if w.initialized {
		return nil
	}
	w.initialized = true

	if w.Writer == nil {
		w.err = errors.New("rac: invalid Writer")
		return w.err
	}
	if w.Codec == 0 {
		w.err = errors.New("rac: invalid Codec")
		return w.err
	}

	if s, ok := w.TempFile.(io.Seeker); ok {
		if n, err := s.Seek(0, io.SeekCurrent); err != nil {
			w.err = err
			return err
		} else {
			w.tempFileSeekStart = n
		}
	}

	switch w.IndexLocation {
	case IndexLocationAtEnd:
		if w.TempFile != nil {
			w.err = errors.New("rac: IndexLocationAtEnd requires a nil TempFile")
			return w.err
		}
		return w.write(indexLocationAtEndMagic)

	case IndexLocationAtStart:
		if w.TempFile == nil {
			w.err = errors.New("rac: IndexLocationAtStart requires a non-nil TempFile")
			return w.err
		}
	}
	return nil
}

func (w *Writer) write(data []byte) error {
	ioWriter := w.Writer
	if w.TempFile != nil {
		ioWriter = w.TempFile
	}

	if _, err := ioWriter.Write(data); err != nil {
		w.err = err
		return err
	}
	w.dataSize += uint64(len(data))
	return nil
}

// AddResource adds a shared resource to the RAC file. It returns a non-zero
// OptResource that identifies the resource's bytes. Future calls to AddChunk
// can pass these identifiers to indicate that decompressing a chunk depends on
// up to two shared resources.
//
// The caller may modify resource's contents after this method returns.
func (w *Writer) AddResource(resource []byte) (OptResource, error) {
	if err := w.initialize(); err != nil {
		return 0, err
	}

	if len(w.resourcesCOffsets) >= (1 << 30) {
		w.err = errors.New("rac: too many shared resources")
		return 0, w.err
	}
	if len(w.resourcesCOffsets) == 0 {
		w.resourcesCOffsets = make([]uint64, 1, 8)
	}
	id := OptResource(len(w.resourcesCOffsets))
	cLength := calcCLength(len(resource))
	w.resourcesCOffsets = append(w.resourcesCOffsets, w.dataSize|(cLength<<48))
	if err := w.write(resource); err != nil {
		return 0, err
	}
	return id, nil
}

// AddChunk adds a chunk of compressed data - the (primary, secondary,
// tertiary) tuple - to the RAC file. Decompressing that chunk should produce
// dRangeSize bytes, although the rac.Writer does not attempt to verify that.
//
// The OptResource arguments may be zero, meaning that no resource is used. In
// that case, the corresponding STag or TTag (see the RAC specification for
// further discussion) will be 0xFF.
//
// The caller may modify primary's contents after this method returns.
func (w *Writer) AddChunk(dRangeSize uint64, primary []byte, secondary OptResource, tertiary OptResource) error {
	if err := w.initialize(); err != nil {
		return err
	}

	if dRangeSize == 0 {
		w.err = errors.New("rac: dRangeSize is too small")
		return w.err
	}
	if (dRangeSize > MaxSize) || ((w.dFileSize + dRangeSize) > MaxSize) {
		w.err = errors.New("rac: dRangeSize is too large")
		return w.err
	}
	cLength := calcCLength(len(primary))
	w.dFileSize += dRangeSize
	w.leafNodes = append(w.leafNodes, node{
		dRangeSize: dRangeSize,
		cOffset:    w.dataSize | (cLength << 48),
		secondary:  secondary,
		tertiary:   tertiary,
	})
	return w.write(primary)
}

// Close writes the RAC index to w.Writer and marks that w accepts no further
// method calls.
//
// For a one pass encoding, no further action is taken. For a two pass encoding
// (i.e. IndexLocationAtStart), it then copies w.TempFile to w.Writer. Either
// way, if this method returns nil error, the entirety of what was written to
// w.Writer constitutes a valid RAC file.
//
// Closing the underlying w.Writer, w.TempFile or both is the responsibility of
// the rac.Writer caller, not the rac.Writer itself.
//
// It is not necessary to call Close, e.g. if an earlier AddXxx call returned a
// non-nil error. Unlike an os.File, failing to call rac.Writer.Close will not
// leak resources such as file descriptors.
func (w *Writer) Close() error {
	if w.err != nil {
		return w.err
	}

	rootNode := node{}
	if len(w.leafNodes) == 0 {
		rootNode.children = []node{{}}
	} else {
		rootNode = gather(w.leafNodes)
	}
	indexSize := rootNode.calcEncodedSize(0)

	nw := &nodeWriter{
		w:                 w.Writer,
		resourcesCOffsets: w.resourcesCOffsets,
		cFileSize:         indexSize + w.dataSize,
		codec:             w.Codec,
	}

	if w.IndexLocation == IndexLocationAtEnd {
		// Write the index. The compressed data has already been written.
		nw.indexCOffset = w.dataSize
		if err := nw.writeIndex(&rootNode); err != nil {
			w.err = err
			return err
		}

	} else {
		// Write the index.
		nw.dataCOffset = indexSize
		if err := nw.writeIndex(&rootNode); err != nil {
			w.err = err
			return err
		}

		// Write the compressed data.
		if s, ok := w.TempFile.(io.Seeker); ok {
			if _, err := s.Seek(w.tempFileSeekStart, io.SeekStart); err != nil {
				w.err = err
				return err
			}
		}
		if n, err := io.Copy(w.Writer, w.TempFile); err != nil {
			w.err = err
			return err
		} else if uint64(n) != w.dataSize {
			w.err = errors.New("rac: inconsistent compressed data size")
			return w.err
		}
	}

	w.err = errors.New("rac: Writer is closed")
	return nil
}

type node struct {
	dRangeSize uint64

	children  []node // Excludes resource-node chidren.
	resources []int

	cOffset   uint64
	secondary OptResource
	tertiary  OptResource
}

// calcEncodedSize accumulates the encoded size of n and its children,
// traversing the nodes in depth-first order.
//
// As a side effect, it also sets n.cOffset for branch nodes.
func (n *node) calcEncodedSize(accumulator uint64) (newAccumulator uint64) {
	arity := len(n.children) + len(n.resources)
	if arity == 0 {
		return accumulator
	}
	size := (arity * 16) + 16
	cLength := calcCLength(size)
	n.cOffset = accumulator | (cLength << 48)
	accumulator += uint64(size)
	for i := range n.children {
		accumulator = n.children[i].calcEncodedSize(accumulator)
	}
	return accumulator
}

type nodeWriter struct {
	w io.Writer

	cFileSize         uint64
	dataCOffset       uint64
	indexCOffset      uint64
	resourcesCOffsets []uint64

	codec Codec

	// A node's maximum arity is 255, so a node's maximum size in bytes is
	// ((255 * 16) + 16), which is 4096.
	buffer [4096]byte
}

func (w *nodeWriter) writeIndex(n *node) error {
	const tagFF = 0xFF << 56

	buf, dPtr := w.buffer[:], uint64(0)
	arity := uint64(len(n.children) + len(n.resources))
	if arity > 0xFF {
		return errors.New("rac: internal error: arity is too large")
	}

	// DPtr's. We write resources before regular children (non-resources), so
	// that any TTag that refers to a resource always avoids the [0xC0, 0xFD]
	// reserved zone. The ratio of resources to regulars is at most 2:1, as
	// every chunk uses exactly 1 regular slot and up to 2 resource slots, and
	// that reserved zone is less than 33% of the [0x00, 0xFF] space.
	for i := range n.resources {
		putU64LE(buf[8*i:], dPtr|tagFF)
	}
	buf = buf[8*len(n.resources):]
	for i, o := range n.children {
		putU64LE(buf[8*i:], dPtr|resourceToTag(n.resources, o.tertiary))
		dPtr += o.dRangeSize
	}
	buf = buf[8*len(n.children):]

	// DPtrMax.
	putU64LE(buf, dPtr|(uint64(w.codec)<<56))
	buf = buf[8:]

	// CPtr's. While the RAC format allows otherwise, this Writer always keeps
	// the CBias at zero, so that a CPtr equals a COffset.
	for i, res := range n.resources {
		cOffset := w.resourcesCOffsets[res] + w.dataCOffset
		putU64LE(buf[8*i:], cOffset|tagFF)
	}
	buf = buf[8*len(n.resources):]
	for i, o := range n.children {
		cOffset := o.cOffset
		if len(o.children) == 0 {
			cOffset += w.dataCOffset
		} else {
			cOffset += w.indexCOffset
		}
		putU64LE(buf[8*i:], cOffset|resourceToTag(n.resources, o.secondary))
	}
	buf = buf[8*len(n.children):]

	// CPtrMax.
	const version = 0x01
	putU64LE(buf, w.cFileSize|(version<<48)|(arity<<56))

	// Magic and Arity.
	w.buffer[0] = 0x72
	w.buffer[1] = 0xC3
	w.buffer[2] = 0x63
	w.buffer[3] = uint8(arity)

	// Checksum.
	size := (arity * 16) + 16
	checksum := crc32.ChecksumIEEE(w.buffer[6:size])
	checksum ^= checksum >> 16
	w.buffer[4] = uint8(checksum >> 0)
	w.buffer[5] = uint8(checksum >> 8)

	_, err := w.w.Write(w.buffer[:size])
	return err
}

func calcCLength(primarySize int) uint64 {
	if primarySize <= 0 {
		return 1
	}

	// Divide by 1024, rounding up.
	primarySize = ((primarySize - 1) >> 10) + 1

	if primarySize > 255 {
		return 0
	}
	return uint64(primarySize)
}

func resourceToTag(resources []int, r OptResource) uint64 {
	if r != 0 {
		for i, res := range resources {
			if res == int(r) {
				return uint64(i) << 56
			}
		}
	}
	return 0xFF << 56
}

// gather brings the given nodes into a tree, such that every branch node's
// arity (the count of its resource and non-resource child nodes) is at most
// 0xFF. It returns the root of that tree.
//
// TODO: it currently builds a tree where all leaf nodes have equal depth. For
// small RAC files (with only one branch node: the mandatory root node), this
// doesn't matter. For larger RAC files, it might be better to build an uneven
// tree to minimize average leaf node depth, with some branch nodes having both
// branch node children and leaf node children. It might even be worth ensuring
// that the root node always directly contains the first and last leaf nodes as
// children, as those leaves presumably contain the most commonly accessed
// parts of the decompressed file.
func gather(nodes []node) node {
	if len(nodes) == 0 {
		panic("gather: no nodes")
	}

	resources := map[OptResource]bool{}
	arity := 0

	for {
		i, j, newNodes := 0, 0, []node(nil)
		for ; j < len(nodes); j++ {
			o := &nodes[j]

			arity++
			if (o.secondary != 0) && !resources[o.secondary] {
				resources[o.secondary] = true
				arity++
			}
			if (o.tertiary != 0) && !resources[o.tertiary] {
				resources[o.tertiary] = true
				arity++
			}
			if arity <= 0xFF {
				continue
			}

			newNodes = append(newNodes, makeBranch(nodes[i:j], resources))
			i = j
			arity = 0
			if len(resources) != 0 {
				resources = map[OptResource]bool{}
			}
		}

		if i == 0 {
			return makeBranch(nodes, resources)
		}
		nodes, newNodes = newNodes, nil
	}
}

func makeBranch(children []node, resMap map[OptResource]bool) node {
	dRangeSize := uint64(0)
	for _, c := range children {
		dRangeSize += c.dRangeSize
	}

	resList := []int(nil)
	if len(resMap) != 0 {
		resList = make([]int, 0, len(resMap))
		for res := range resMap {
			resList = append(resList, int(res))
		}
		sort.Ints(resList)
	}

	return node{
		dRangeSize: dRangeSize,
		children:   children,
		resources:  resList,
		cOffset:    invalidCOffset,
	}
}
