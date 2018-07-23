#include "cubee.h"

#include <map>

#include "engine.h"
#include "log.h"
#include "matrix.h"
#include "state.h"
#include "trace.h"

namespace {

Trace Trace30Cube(const Voxel v, const ProblemType pt) {
  const int yu = (pt == ProblemType::Reassembly || pt == ProblemType::Disassembly) ? 12 : 11;  // Y-up
  const int yd = (pt == ProblemType::Disassembly) ? 12 : 11;  // Y-down

  Trace trace;
  // 1 bot
  trace.push_back(CommandSMove { Vec3 {0, 15, 0} });  // 1: (0, 15, 0)[2-40]
  trace.push_back(CommandFission { Vec3 {0, 1, 0}, 19 }); // 2: (0, 16, 0)[3-21], 1:[22-40]
  // 2 bots
  trace.push_back(CommandSMove { Vec3 {0, -15, 0} });  // 1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, yu, 0} });   // 2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });   // 1: (15, 0, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });   // 2: (15, 27, 0)
  trace.push_back(CommandFission { Vec3 {1, 0, 0}, 9 });  // 22: (16, 0, 0)[23-31], 1:[32-40]
  trace.push_back(CommandFission { Vec3 {1, 0, 0}, 9 });  // 3: (16, 27, 0)[4-12], 2:[13-21]
  // 4 bots
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });  //  1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });  //  2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {13, 0, 0} });   //  3: (29, 27, 0)
  trace.push_back(CommandSMove { Vec3 {13, 0, 0} });   // 22: (29, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  1: (0, 0, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  2: (0, 27, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  3: (29, 27, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  // 22: (29, 0, 15)
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 4 });  // 32: (0, 0, 16)[33-36], 1:[37-40]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 4 });  // 13: (0, 27, 16)[14-17], 2:[18-21]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 4 });  // 4: (29, 27, 16)[5-8], 3:[9-12]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 4 });  // 23: (29, 0, 15)[24-27], 22:[2-31]
  // 8 bots
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  3: (29, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   //  4: (29, 27, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 13: (0, 27, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  // 22: (29, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 23: (29, 0, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 32: (0, 0, 29)
  if (pt == ProblemType::Disassembly) {
    trace.push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { 27, 28, 27} });  //  1
    trace.push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { 27,-28, 27} });  //  2
    trace.push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-27,-28, 27} });  //  3
    trace.push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-27,-28,-27} });  //  4
    trace.push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { 27,-28,-27} });  // 13
    trace.push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-27, 28, 27} });  // 22
    trace.push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-27, 28,-27} });  // 23
    trace.push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { 27, 28,-27} });  // 32
  }
  if (v == Voxel::Full) {
    if (pt == ProblemType::Reassembly) {
      trace.push_back(CommandWait {});  // 1
      trace.push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { 27, 0, 27} });  //  2
      trace.push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-27, 0, 27} });  //  3
      trace.push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-27, 0,-27} });  //  4
      trace.push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { 27, 0,-27} });  // 13
      trace.push_back(CommandWait {}); // 22
      trace.push_back(CommandWait {}); // 23
      trace.push_back(CommandWait {}); // 32

      trace.push_back(CommandWait {});
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandWait {});
      trace.push_back(CommandWait {});
      trace.push_back(CommandWait {});
    }
    if (pt == ProblemType::Assembly || pt == ProblemType::Reassembly) {
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27, 27, 27} });  //  1
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27,-27, 27} });  //  2
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27,-27, 27} });  //  3
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27,-27,-27} });  //  4
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27,-27,-27} });  // 13
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27, 27, 27} });  // 22
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27, 27,-27} });  // 23
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27, 27,-27} });  // 32
    }
  } else {
    if (pt == ProblemType::Reassembly) {
      trace.push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { 27, 28, 27} });  //  1
      trace.push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { 27,-28, 27} });  //  2
      trace.push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-27,-28, 27} });  //  3
      trace.push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-27,-28,-27} });  //  4
      trace.push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { 27,-28,-27} });  // 13
      trace.push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-27, 28, 27} });  // 22
      trace.push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-27, 28,-27} });  // 23
      trace.push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { 27, 28,-27} });  // 32

      trace.push_back(CommandWait {});
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandSMove { Vec3 {0, -1, 0} });
      trace.push_back(CommandWait {});
      trace.push_back(CommandWait {});
      trace.push_back(CommandWait {});
    }
    if (pt == ProblemType::Assembly || pt == ProblemType::Reassembly) {
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27, 27, 0} });  //  1
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27,-27, 0} });  //  2
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27,-27, 0} });  //  3
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27,-27, 0} });  //  4
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27,-27, 0} });  // 13
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27, 27, 0} });  // 22
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27, 27, 0} });  // 23
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27, 27, 0} });  // 32
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 0, 27, 27} });  //  1
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 0,-27, 27} });  //  2
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 { 0,-27, 27} });  //  3
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 { 0,-27,-27} });  //  4
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 0,-27,-27} });  // 13
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 { 0, 27, 27} });  // 22
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 { 0, 27,-27} });  // 23
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 0, 27,-27} });  // 32
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27, 0, 27} });  //  1
      trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27, 0, 27} });  //  2
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27, 0, 27} });  //  3
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27, 0,-27} });  //  4
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27, 0,-27} });  // 13
      trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27, 0, 27} });  // 22
      trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27, 0,-27} });  // 23
      trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27, 0,-27} });  // 32
    }
  }
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  1
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  2
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  3
  trace.push_back(CommandSMove { Vec3 {0, 0, -13} });   //  4
  trace.push_back(CommandSMove { Vec3 {0, 0, -13} });   // 13
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  // 22
  trace.push_back(CommandSMove { Vec3 {0, 0, -13} });   // 23
  trace.push_back(CommandSMove { Vec3 {0, 0, -13} });   // 32
  trace.push_back(CommandFusionP { Vec3 {0, 0, 1} });  //  1: (0, 0, 15)
  trace.push_back(CommandFusionP { Vec3 {0, 0, 1} });  //  2: (0, 27, 15)
  trace.push_back(CommandFusionP { Vec3 {0, 0, 1} });  //  3: (29, 27, 15)
  trace.push_back(CommandFusionS { Vec3 {0, 0, -1} });   //  4->3
  trace.push_back(CommandFusionS { Vec3 {0, 0, -1} });   // 13->2
  trace.push_back(CommandFusionP { Vec3 {0, 0, 1} });  // 22: (29, 0, 15)
  trace.push_back(CommandFusionS { Vec3 {0, 0, -1} });   // 23->22
  trace.push_back(CommandFusionS { Vec3 {0, 0, -1} });   // 32->1
  // 4 bots
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  3: (29, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  // 22: (29, 0, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });  //  1: (15, 0, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });  //  2: (15, 27, 0)
  trace.push_back(CommandSMove { Vec3 {-13, 0, 0} });   //  3: (16, 27, 0)
  trace.push_back(CommandSMove { Vec3 {-13, 0, 0} });   // 22: (16, 0, 0)
  trace.push_back(CommandFusionP { Vec3 {1, 0, 0} });  //  1: (15, 0, 0)
  trace.push_back(CommandFusionP { Vec3 {1, 0, 0} });  //  2: (15, 27, 0)
  trace.push_back(CommandFusionS { Vec3 {-1, 0, 0} });  //  3->2
  trace.push_back(CommandFusionS { Vec3 {-1, 0, 0} });  // 22->1
  // 2 bots
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });   // 1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });   // 2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 15, 0} });  // 1: (0, 15, 0)
  trace.push_back(CommandSMove { Vec3 {0, -yd, 0} });   // 2: (0, 16, 0)
  trace.push_back(CommandFusionP { Vec3 {0, 1, 0} });  //  1: (0, 15, 0)
  trace.push_back(CommandFusionS { Vec3 {0, -1, 0} });  //  2->1
  // 1 bot
  trace.push_back(CommandSMove { Vec3 {0, -15, 0} });  // 1: (0, 0, 0)
  trace.push_back(CommandHalt {});  // 1: (0, 0, 0)

  return trace;
}

Trace solver(ProblemType problem_type, const Matrix& source, const Matrix& target) {
  ASSERT_RETURN(problem_type == ProblemType::Disassembly, Trace());
  ASSERT_RETURN(source.R == 30, Trace());
  return Trace30Cube(Voxel::Full, ProblemType::Disassembly);
}

}  // namespace

REGISTER_ENGINE(cubee_classic, solver);
