// Copyright 2017 The Puffs Authors.
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

package cgen

// This file hard-codes PUFFS_USE_NO_OP_PERFORMANCE_HACKS code fragments that
// look like redundant no-ops, but for reasons to be investigated, can have
// dramatic performance effects with gcc 4.8.4 (e.g. 1.2x on some benchmarks).
//
// TODO: investigate; delete these hacks (without regressing performance).

// TODO: delete this file.

type nophKey struct {
	gCurrFunkCName string
	genMethod      string
	headerFooter   bool
}

var noOpPerformanceHacks = map[nophKey]string{
	{
		gCurrFunkCName: "puffs_gif__decoder__decode_id",
		genMethod:      "writeSaveDerivedVar",
		headerFooter:   false,
	}: `
#ifdef PUFFS_USE_NO_OP_PERFORMANCE_HACKS
if (a_src.private_impl.no_op_performance_hacks) {
  puffs_base__paired_nulls* pn =
      a_src.private_impl.no_op_performance_hacks->noph0;
  while (pn && pn->always_null0) {
    *((int*)(pn->always_null0)) = 0;
    pn = (puffs_base__paired_nulls*)(pn->always_null1);
  }
}
#endif
`,
	{
		gCurrFunkCName: "puffs_gif__lzw_decoder__decode",
		genMethod:      "writeLoadDerivedVar",
		headerFooter:   true,
	}: `
#ifdef PUFFS_USE_NO_OP_PERFORMANCE_HACKS
if (a_src.private_impl.no_op_performance_hacks) {
  if (a_src.private_impl.no_op_performance_hacks->noph1 &&
      (len > a_src.private_impl.no_op_performance_hacks->noph1)) {
    len = a_src.private_impl.no_op_performance_hacks->noph1;
  }
}
#endif
`,
}
