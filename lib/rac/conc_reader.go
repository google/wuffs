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
	"io"
)

// rWork is a unit of work for concurrent reading. The Manager sends empty
// buffers. Workers receive empty buffers and send filled buffers.
type rWork struct {
	dRange Range
	buf    *bytes.Buffer
	err    error
}

// stopWork is a cancel (non-permanent) or close (permanent) notice to the
// Manager and Workers. The recipient needs to acknowledge the request by
// receiving from ackc (if non-nil).
type stopWork struct {
	ackc        <-chan struct{}
	keepWorking bool
}

type concReader struct {
	// Channels between the concReader, the Manager and multiple Workers.
	//
	// The concReader sends roic and                recvs resc.
	// The Manager    recvs roic and sends reqc.
	// Each Worker                   recvs reqc and sends resc.
	roic chan Range // Region of Interest channel.
	reqc chan rWork // Work-Request       channel.
	resc chan rWork // Work-Response      channel.

	// bufc holds re-usable buffers.
	bufc chan *bytes.Buffer

	// stopc and ackc are used to synchronize the concReader, Manager and
	// Workers, either canceling work-in-progress or closing everything down.
	//
	// Importantly, these are unbuffered channels. Sending and receiving will
	// wait for the other end to synchronize.
	stopc chan stopWork
	ackc  chan struct{}

	// currWork holds the unit of work currently being processed by Read. It
	// will be recycled after Read is done with it, or if a Seek causes us to
	// cancel the work-in-progress.
	//
	// currBytes is currWork's buffer's bytes. currBytes[:currIndex] has
	// already been sent out via Read. currBytes[currIndex:] has not.
	currWork  rWork
	currBytes []byte
	currIndex int

	// completedWorks hold completed units of work that are not the next unit
	// to be sent out via Read. Works may arrive out of order.
	//
	// The map is keyed by an rWork's dRange[0].
	completedWorks map[int64]rWork

	// numWorkers is the number of concurrent Workers.
	numWorkers int

	// seekResolved means that Read does not have to seek to pos.
	//
	// Each Seek call is relatively cheap, only changing the pos field. The
	// bulk of the work happens in the first Read call following a Seek call.
	seekResolved bool

	// seenRead means that we've already seen at least one Read call.
	seenRead bool

	// pos is the current position, in DSpace. It is the base value when Seek
	// is called with io.SeekCurrent.
	pos int64

	// decompressedSize is the size of the RAC file in DSpace.
	decompressedSize int64
}

func (c *concReader) initialize(racReader *Reader) {
	if racReader.Concurrency <= 1 {
		return
	}
	c.numWorkers = racReader.Concurrency
	if c.numWorkers > 65536 {
		c.numWorkers = 65536
	}

	// Set up other state.
	c.completedWorks = map[int64]rWork{}
	c.decompressedSize = racReader.chunkReader.decompressedSize

	// Set up re-usable buffers to hold decoded bytes. The number of buffers is
	// arbitrary, but is larger than the number of Workers because individual
	// pieces of work (the rWork type) may complete in a different order than
	// they need to be written to dst. Having spare buffers means fewer idle
	// Workers.
	c.bufc = make(chan *bytes.Buffer, 2*c.numWorkers)
	for i := 0; i < cap(c.bufc); i++ {
		c.bufc <- &bytes.Buffer{}
	}

	// Set up the Manager and the Workers.
	c.roic = make(chan Range, 1)
	c.reqc = make(chan rWork, c.numWorkers)
	c.resc = make(chan rWork, c.numWorkers)
	c.ackc = make(chan struct{})
	c.stopc = make(chan stopWork)
	for i := 0; i < c.numWorkers; i++ {
		rr := racReader.clone()
		rr.Concurrency = 0
		go runRWorker(c.stopc, c.resc, c.reqc, rr)
	}
	go runRManager(c.stopc, c.roic, c.bufc, c.reqc, &racReader.chunkReader)
}

func (c *concReader) ready() bool {
	return c.stopc != nil
}

func (c *concReader) Close() error {
	if c.stopc != nil {
		c.stopAnyWorkInProgress(false)
		c.stopc = nil
	}
	return nil
}

func (c *concReader) CloseWithoutWaiting() error {
	if c.stopc != nil {
		// Just close the c.stopc channel, which should eventually shut down
		// the Manager and Worker goroutines. Everything else can be garbage
		// collected.
		close(c.stopc)
		c.stopc = nil
	}
	return nil
}

func (c *concReader) Seek(offset int64, whence int) (int64, error) {
	pos := c.pos
	switch whence {
	case io.SeekStart:
		pos = offset
	case io.SeekCurrent:
		pos += offset
	case io.SeekEnd:
		pos = c.decompressedSize + offset
	default:
		return 0, errSeekToInvalidWhence
	}

	if c.pos != pos {
		if pos < 0 {
			return 0, errSeekToNegativePosition
		}
		c.pos = pos
		c.seekResolved = false
	}
	return pos, nil
}

func (c *concReader) Read(p []byte) (int, error) {
	if c.pos >= c.decompressedSize {
		return 0, io.EOF
	}

	if !c.seekResolved {
		c.seekResolved = true
		if c.seenRead {
			c.stopAnyWorkInProgress(true)
		}
		c.seenRead = true
		c.roic <- Range{c.pos, c.decompressedSize}
	}

	for numRead := 0; ; {
		if c.pos >= c.decompressedSize {
			return numRead, io.EOF
		}
		if len(p) == 0 {
			return numRead, nil
		}

		// Fill p from c.currWork.
		if c.currWork.buf != nil {
			n := copy(p, c.currBytes[c.currIndex:])
			p = p[n:]
			numRead += n
			c.pos += int64(n)
			c.currIndex += n
			err := c.currWork.err

			// Recycle c.currWork if we're done with it.
			if c.currIndex == len(c.currBytes) {
				c.bufc <- c.currWork.buf
				c.currWork = rWork{}
				c.currBytes = nil
				c.currIndex = 0
			}

			if err != nil {
				return numRead, err
			}
			continue
		}

		work, err := c.nextWork()
		if err != nil {
			return numRead, err
		}
		c.currWork = work
		c.currBytes = work.buf.Bytes()
		c.currIndex = 0
	}
}

func (c *concReader) nextWork() (rWork, error) {
	for {
		if work, ok := c.completedWorks[c.pos]; ok {
			delete(c.completedWorks, c.pos)
			return work, nil
		}

		// This shouldn't happen, but if it does, print a specific error
		// message instead of the more general "deadlock" message.
		if len(c.completedWorks) == cap(c.bufc) {
			return rWork{}, errInternalAllWorkersIdle
		}

		work := <-c.resc
		c.completedWorks[work.dRange[0]] = work
	}
}

// stopAnyWorkInProgress winds up any Manager and Worker work-in-progress.
// keepWorking is whether those goroutines should stick around to do future
// work. It should be false for closes and true otherwise.
func (c *concReader) stopAnyWorkInProgress(keepWorking bool) {
	// Synchronize the Manager and Workers on stopc (an unbuffered channel).
	for i, n := 0, 1+c.numWorkers; i < n; i++ {
		c.stopc <- stopWork{c.ackc, keepWorking}
	}

	if keepWorking {
		c.recycleBuffers()
	}

	// Synchronize the Manager and Workers on ackc (an unbuffered channel).
	for i, n := 0, 1+c.numWorkers; i < n; i++ {
		c.ackc <- struct{}{}
	}
}

func (c *concReader) recycleBuffers() {
	if c.currWork.buf != nil {
		c.bufc <- c.currWork.buf
		c.currWork = rWork{}
		c.currBytes = nil
		c.currIndex = 0
	}

	for k, work := range c.completedWorks {
		c.bufc <- work.buf
		delete(c.completedWorks, k)
	}

	for {
		select {
		case work := <-c.resc:
			c.bufc <- work.buf
		default:
			return
		}
	}
}

func runRWorker(stopc <-chan stopWork, resc chan<- rWork, reqc <-chan rWork, racReader *Reader) {
	input, output := reqc, (chan<- rWork)(nil)
	work := rWork{}
	limitedReader := io.LimitedReader{}

loop:
	for {
		select {
		case stop := <-stopc:
			if stop.ackc != nil {
				<-stop.ackc
			} else {
				// No need to ack. This is CloseWithoutWaiting.
			}
			if !stop.keepWorking {
				return
			}
			continue loop

		case work = <-input:
			input, output = nil, resc
			if work.err == nil {
				_, work.err = racReader.Seek(work.dRange[0], io.SeekStart)
			}
			if work.err == nil {
				limitedReader = io.LimitedReader{
					R: racReader,
					N: work.dRange.Size(),
				}
				_, work.err = io.Copy(work.buf, &limitedReader)
			}
			if work.err == io.EOF {
				work.err = io.ErrUnexpectedEOF
			}

		case output <- work:
			input, output = reqc, nil
			work = rWork{}
		}
	}
}

func runRManager(stopc <-chan stopWork, roic <-chan Range, bufc <-chan *bytes.Buffer, reqc chan<- rWork, chunkReader *ChunkReader) {
	input, output := roic, (chan<- rWork)(nil)
	roi := Range{}
	work := rWork{}

loop:
	for {
		select {
		case stop := <-stopc:
			if stop.ackc != nil {
				<-stop.ackc
			} else {
				// No need to ack. This is CloseWithoutWaiting.
			}
			if !stop.keepWorking {
				return
			}
			continue loop

		case roi = <-input:
			input, output = nil, reqc
			if err := chunkReader.SeekToChunkContaining(roi[0]); err != nil {
				if err == io.EOF {
					err = io.ErrUnexpectedEOF
				}
				work = rWork{err: err}
				continue loop
			}

		case output <- work:
			err := work.err
			work = rWork{}
			if err != nil {
				input, output = roic, nil
				continue loop
			}
		}

		for {
			chunk, err := chunkReader.NextChunk()
			if err == io.EOF {
				input, output = roic, nil
				continue loop
			} else if err != nil {
				work = rWork{err: err}
				continue loop
			}

			dr := chunk.DRange.Intersect(roi)
			if dr[0] >= roi[1] {
				input, output = roic, nil
				continue loop
			} else if dr.Empty() {
				continue
			}

			select {
			case stop := <-stopc:
				if stop.ackc != nil {
					<-stop.ackc
				} else {
					// No need to ack. This is CloseWithoutWaiting.
				}
				if !stop.keepWorking {
					return
				}
				continue loop

			case buf := <-bufc:
				buf.Reset()
				work = rWork{dRange: dr, buf: buf}
				continue loop
			}
		}
	}
}
