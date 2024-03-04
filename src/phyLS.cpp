/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "store.hpp"
#include "commands/balance.hpp"
#include "commands/exprsim.hpp"
#include "commands/load.hpp"
#include "commands/lut_mapping.hpp"
#include "commands/node_resynthesis.hpp"
#include "commands/resub.hpp"
#include "commands/sim.hpp"
#include "commands/stpsat.hpp"
#include "commands/techmap.hpp"
#include "commands/write_dot.hpp"
#include "commands/refactor.hpp"
#include "commands/cut_rewriting.hpp"
#include "commands/cec.hpp"
#include "commands/functional_reduction.hpp"
#include "commands/convert_graph.hpp"
#include "commands/lutmap.hpp"
#include "commands/abc/read.hpp"
#include "commands/abc/write.hpp"
#include "commands/abc/strash.hpp"
#include "commands/abc/resub.hpp"
#include "commands/abc/fraig.hpp"
#include "commands/abc/&get.hpp"
#include "commands/abc/&fraig.hpp"
#include "commands/abc/comb.hpp"
#include "commands/simulator.hpp"
#include "commands/abc/cec.hpp"
#include "commands/stpfr.hpp"
#include "commands/exact/exact_lut.hpp"
#include "commands/exact/lutrw.hpp"
#include "commands/abc/if.hpp"
#include "commands/abc/map.hpp"
#include "commands/window_rewriting.hpp"
#include "commands/ps2.hpp"
#include "commands/xmg/xmginv.hpp"
#include "commands/abc/read_genlib.hpp"
#include "commands/abc/&nf.hpp"
#include "commands/abc/&if.hpp"
#include "commands/abc/put.hpp"
#include "commands/abc/amap.hpp"
#include "commands/abc/compress2rs.hpp"
#include "commands/abc/resyn2rs.hpp"
#include "commands/abc/dc2.hpp"
#include "commands/abc/sop.hpp"
#include "commands/abc/balance.hpp"
#include "commands/abc/refactor.hpp"
#include "commands/abc/rewrite.hpp"
#include "commands/xag/rm.hpp"
#include "commands/xag/rm2.hpp"
#include "commands/xag/xagrw.hpp"
#include "commands/xag/xagrs.hpp"
#include "commands/xmg/xmgrs.hpp"
#include "commands/xmg/xmgrw.hpp"
#include "commands/exact/exact_multi.hpp"
// #include "commands/exact/exact_klut.hpp"
// #include "commands/exact/exactlut.hpp"
#include "commands/to_npz.hpp"

ALICE_MAIN(phyLS)