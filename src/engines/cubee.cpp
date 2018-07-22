#include "cubee.h"

#include "engine.h"
#include "log.h"
#include "matrix.h"
#include "state.h"
#include "trace.h"

bool Is30Cube(const Matrix& m, const Voxel fv) {
  if (m.R != 30)
    return false;

  constexpr int R = 30;
  for (int y = 0; y < R - 2; ++y) {
    for (int x = 1; x < R - 1; ++x) {
      for (int z = 1; z < R - 1; ++z) {
        Voxel v = m(x, y, z);
        if (y == 0 || y == R - 3 || x == 1 || x == R - 2 || z == 1 || z == R - 2) {
          if (v == Voxel::Void) {
            return false;
          }
        } else {
          if(v != fv) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

Trace Trace30Cube(const Voxel v) {
  Trace trace;
  // 1 bot
  trace.push_back(CommandSMove { Vec3 {0, 15, 0} });  // 1: (0, 15, 0)[2-40]
  trace.push_back(CommandFission { Vec3 {0, 1, 0}, 20 }); // 2: (0, 16, 0)[3-21], 1:[22-40]
  // 2 bots
  trace.push_back(CommandSMove { Vec3 {0, -15, 0} });  // 1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 11, 0} });   // 2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });   // 1: (15, 0, 0)
  trace.push_back(CommandSMove { Vec3 {15, 0, 0} });   // 2: (15, 27, 0)
  trace.push_back(CommandFission { Vec3 {1, 0, 0}, 10 });  // 22: (16, 0, 0)[23-31], 1:[32-40]
  trace.push_back(CommandFission { Vec3 {1, 0, 0}, 10 });  // 3: (16, 27, 0)[4-12], 2:[13-21]
  // 4 bots
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });  //  1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {-15, 0, 0} });  //  2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {13, 0, 0} });   //  3: (29, 27, 0)
  trace.push_back(CommandSMove { Vec3 {13, 0, 0} });   // 22: (29, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  1: (0, 0, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  2: (0, 27, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  //  3: (29, 27, 15)
  trace.push_back(CommandSMove { Vec3 {0, 0, 15} });  // 22: (29, 0, 15)
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 5 });  // 32: (0, 0, 16)[33-36], 1:[37-40]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 5 });  // 13: (0, 27, 16)[14-17], 2:[18-21]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 5 });  // 4: (29, 27, 16)[5-8], 3:[9-12]
  trace.push_back(CommandFission { Vec3 {0, 0, 1}, 5 });  // 23: (29, 0, 15)[24-27], 22:[2-31]
  // 8 bots
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  1: (0, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  2: (0, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  //  3: (29, 27, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   //  4: (29, 27, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 13: (0, 27, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, -15} });  // 22: (29, 0, 0)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 23: (29, 0, 29)
  trace.push_back(CommandSMove { Vec3 {0, 0, 13} });   // 32: (0, 0, 29)
  if (v == Voxel::Full) {
    trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27, 27, 27} });  //  1
    trace.push_back(CommandGFill { Vec3 { 1, 0, 1}, Vec3 { 27,-27, 27} });  //  2
    trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27,-27, 27} });  //  3
    trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27,-27,-27} });  //  4
    trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27,-27,-27} });  // 13
    trace.push_back(CommandGFill { Vec3 {-1, 0, 1}, Vec3 {-27, 27, 27} });  // 22
    trace.push_back(CommandGFill { Vec3 {-1, 0,-1}, Vec3 {-27, 27,-27} });  // 23
    trace.push_back(CommandGFill { Vec3 { 1, 0,-1}, Vec3 { 27, 27,-27} });  // 32
  } else {
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
  trace.push_back(CommandSMove { Vec3 {0, -11, 0} });   // 2: (0, 16, 0)
  trace.push_back(CommandFusionP { Vec3 {0, 1, 0} });  //  1: (0, 15, 0)
  trace.push_back(CommandFusionS { Vec3 {0, -1, 0} });  //  2->1
  // 1 bot
  trace.push_back(CommandSMove { Vec3 {0, -15, 0} });  // 1: (0, 0, 0)
  trace.push_back(CommandHalt {});  // 1: (0, 0, 0)

  return trace;
}

Trace AssemblySolver(const Matrix& matrix) {
  if (Is30Cube(matrix, Voxel::Full))
    return Trace30Cube(Voxel::Full);
  if (Is30Cube(matrix, Voxel::Void))
    return Trace30Cube(Voxel::Void);
  return {};
}

bool IsEqual(const Matrix& a, const Matrix& b) {
  const int R = a.R;
  for (int x = 0; x < R; ++x)
    for (int y = 0; y < R; ++y)
      for (int z = 0; z < R; ++z)
        if (a(x, y, z) != b(x, y, z))
          return false;
  LOG();
  return true;
}

Trace cubee(ProblemType problem_type, const Matrix& source, const Matrix& target) {
  if (IsEqual(source, target)) {
    Trace trace;
    trace.push_back(CommandHalt {});
    return trace;
  }

  switch (problem_type) {
  case ProblemType::Assembly:
    return AssemblySolver(target);
  case ProblemType::Disassembly:
  case ProblemType::Reassembly:
  case ProblemType::Invalid:
    return {};
  }
  ASSERT(false);
  return {};
}

REGISTER_ENGINE(cube, cubee);
