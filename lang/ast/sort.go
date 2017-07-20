// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	t "github.com/google/puffs/lang/token"
)

func TopologicalSortStructs(ns []*Struct) (sorted []*Struct, ok bool) {
	// Algorithm is a depth-first search as per
	// https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
	sorted = make([]*Struct, 0, len(ns))
	byName := map[t.ID]*Struct{}
	for _, n := range ns {
		byName[n.Name()] = n
	}
	marks := map[*Struct]uint8{}
	for _, n := range ns {
		if _, ok := marks[n]; !ok {
			sorted, ok = tssVisit(sorted, n, byName, marks)
			if !ok {
				return nil, false
			}
		}
	}
	return sorted, true
}

func tssVisit(dst []*Struct, n *Struct, byName map[t.ID]*Struct, marks map[*Struct]uint8) ([]*Struct, bool) {
	const (
		unmarked  = 0
		temporary = 1
		permanent = 2
	)
	switch marks[n] {
	case temporary:
		return nil, false
	case permanent:
		return dst, true
	}
	marks[n] = temporary

	for _, f := range n.Fields() {
		x := f.Field().XType().Innermost()
		if x.Decorator() != 0 {
			continue
		}
		if o := byName[x.Name()]; o != nil {
			var ok bool
			dst, ok = tssVisit(dst, o, byName, marks)
			if !ok {
				return nil, false
			}
		}
	}

	marks[n] = permanent
	return append(dst, n), true
}
