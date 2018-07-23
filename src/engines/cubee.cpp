#include "cubee.h"

#include <map>

#include "engine.h"
#include "log.h"
#include "matrix.h"
#include "state.h"
#include "trace.h"

namespace {

Trace TraceSerializer(std::vector<Trace>& trace_2d) {
  std::vector<std::pair<int,int>> id_map = {
    { 1,  0}, { 2,  1}, {22,  2}, { 3,  3}, {32,  4}, {13,  5}, {23,  6}, { 4,  7},
    {33,  8}, {14,  9}, {24, 10}, { 5, 11}, {34, 12}, {15, 13}, {25, 14}, { 6, 15},
    {35, 16}, {16, 17}, {26, 18}, { 7, 19}, {36, 20}, {17, 21}, {27, 22}, { 8, 23},
    {37, 24}, {18, 25}, {28, 26}, { 9, 27}, {38, 28}, {19, 29}, {29, 30}, {10, 31},
    {39, 32}, {20, 33}, {30, 34}, {11, 35}, {40, 36}, {21, 37}, {31, 38}, {12, 39},
  };

  int num_active = 1;
  Trace trace;
  while (trace_2d[0].size()) {
    int add = 0;
    for (int i = 0; i < num_active; ++i) {
      Trace& t = trace_2d[id_map[i].second];
      Command cmd = t.front(); t.pop_front();
      trace.push_back(cmd);
      if (cmd.type() == typeid(CommandFission))
        ++add;
    }
    if (add) {
      num_active += add;
      std::sort(id_map.begin(), id_map.begin() + num_active);
    }
  }
  int rem = 0;
  for (int i = 0; i < trace_2d.size(); ++i) {
    if (trace_2d[i].size()) {
      LOG() << "[" << i << "]: " << trace_2d[i].size() << " commands remained\n";
      rem += trace_2d[i].size();
    }
  }
  if (rem)
    LOG() << "rem: " << rem << "\n";

  return trace;
}

void InitTower(const Matrix& matrix, std::vector<Trace>& trace_2d) {
  const int R = matrix.R;
  trace_2d[0].push_back(CommandFission { Vec3 {1, 0, 0}, 19}); // 1 [22-40]

  int x, y, z;
  for (x = 1; x < std::min(R - 1, 31);) {
    trace_2d[0].push_back(CommandWait {});  // 1 [22-40]
    int diff = std::min(R - 1, 31) - x;
    trace_2d[1].push_back(CommandSMove { Vec3 {diff, 0, 0} }); // 2 [3-21]
    x += diff;
  }

  if (matrix(x, 0, 1) == Voxel::Full) {
    trace_2d[0].push_back(CommandWait {});  // 1
    trace_2d[1].push_back(CommandVoid { Vec3 {0, 0, 1} });  // 2
  }
  trace_2d[0].push_back(CommandFission { Vec3 {0, 0, 1}, 9});  // 1 [22-40]
  trace_2d[1].push_back(CommandFission { Vec3 {0, 0, 1}, 9});  // 2 [3-21]
  for (z = 1; z < std::min(R - 1, 31); ++z) {
    if (matrix(x, 0, z + 1) == Voxel::Full) {
      trace_2d[0].push_back(CommandWait {});  // 1 [32-40]
      trace_2d[1].push_back(CommandWait {});  // 2 [13-21]
      trace_2d[2].push_back(CommandWait {});  // 22 [23-31]
      trace_2d[3].push_back(CommandVoid { Vec3 {0, 0, 1} });  // 3 [4-12]
    }
    trace_2d[0].push_back(CommandWait {});  // 1
    trace_2d[1].push_back(CommandWait {});  // 2
    trace_2d[2].push_back(CommandSMove { Vec3 {0, 0, 1} });  // 22 [23-31]
    trace_2d[3].push_back(CommandSMove { Vec3 {0, 0, 1} });  // 3 [4-12]
  }

  if (matrix(x, 1, z) == Voxel::Full) {
    trace_2d[0].push_back(CommandWait {});  // 1
    trace_2d[1].push_back(CommandWait {});  // 2
    trace_2d[2].push_back(CommandWait {});  // 22
    trace_2d[3].push_back(CommandVoid { Vec3 {0, 1, 0} });  // 3 [4-12]
  }
  trace_2d[0].push_back(CommandFission { Vec3 {0, 1, 0}, 8});  // 1 [32-40]
  trace_2d[1].push_back(CommandFission { Vec3 {0, 1, 0}, 8});  // 2 [13-21]
  trace_2d[2].push_back(CommandFission { Vec3 {0, 1, 0}, 8});  // 22 [23-31]
  trace_2d[3].push_back(CommandFission { Vec3 {0, 1, 0}, 8});  // 3 [4-12]

  for (y = 1; y < std::min(R - 2, 30); ++y) {
    if (matrix(x, y + 1, z) == Voxel::Full) {
      trace_2d[0].push_back(CommandWait {});  // 1
      trace_2d[1].push_back(CommandWait {});  // 2
      trace_2d[2].push_back(CommandWait {});  // 22
      trace_2d[3].push_back(CommandWait {});  // 3
      trace_2d[4].push_back(CommandWait {});  // 32
      trace_2d[5].push_back(CommandWait {});  // 13
      trace_2d[6].push_back(CommandWait {});  // 23
      trace_2d[7].push_back(CommandVoid { Vec3 {0, 1, 0} });  // 4
    }
    trace_2d[0].push_back(CommandWait {});  // 1 [37-40]
    trace_2d[1].push_back(CommandWait {});  // 2 [18-21]
    trace_2d[2].push_back(CommandWait {});  // 22 [28-31]
    trace_2d[3].push_back(CommandWait {});  // 3 [9-12]
    trace_2d[4].push_back(CommandSMove { Vec3 {0, 1, 0} });  // 32 [33-36]
    trace_2d[5].push_back(CommandSMove { Vec3 {0, 1, 0} });  // 13 [14-17]
    trace_2d[6].push_back(CommandSMove { Vec3 {0, 1, 0} });  // 23 [24-27]
    trace_2d[7].push_back(CommandSMove { Vec3 {0, 1, 0} });  // 4 [5-8]
  }

  int num_bots = 8;
  while (y < R - 2) {
    if (matrix(x, 1, z) == Voxel::Full) {
      for (int i = 0; i < num_bots - 1; ++i)
        trace_2d[i].push_back(CommandWait {});
      trace_2d[num_bots - 1].push_back(CommandVoid { Vec3 {0, 1, 0} });
    }

    int m = 9 - (num_bots / 4);
    for (int i = 0; i < num_bots - 4; ++i)
      trace_2d[i].push_back(CommandWait {});
    trace_2d[num_bots - 4].push_back(CommandFission { Vec3 {0, 1, 0}, m});
    trace_2d[num_bots - 3].push_back(CommandFission { Vec3 {0, 1, 0}, m});
    trace_2d[num_bots - 2].push_back(CommandFission { Vec3 {0, 1, 0}, m});
    trace_2d[num_bots - 1].push_back(CommandFission { Vec3 {0, 1, 0}, m});
    ++y;
    num_bots += 4;

    int dy = 1;
    for (; y < R - 2 && dy < 30; ++y, ++dy) {
      if (matrix(x, y + 1, z) == Voxel::Full) {
        for (int i = 0; i < num_bots - 1; ++i)
          trace_2d[i].push_back(CommandWait {});
        trace_2d[num_bots - 1].push_back(CommandVoid { Vec3 {0, 1, 0} });
      }
      for (int i = 0; i < num_bots - 4; ++i)
        trace_2d[i].push_back(CommandWait {});
      for (int i = num_bots - 4; i < num_bots; ++i)
        trace_2d[i].push_back(CommandSMove { Vec3 {0, 1, 0} });
    }
  }

  trace_2d.resize(num_bots);
}

int Sum(std::vector<Trace>& t2) {
  int n = 0;
  for (auto& t : t2)
    n += t.size();
  return n;
}

Trace GVoider(const Matrix& matrix) {
  const int R = matrix.R;
  const Region region { Vec3 { 0, 0, 0 }, { Vec3 { R, R, R } } };
  std::vector<Trace> trace_2d(k_MaxNumberOfBots);
  InitTower(matrix, trace_2d);
  const int N = trace_2d.size();

  auto void_tower = [R, N](int dx, int dz, std::vector<Trace>& trace_2d) {
    for (int i = 0; i < N - 7; i += 8) {
      int y0 = std::min(R - 2, (i / 8) * 60), y1 = std::min(R - 2, y0 + 30);
      int dy = y1 - y0 + 1;
      trace_2d[i + 0].push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { dx, dy, dz} });
      trace_2d[i + 1].push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-dx, dy, dz} });
      trace_2d[i + 2].push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { dx, dy,-dz} });
      trace_2d[i + 3].push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-dx, dy,-dz} });
      trace_2d[i + 4].push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { dx,-dy, dz} });
      trace_2d[i + 5].push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-dx,-dy, dz} });
      trace_2d[i + 6].push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { dx,-dy,-dz} });
      trace_2d[i + 7].push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-dx,-dy,-dz} });
    }
    if (N % 8) {
      trace_2d[N - 4].push_back(CommandWait {});
      trace_2d[N - 3].push_back(CommandWait {});
      trace_2d[N - 2].push_back(CommandWait {});
      trace_2d[N - 1].push_back(CommandWait {});
    }

    trace_2d[0].push_back(CommandWait {});
    trace_2d[1].push_back(CommandWait {});
    trace_2d[2].push_back(CommandWait {});
    trace_2d[3].push_back(CommandWait {});
    for (int i = 4; i < N - 7; i += 8) {
      int y0 = (i / 8) * 60 + 30, y1 = std::min(R - 2, y0 + 30);
      int dy = y1 - y0 + 1;
      trace_2d[i + 0].push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { dx, dy, dz} });
      trace_2d[i + 1].push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-dx, dy, dz} });
      trace_2d[i + 2].push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { dx, dy,-dz} });
      trace_2d[i + 3].push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-dx, dy,-dz} });
      trace_2d[i + 4].push_back(CommandGVoid { Vec3 { 1, 0, 1}, Vec3 { dx,-dy, dz} });
      trace_2d[i + 5].push_back(CommandGVoid { Vec3 {-1, 0, 1}, Vec3 {-dx,-dy, dz} });
      trace_2d[i + 6].push_back(CommandGVoid { Vec3 { 1, 0,-1}, Vec3 { dx,-dy,-dz} });
      trace_2d[i + 7].push_back(CommandGVoid { Vec3 {-1, 0,-1}, Vec3 {-dx,-dy,-dz} });
    }
    if (N % 8 == 0) {
      trace_2d[N - 4].push_back(CommandWait {});
      trace_2d[N - 3].push_back(CommandWait {});
      trace_2d[N - 2].push_back(CommandWait {});
      trace_2d[N - 1].push_back(CommandWait {});
    }
  };

  int x0 = 0, x1 = std::min(R - 1, 31);
  int z0 = 0, z1 = std::min(R - 1, 31);
  int zsign = 1;
  while (true) {
    const int dx = x1 - x0 - 1;
    const int dz = z1 - z0 - 1;
    if (zsign > 0) {
      while (true) {
        void_tower(dx, dz, trace_2d);
        if (z1 == R - 1)
          break;
        int mvz = std::min(R - 1, z1 + 30) - z1;
        for (int zz = 0; zz < mvz; ++zz) {
          bool need_dig = false;
          for (int y = 0; y < R - 1; y += 30) {
            if (matrix(x0, y, z1 + 1) == Voxel::Full || matrix(x1, y, z1 + 1) == Voxel::Full)
              need_dig = true;
          }
          if (need_dig) {
            for (int y = 0, i = 0; i < N; i += 4, y += 30) {
              trace_2d[i + 0].push_back(CommandWait {});
              trace_2d[i + 1].push_back(CommandWait {});
              if (matrix(x0, y, z1 + 1) == Voxel::Full)
                trace_2d[i + 2].push_back(CommandVoid { Vec3 {0, 0, 1} });
              else
                trace_2d[i + 2].push_back(CommandWait {});

              if (matrix(x1, y, z1 + 1) == Voxel::Full)
                trace_2d[i + 3].push_back(CommandVoid { Vec3 {0, 0, 1} });
              else
                trace_2d[i + 3].push_back(CommandWait {});
            }
          }
          for (int i = 0; i < N; ++i) {
            trace_2d[i].push_back(CommandSMove { Vec3 {0, 0, 1} });
          }
          ++z0;
          ++z1;
        }
      }
    } else {
      while (true) {
        void_tower(dx, dz, trace_2d);
        if (z0 == 0)
          break;
        int mvz = std::min(z0, 30);
        for (int zz = 0; zz < mvz; ++zz) {
          bool need_dig = false;
          for (int y = 0; y < R - 1; y += 30) {
            if (matrix(x0, y, z0 - 1) == Voxel::Full || matrix(x1, y, z0 - 1) == Voxel::Full)
              need_dig = true;
          }
          if (need_dig) {
            for (int y = 0, i = 0; i < N; i += 4, y += 30) {
              if (matrix(x0, y, z0 - 1) == Voxel::Full)
                trace_2d[i + 0].push_back(CommandVoid { Vec3 {0, 0, -1} });
              else
                trace_2d[i + 0].push_back(CommandWait {});

              if (matrix(x1, y, z0 - 1) == Voxel::Full)
                trace_2d[i + 1].push_back(CommandVoid { Vec3 {0, 0, -1} });
              else
                trace_2d[i + 1].push_back(CommandWait {});
              trace_2d[i + 2].push_back(CommandWait {});
              trace_2d[i + 3].push_back(CommandWait {});
            }
          }
          for (int i = 0; i < N; ++i) {
            trace_2d[i].push_back(CommandSMove { Vec3 {0, 0, -1} });
          }
          --z0;
          --z1;
        }
      }
    }

    if (x1 == R - 1)
      break;

    int mvx = std::min(R - 1, x1 + 30) - x1;
    x0 += mvx;
    x1 += mvx;
    zsign = -zsign;
  }

  return TraceSerializer(trace_2d);
}

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

Trace AssemblySolver(const Matrix& matrix) {
  if (Is30Cube(matrix, Voxel::Full))
    return Trace30Cube(Voxel::Full, ProblemType::Assembly);
  if (Is30Cube(matrix, Voxel::Void))
    return Trace30Cube(Voxel::Void, ProblemType::Assembly);
  return {};
}

Trace DisassemblySolver(const Matrix& matrix) {
  if (matrix.R == 30)
    return Trace30Cube(Voxel::Full, ProblemType::Disassembly);
  return GVoider(matrix);
}

Trace ReassemblySolver(const Matrix& source, const Matrix& target) {
  if (Is30Cube(target, Voxel::Full))
    return Trace30Cube(Voxel::Full, ProblemType::Reassembly);
  if (Is30Cube(target, Voxel::Void))
    return Trace30Cube(Voxel::Void, ProblemType::Reassembly);
  return {};
}

Trace solver(ProblemType problem_type, const Matrix& source, const Matrix& target) {
  if (source == target) {
    Trace trace;
    trace.push_back(CommandHalt {});
    return trace;
  }

  switch (problem_type) {
  case ProblemType::Assembly:
    return AssemblySolver(target);
  case ProblemType::Disassembly:
    return DisassemblySolver(source);
  case ProblemType::Reassembly:
    return ReassemblySolver(source, target);
  case ProblemType::Invalid:
    return {};
  }
  ASSERT(false);
  return {};
}

}  // namespace

REGISTER_ENGINE(cubee, solver);
