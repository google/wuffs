// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// extract-palette-indexes.go extracts the 1-byte-per-pixel indexes and the
// 3-byte-per-entry RGB palette of a paletted GIF and PNG image. It checks that
// the GIF and PNG encode equivalent images.
//
// Usage: go run extract-palette-indexes.go foo.gif foo.png

import (
	"fmt"
	"image"
	"image/gif"
	"image/png"
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	baseFilename, img := "", (*image.Paletted)(nil)
	for i, a := range os.Args[1:] {
		b := a[:len(a)-len(filepath.Ext(a))]
		m, err := decode(a)
		if err != nil {
			return err
		}
		if i == 0 {
			baseFilename, img = b, m
		} else if baseFilename != b {
			return fmt.Errorf("arguments do not have a common base path")
		} else if !reflect.DeepEqual(img, m) {
			return fmt.Errorf("images are not equivalent")
		}
	}
	palette := make([]byte, 3*256)
	for i, c := range img.Palette {
		r, g, b, _ := c.RGBA()
		palette[3*i+0] = uint8(r >> 8)
		palette[3*i+1] = uint8(g >> 8)
		palette[3*i+2] = uint8(b >> 8)
	}
	if err := ioutil.WriteFile(baseFilename+".indexes", img.Pix, 0644); err != nil {
		return err
	}
	if err := ioutil.WriteFile(baseFilename+".palette", palette, 0644); err != nil {
		return err
	}
	return nil
}

func decode(filename string) (*image.Paletted, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	m, err := image.Image(nil), error(nil)
	switch ext := filepath.Ext(filename); ext {
	case ".gif":
		m, err = gif.Decode(f)
	case ".png":
		m, err = png.Decode(f)
	default:
		return nil, fmt.Errorf("unsupported filename extension %q", ext)
	}
	if err != nil {
		return nil, err
	}
	if p, ok := m.(*image.Paletted); ok {
		return p, nil
	}
	return nil, fmt.Errorf("decoded image type: got %T, want *image.Paletted", m)
}
