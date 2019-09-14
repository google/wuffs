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

package compression

import (
	"reflect"
	"testing"
)

func TestInterpolate(tt *testing.T) {
	got := []int32{}
	want := []int32{
		1, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 4, 4, 5, 5, 6,
		6, 6, 6, 7, 7, 7, 8, 8,
		9, 9, 9, 9, 9, 9, 9, 9,
		9,
	}

	for l := LevelFastest; l <= LevelSmallest; l += gap / 8 {
		got = append(got, l.Interpolate(1, 2, 6, 9, 9))
	}
	if !reflect.DeepEqual(got, want) {
		tt.Fatalf("\ngot  %d\nwant %d", got, want)
	}
}
