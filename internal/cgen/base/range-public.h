// After editing this file, run "go generate" in the parent directory.

// Copyright 2017 The Wuffs Authors.
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

// ---------------- Ranges and Rects

// Ranges are either inclusive ("range_ii") or exclusive ("range_ie") on the
// high end. Both the "ii" and "ie" flavors are useful in practice.
//
// The "ei" and "ee" flavors also exist in theory, but aren't widely used. In
// Wuffs, the low end is always inclusive.
//
// The "ii" (closed interval) flavor is useful when refining e.g. "the set of
// all uint32_t values" to a contiguous subset: "uint32_t values in the closed
// interval [M, N]", for uint32_t values M and N. An unrefined type (in other
// words, the set of all uint32_t values) is not representable in the "ie"
// flavor because if N equals ((1<<32) - 1) then (N + 1) will overflow.
//
// On the other hand, the "ie" (half-open interval) flavor is recommended by
// Dijkstra's "Why numbering should start at zero" at
// http://www.cs.utexas.edu/users/EWD/ewd08xx/EWD831.PDF and a further
// discussion of motivating rationale is at
// https://www.quora.com/Why-are-Python-ranges-half-open-exclusive-instead-of-closed-inclusive
//
// For example, with "ie", the number of elements in "uint32_t values in the
// half-open interval [M, N)" is equal to max(0, N-M). Furthermore, that number
// of elements (in one dimension, a length, in two dimensions, a width or
// height) is itself representable as a uint32_t without overflow, again for
// uint32_t values M and N. In the contrasting "ii" flavor, the length of the
// closed interval [0, (1<<32) - 1] is 1<<32, which cannot be represented as a
// uint32_t. In Wuffs, because of this potential overflow, the "ie" flavor has
// length / width / height methods, but the "ii" flavor does not.
//
// It is valid for min > max (for range_ii) or for min >= max (for range_ie),
// in which case the range is empty. There are multiple representations of an
// empty range.

typedef struct wuffs_base__range_ii_u32__struct {
  uint32_t min_incl;
  uint32_t max_incl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ii_u32__struct s) const;
  inline wuffs_base__range_ii_u32__struct intersect(
      wuffs_base__range_ii_u32__struct s) const;
  inline wuffs_base__range_ii_u32__struct unite(
      wuffs_base__range_ii_u32__struct s) const;
  inline bool contains(uint32_t x) const;
  inline bool contains_range(wuffs_base__range_ii_u32__struct s) const;
#endif  // __cplusplus

} wuffs_base__range_ii_u32;

static inline wuffs_base__range_ii_u32  //
wuffs_base__make_range_ii_u32(uint32_t min_incl, uint32_t max_incl) {
  wuffs_base__range_ii_u32 ret;
  ret.min_incl = min_incl;
  ret.max_incl = max_incl;
  return ret;
}

static inline bool  //
wuffs_base__range_ii_u32__is_empty(const wuffs_base__range_ii_u32* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u32__equals(const wuffs_base__range_ii_u32* r,
                                 wuffs_base__range_ii_u32 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u32__is_empty(r) &&
          wuffs_base__range_ii_u32__is_empty(&s));
}

static inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32__intersect(const wuffs_base__range_ii_u32* r,
                                    wuffs_base__range_ii_u32 s) {
  wuffs_base__range_ii_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u32__min(r->max_incl, s.max_incl);
  return t;
}

static inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32__unite(const wuffs_base__range_ii_u32* r,
                                wuffs_base__range_ii_u32 s) {
  if (wuffs_base__range_ii_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ii_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ii_u32 t;
  t.min_incl = wuffs_base__u32__min(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u32__max(r->max_incl, s.max_incl);
  return t;
}

static inline bool  //
wuffs_base__range_ii_u32__contains(const wuffs_base__range_ii_u32* r,
                                   uint32_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline bool  //
wuffs_base__range_ii_u32__contains_range(const wuffs_base__range_ii_u32* r,
                                         wuffs_base__range_ii_u32 s) {
  return wuffs_base__range_ii_u32__equals(
      &s, wuffs_base__range_ii_u32__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u32::is_empty() const {
  return wuffs_base__range_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u32::equals(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__equals(this, s);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::intersect(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__intersect(this, s);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::unite(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__unite(this, s);
}

inline bool  //
wuffs_base__range_ii_u32::contains(uint32_t x) const {
  return wuffs_base__range_ii_u32__contains(this, x);
}

inline bool  //
wuffs_base__range_ii_u32::contains_range(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__contains_range(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u32__struct {
  uint32_t min_incl;
  uint32_t max_excl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ie_u32__struct s) const;
  inline wuffs_base__range_ie_u32__struct intersect(
      wuffs_base__range_ie_u32__struct s) const;
  inline wuffs_base__range_ie_u32__struct unite(
      wuffs_base__range_ie_u32__struct s) const;
  inline bool contains(uint32_t x) const;
  inline bool contains_range(wuffs_base__range_ie_u32__struct s) const;
  inline uint32_t length() const;
#endif  // __cplusplus

} wuffs_base__range_ie_u32;

static inline wuffs_base__range_ie_u32  //
wuffs_base__make_range_ie_u32(uint32_t min_incl, uint32_t max_excl) {
  wuffs_base__range_ie_u32 ret;
  ret.min_incl = min_incl;
  ret.max_excl = max_excl;
  return ret;
}

static inline bool  //
wuffs_base__range_ie_u32__is_empty(const wuffs_base__range_ie_u32* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u32__equals(const wuffs_base__range_ie_u32* r,
                                 wuffs_base__range_ie_u32 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u32__is_empty(r) &&
          wuffs_base__range_ie_u32__is_empty(&s));
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__intersect(const wuffs_base__range_ie_u32* r,
                                    wuffs_base__range_ie_u32 s) {
  wuffs_base__range_ie_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u32__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__unite(const wuffs_base__range_ie_u32* r,
                                wuffs_base__range_ie_u32 s) {
  if (wuffs_base__range_ie_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ie_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ie_u32 t;
  t.min_incl = wuffs_base__u32__min(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u32__max(r->max_excl, s.max_excl);
  return t;
}

static inline bool  //
wuffs_base__range_ie_u32__contains(const wuffs_base__range_ie_u32* r,
                                   uint32_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline bool  //
wuffs_base__range_ie_u32__contains_range(const wuffs_base__range_ie_u32* r,
                                         wuffs_base__range_ie_u32 s) {
  return wuffs_base__range_ie_u32__equals(
      &s, wuffs_base__range_ie_u32__intersect(r, s));
}

static inline uint32_t  //
wuffs_base__range_ie_u32__length(const wuffs_base__range_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u32::is_empty() const {
  return wuffs_base__range_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u32::equals(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__equals(this, s);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::intersect(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__intersect(this, s);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::unite(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__unite(this, s);
}

inline bool  //
wuffs_base__range_ie_u32::contains(uint32_t x) const {
  return wuffs_base__range_ie_u32__contains(this, x);
}

inline bool  //
wuffs_base__range_ie_u32::contains_range(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__contains_range(this, s);
}

inline uint32_t  //
wuffs_base__range_ie_u32::length() const {
  return wuffs_base__range_ie_u32__length(this);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ii_u64__struct {
  uint64_t min_incl;
  uint64_t max_incl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ii_u64__struct s) const;
  inline wuffs_base__range_ii_u64__struct intersect(
      wuffs_base__range_ii_u64__struct s) const;
  inline wuffs_base__range_ii_u64__struct unite(
      wuffs_base__range_ii_u64__struct s) const;
  inline bool contains(uint64_t x) const;
  inline bool contains_range(wuffs_base__range_ii_u64__struct s) const;
#endif  // __cplusplus

} wuffs_base__range_ii_u64;

static inline wuffs_base__range_ii_u64  //
wuffs_base__make_range_ii_u64(uint64_t min_incl, uint64_t max_incl) {
  wuffs_base__range_ii_u64 ret;
  ret.min_incl = min_incl;
  ret.max_incl = max_incl;
  return ret;
}

static inline bool  //
wuffs_base__range_ii_u64__is_empty(const wuffs_base__range_ii_u64* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u64__equals(const wuffs_base__range_ii_u64* r,
                                 wuffs_base__range_ii_u64 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u64__is_empty(r) &&
          wuffs_base__range_ii_u64__is_empty(&s));
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__intersect(const wuffs_base__range_ii_u64* r,
                                    wuffs_base__range_ii_u64 s) {
  wuffs_base__range_ii_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u64__min(r->max_incl, s.max_incl);
  return t;
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__unite(const wuffs_base__range_ii_u64* r,
                                wuffs_base__range_ii_u64 s) {
  if (wuffs_base__range_ii_u64__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ii_u64__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ii_u64 t;
  t.min_incl = wuffs_base__u64__min(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u64__max(r->max_incl, s.max_incl);
  return t;
}

static inline bool  //
wuffs_base__range_ii_u64__contains(const wuffs_base__range_ii_u64* r,
                                   uint64_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline bool  //
wuffs_base__range_ii_u64__contains_range(const wuffs_base__range_ii_u64* r,
                                         wuffs_base__range_ii_u64 s) {
  return wuffs_base__range_ii_u64__equals(
      &s, wuffs_base__range_ii_u64__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u64::is_empty() const {
  return wuffs_base__range_ii_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u64::equals(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__equals(this, s);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::intersect(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__intersect(this, s);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::unite(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__unite(this, s);
}

inline bool  //
wuffs_base__range_ii_u64::contains(uint64_t x) const {
  return wuffs_base__range_ii_u64__contains(this, x);
}

inline bool  //
wuffs_base__range_ii_u64::contains_range(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__contains_range(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u64__struct {
  uint64_t min_incl;
  uint64_t max_excl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ie_u64__struct s) const;
  inline wuffs_base__range_ie_u64__struct intersect(
      wuffs_base__range_ie_u64__struct s) const;
  inline wuffs_base__range_ie_u64__struct unite(
      wuffs_base__range_ie_u64__struct s) const;
  inline bool contains(uint64_t x) const;
  inline bool contains_range(wuffs_base__range_ie_u64__struct s) const;
  inline uint64_t length() const;
#endif  // __cplusplus

} wuffs_base__range_ie_u64;

static inline wuffs_base__range_ie_u64  //
wuffs_base__make_range_ie_u64(uint64_t min_incl, uint64_t max_excl) {
  wuffs_base__range_ie_u64 ret;
  ret.min_incl = min_incl;
  ret.max_excl = max_excl;
  return ret;
}

static inline bool  //
wuffs_base__range_ie_u64__is_empty(const wuffs_base__range_ie_u64* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u64__equals(const wuffs_base__range_ie_u64* r,
                                 wuffs_base__range_ie_u64 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u64__is_empty(r) &&
          wuffs_base__range_ie_u64__is_empty(&s));
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__intersect(const wuffs_base__range_ie_u64* r,
                                    wuffs_base__range_ie_u64 s) {
  wuffs_base__range_ie_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u64__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__unite(const wuffs_base__range_ie_u64* r,
                                wuffs_base__range_ie_u64 s) {
  if (wuffs_base__range_ie_u64__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ie_u64__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ie_u64 t;
  t.min_incl = wuffs_base__u64__min(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u64__max(r->max_excl, s.max_excl);
  return t;
}

static inline bool  //
wuffs_base__range_ie_u64__contains(const wuffs_base__range_ie_u64* r,
                                   uint64_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline bool  //
wuffs_base__range_ie_u64__contains_range(const wuffs_base__range_ie_u64* r,
                                         wuffs_base__range_ie_u64 s) {
  return wuffs_base__range_ie_u64__equals(
      &s, wuffs_base__range_ie_u64__intersect(r, s));
}

static inline uint64_t  //
wuffs_base__range_ie_u64__length(const wuffs_base__range_ie_u64* r) {
  return wuffs_base__u64__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u64::is_empty() const {
  return wuffs_base__range_ie_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u64::equals(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__equals(this, s);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::intersect(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__intersect(this, s);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::unite(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__unite(this, s);
}

inline bool  //
wuffs_base__range_ie_u64::contains(uint64_t x) const {
  return wuffs_base__range_ie_u64__contains(this, x);
}

inline bool  //
wuffs_base__range_ie_u64::contains_range(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__contains_range(this, s);
}

inline uint64_t  //
wuffs_base__range_ie_u64::length() const {
  return wuffs_base__range_ie_u64__length(this);
}

#endif  // __cplusplus

// --------

// wuffs_base__rect_ii_u32 is a rectangle (a 2-dimensional range) on the
// integer grid. The "ii" means that the bounds are inclusive on the low end
// and inclusive on the high end. It contains all points (x, y) such that
// ((min_incl_x <= x) && (x <= max_incl_x)) and likewise for y.
//
// It is valid for min > max, in which case the rectangle is empty. There are
// multiple representations of an empty rectangle.
//
// The X and Y axes increase right and down.
typedef struct wuffs_base__rect_ii_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_incl_x;
  uint32_t max_incl_y;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__rect_ii_u32__struct s) const;
  inline wuffs_base__rect_ii_u32__struct intersect(
      wuffs_base__rect_ii_u32__struct s) const;
  inline wuffs_base__rect_ii_u32__struct unite(
      wuffs_base__rect_ii_u32__struct s) const;
  inline bool contains(uint32_t x, uint32_t y) const;
  inline bool contains_rect(wuffs_base__rect_ii_u32__struct s) const;
#endif  // __cplusplus

} wuffs_base__rect_ii_u32;

static inline wuffs_base__rect_ii_u32  //
wuffs_base__make_rect_ii_u32(uint32_t min_incl_x,
                             uint32_t min_incl_y,
                             uint32_t max_incl_x,
                             uint32_t max_incl_y) {
  wuffs_base__rect_ii_u32 ret;
  ret.min_incl_x = min_incl_x;
  ret.min_incl_y = min_incl_y;
  ret.max_incl_x = max_incl_x;
  ret.max_incl_y = max_incl_y;
  return ret;
}

static inline bool  //
wuffs_base__rect_ii_u32__is_empty(const wuffs_base__rect_ii_u32* r) {
  return (r->min_incl_x > r->max_incl_x) || (r->min_incl_y > r->max_incl_y);
}

static inline bool  //
wuffs_base__rect_ii_u32__equals(const wuffs_base__rect_ii_u32* r,
                                wuffs_base__rect_ii_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_incl_x == s.max_incl_x && r->max_incl_y == s.max_incl_y) ||
         (wuffs_base__rect_ii_u32__is_empty(r) &&
          wuffs_base__rect_ii_u32__is_empty(&s));
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__intersect(const wuffs_base__rect_ii_u32* r,
                                   wuffs_base__rect_ii_u32 s) {
  wuffs_base__rect_ii_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_incl_x = wuffs_base__u32__min(r->max_incl_x, s.max_incl_x);
  t.max_incl_y = wuffs_base__u32__min(r->max_incl_y, s.max_incl_y);
  return t;
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__unite(const wuffs_base__rect_ii_u32* r,
                               wuffs_base__rect_ii_u32 s) {
  if (wuffs_base__rect_ii_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__rect_ii_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__rect_ii_u32 t;
  t.min_incl_x = wuffs_base__u32__min(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__min(r->min_incl_y, s.min_incl_y);
  t.max_incl_x = wuffs_base__u32__max(r->max_incl_x, s.max_incl_x);
  t.max_incl_y = wuffs_base__u32__max(r->max_incl_y, s.max_incl_y);
  return t;
}

static inline bool  //
wuffs_base__rect_ii_u32__contains(const wuffs_base__rect_ii_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x <= r->max_incl_x) && (r->min_incl_y <= y) &&
         (y <= r->max_incl_y);
}

static inline bool  //
wuffs_base__rect_ii_u32__contains_rect(const wuffs_base__rect_ii_u32* r,
                                       wuffs_base__rect_ii_u32 s) {
  return wuffs_base__rect_ii_u32__equals(
      &s, wuffs_base__rect_ii_u32__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ii_u32::is_empty() const {
  return wuffs_base__rect_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ii_u32::equals(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__equals(this, s);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::intersect(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__intersect(this, s);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::unite(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__unite(this, s);
}

inline bool  //
wuffs_base__rect_ii_u32::contains(uint32_t x, uint32_t y) const {
  return wuffs_base__rect_ii_u32__contains(this, x, y);
}

inline bool  //
wuffs_base__rect_ii_u32::contains_rect(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__contains_rect(this, s);
}

#endif  // __cplusplus

// --------

// wuffs_base__rect_ie_u32 is a rectangle (a 2-dimensional range) on the
// integer grid. The "ie" means that the bounds are inclusive on the low end
// and exclusive on the high end. It contains all points (x, y) such that
// ((min_incl_x <= x) && (x < max_excl_x)) and likewise for y.
//
// It is valid for min >= max, in which case the rectangle is empty. There are
// multiple representations of an empty rectangle, including a value with all
// fields zero.
//
// The X and Y axes increase right and down.
typedef struct wuffs_base__rect_ie_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_excl_x;
  uint32_t max_excl_y;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__rect_ie_u32__struct s) const;
  inline wuffs_base__rect_ie_u32__struct intersect(
      wuffs_base__rect_ie_u32__struct s) const;
  inline wuffs_base__rect_ie_u32__struct unite(
      wuffs_base__rect_ie_u32__struct s) const;
  inline bool contains(uint32_t x, uint32_t y) const;
  inline bool contains_rect(wuffs_base__rect_ie_u32__struct s) const;
  inline uint32_t width() const;
  inline uint32_t height() const;
#endif  // __cplusplus

} wuffs_base__rect_ie_u32;

static inline wuffs_base__rect_ie_u32  //
wuffs_base__make_rect_ie_u32(uint32_t min_incl_x,
                             uint32_t min_incl_y,
                             uint32_t max_excl_x,
                             uint32_t max_excl_y) {
  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = min_incl_x;
  ret.min_incl_y = min_incl_y;
  ret.max_excl_x = max_excl_x;
  ret.max_excl_y = max_excl_y;
  return ret;
}

static inline bool  //
wuffs_base__rect_ie_u32__is_empty(const wuffs_base__rect_ie_u32* r) {
  return (r->min_incl_x >= r->max_excl_x) || (r->min_incl_y >= r->max_excl_y);
}

static inline bool  //
wuffs_base__rect_ie_u32__equals(const wuffs_base__rect_ie_u32* r,
                                wuffs_base__rect_ie_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_excl_x == s.max_excl_x && r->max_excl_y == s.max_excl_y) ||
         (wuffs_base__rect_ie_u32__is_empty(r) &&
          wuffs_base__rect_ie_u32__is_empty(&s));
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__intersect(const wuffs_base__rect_ie_u32* r,
                                   wuffs_base__rect_ie_u32 s) {
  wuffs_base__rect_ie_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_excl_x = wuffs_base__u32__min(r->max_excl_x, s.max_excl_x);
  t.max_excl_y = wuffs_base__u32__min(r->max_excl_y, s.max_excl_y);
  return t;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__unite(const wuffs_base__rect_ie_u32* r,
                               wuffs_base__rect_ie_u32 s) {
  if (wuffs_base__rect_ie_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__rect_ie_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__rect_ie_u32 t;
  t.min_incl_x = wuffs_base__u32__min(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__min(r->min_incl_y, s.min_incl_y);
  t.max_excl_x = wuffs_base__u32__max(r->max_excl_x, s.max_excl_x);
  t.max_excl_y = wuffs_base__u32__max(r->max_excl_y, s.max_excl_y);
  return t;
}

static inline bool  //
wuffs_base__rect_ie_u32__contains(const wuffs_base__rect_ie_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x < r->max_excl_x) && (r->min_incl_y <= y) &&
         (y < r->max_excl_y);
}

static inline bool  //
wuffs_base__rect_ie_u32__contains_rect(const wuffs_base__rect_ie_u32* r,
                                       wuffs_base__rect_ie_u32 s) {
  return wuffs_base__rect_ie_u32__equals(
      &s, wuffs_base__rect_ie_u32__intersect(r, s));
}

static inline uint32_t  //
wuffs_base__rect_ie_u32__width(const wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_x, r->min_incl_x);
}

static inline uint32_t  //
wuffs_base__rect_ie_u32__height(const wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_y, r->min_incl_y);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ie_u32::is_empty() const {
  return wuffs_base__rect_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ie_u32::equals(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__equals(this, s);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::intersect(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__intersect(this, s);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::unite(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__unite(this, s);
}

inline bool  //
wuffs_base__rect_ie_u32::contains(uint32_t x, uint32_t y) const {
  return wuffs_base__rect_ie_u32__contains(this, x, y);
}

inline bool  //
wuffs_base__rect_ie_u32::contains_rect(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__contains_rect(this, s);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::width() const {
  return wuffs_base__rect_ie_u32__width(this);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::height() const {
  return wuffs_base__rect_ie_u32__height(this);
}

#endif  // __cplusplus
