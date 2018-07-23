#include "parallel_v3.h"

#include <utility>
#include <vector>
#include <cstdint>

#include "engine.h"
#include "engines/naive_converter.h"
#include "log.h"
#include "nmms.h"
#include "state.h"
#include "system.h"

using namespace std;

namespace {

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

using Boundary = pair<int64_t, vector<int>>;

void naive_move(const Vec3& destination, Vec3& position, Trace& trace) {
    const Vec3 dx = (destination.x > position.x) ? unitX : -unitX;
    while (position.x != destination.x) {
        trace.push_back(CommandSMove{dx});
        position += dx;
    }
    const Vec3 dy = (destination.y > position.y) ? unitY : -unitY;
    while (position.y != destination.y) {
        trace.push_back(CommandSMove{dy});
        position += dy;
    }
    const Vec3 dz = (destination.z > position.z) ? unitZ : -unitZ;
    while (position.z != destination.z) {
        trace.push_back(CommandSMove{dz});
        position += dz;
    }
}

Trace single_stupid_solver(const System& system, const Matrix& tgt_matrix,
                           const Vec3& lower_bound, const Vec3& upper_bound,
                           Vec3& position) {
    // calculate bounding box
    Vec3 lower = upper_bound - Vec3(1, 1, 1);
    Vec3 upper = lower_bound;
    for (int x = lower_bound.x; x < upper_bound.x; ++x) {
        for (int y = lower_bound.y; y < upper_bound.y; ++y) {
            for (int z = lower_bound.z; z < upper_bound.z; ++z) {
                if (tgt_matrix(Vec3(x, y, z))) {
                    lower.x = std::min(lower.x, x);
                    lower.y = std::min(lower.y, y);
                    lower.z = std::min(lower.z, z);
                    upper.x = std::max(upper.x, x + 1);
                    upper.y = std::max(upper.y, y + 1);
                    upper.z = std::max(upper.z, z + 1);
                }
            }
        }
    }
    if (lower.x >= upper.x) {
        return {};
    }

    // print
    Trace trace;
    set<Vec3> filled_set;
    naive_move(lower, position, trace);
    int dx = -1;
    int dz = -1;
    for (int y = lower.y + 1; y <= upper.y; ++y) {
        dz *= -1;
        trace.push_back(CommandSMove{unitY});
        position += unitY;
	int zratio = 2;
        for (int z = lower.z; z < upper.z; z+=zratio) {
            dx *= -1;
            if (z > lower.z) {
                trace.push_back(CommandSMove{Vec3(0, 0, dz*zratio)});
                position += Vec3(0, 0, dz*zratio);
            }
	    if( (position.z == upper.z - 2 && dz > 0) || (position.z == lower.z + 1 && dz < 0) ){
	      zratio = 1;
	    }
	    int xratio = 1;
            for(int x = lower.x; x < upper.x ; x+=xratio) {
	      if (x > lower.x) {
		trace.push_back(CommandSMove{Vec3(dx*xratio, 0, 0)});
		position += Vec3(dx*xratio, 0, 0);
	      }
	      for(auto v : {Vec3(0, -1, 1), Vec3(0, -1, -1), Vec3(1, -1, 0), Vec3(-1, -1, 0), Vec3(0, -1, 0)}){
		const auto pv = position + v;

		if(pv.x < lower.x || pv.x >= upper.x || pv.z < lower.z || pv.z >= upper.z){
		  continue;
		}

		if (tgt_matrix(pv) && filled_set.find(pv) == filled_set.end()) {
		  trace.push_back(CommandFill{v});
		  filled_set.insert(pv);
		}
	      }

	      if(dx > 0 && position.x % 2 == 0){
		xratio = 2;
	      }
	      if(dx < 0 && position.x % 2 == 1){
		xratio = 2;
	      }

	      if( (position.x == lower.x + 1 && dx < 0)  || (position.x == upper.x - 2 && dx > 0) ){
		xratio = 1;
	      }
            }
        }
    }

    return trace;
}

Boundary sepbound(const vector<int64_t> &cellnum, const int64_t lim, const int num){
  vector<int> out;
  out.push_back(0);
  out.push_back(1);

  int64_t cnt = 0;
  int64_t maxcnt = 0;

  bool boundnext = false;
  for(int x=1; x<=cellnum.size(); ++x){
    const int osiz = out.size();
    if(boundnext && osiz < num -1){
      out.push_back(x);
      if(cnt > maxcnt){
	maxcnt = cnt;
      }
      cnt = 0;
      boundnext = false;
    }else if(num + 1 - osiz == cellnum.size() - x + 1){
      out.push_back(x);
      if(cnt > maxcnt){
	maxcnt = cnt;
      }
      cnt = 0;
      boundnext = false;
    }

    cnt += cellnum[x];
    if(cnt > lim){
      boundnext = true;
    }
  }

  return Boundary(maxcnt, out);
}

int64_t getcellnumber(const Matrix &matrix, const int x, const int zlower, const int zupper){
  int64_t output = 0;
  for(int y=0;y<matrix.R;++y){
    for(int z=zlower;z<zupper;++z){
      if(matrix(x,y,z))
	++output;
    }
  }
  return output;
}

vector<int> getboundaries(const Matrix &matrix, int num, const int zlower, const int zupper){
  std::vector<int64_t> cellnumbers;
  std::vector<int> boundaries;
  int64_t sihyou = 0;
  const int R = matrix.R;
  for(int x=0; x<R; ++x){
    const int64_t cnum = getcellnumber(matrix, x, zlower, zupper);
    cellnumbers.push_back(cnum);
    sihyou += cnum;
  }

  // dist
  sihyou = (sihyou / num) + 1;

  int64_t cnt = cellnumbers[0];
  boundaries.push_back(0);
  boundaries.push_back(1);
  bool boundnext = false;
  for(int x=1; x<=R; ++x){
    if(boundnext){
      boundaries.push_back(x);
      cnt = 0;
      boundnext = false;
    }else if(num + 1 - boundaries.size() == R - x + 1){
      boundaries.push_back(x);
    }

    cnt += cellnumbers[x];
    if(cnt > sihyou){
      boundnext = true;
    }
  }
  return boundaries;
}

Trace solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) {
    if (problem_type == ProblemType::Disassembly) {
        return NaiveConverter::reverse(solver)(problem_type, src_matrix, tgt_matrix);
    } else if (problem_type == ProblemType::Reassembly) {
        return NaiveConverter::concatenate(NaiveConverter::reverse(solver), solver)(problem_type, src_matrix, tgt_matrix);
    }
    ASSERT_RETURN(problem_type == ProblemType::Assembly, Trace());

    System system(tgt_matrix.R);
    Trace trace;

    // spread nanobots
    ASSERT(system.bots.size() == 1);
    const int R = system.matrix.R;
    const int N = std::min<int>(system.bots.size() + system.bots[0].seeds.size(), R);

    std::vector<int> boundaries = getboundaries(tgt_matrix, N, 0, R);

    std::vector<int64_t> cellnum;
    int64_t sihyou = 0;
    for(int x=0; x<R; ++x){
      const int64_t cnum = getcellnumber(tgt_matrix, x, 0, R);
      cellnum.push_back(cnum);
      sihyou += cnum;
    }

    // dist
    sihyou = (sihyou / N) + 1;
    int64_t maxcnt = 1145141919;
    int imax = 256;
    for(int i = 1; i<= imax; ++i){
      Boundary result = sepbound(cellnum, sihyou * i /imax, N);
      if(maxcnt > result.first){
	boundaries = result.second;
	maxcnt = result.first;
      }
      /*
      cout<<i<<","<<result.second.size()<<","<<result.first<<",";
      for(auto b : result.second){
	cout<<b<<",";
      }
      cout<<endl;
      */
    }

    /*
    std::vector<int> boundaries;

    //old version
    boundaries.push_back(0);
    for (int i = 1; i <= N; ++i) {
      boundaries.push_back((R - 1) * (i - 1) / (N - 1) + 1);
    }
    */


    ASSERT(system.bots[0].pos == Vec3(0, 0, 0));
    std::vector<Vec3> positions(N, system.bots[0].pos);
    int num_active_nanobots = 1;
    while (positions[N - 1].x < boundaries[N - 1]) {
        const int index = num_active_nanobots - 1;
        for (int i = 0; i < index; ++i) {
            trace.push_back(CommandWait{});
        }
        if (positions[index].x == boundaries[index]) {
            trace.push_back(CommandFission{unitX, N - ++num_active_nanobots});
            positions[index + 1] = positions[index] + unitX;
        } else {
            trace.push_back(CommandSMove{unitX});
            positions[index] += unitX;
        }
    }
    auto shadow_positions = positions;

    // run each nanobot
    int highest = 0;
    std::vector<Trace> traces;
    traces.push_back({});
    for (int i = 1; i < N; ++i) {
        traces.push_back(single_stupid_solver(system, tgt_matrix,
                                              Vec3(boundaries[i], 0, 0),
                                              Vec3(boundaries[i + 1], R, R),
                                              positions[i]));
        highest = std::max(highest, positions[i].y);
    }

    std::size_t max_trace_size = 0;
    for (int i = 1; i < N; ++i) {
        naive_move(Vec3(positions[i].x, highest, positions[i].z),
             positions[i], traces[i]);
        max_trace_size = std::max(max_trace_size, traces[i].size());
    }


    // add header waiting
    // it it not effective
    /*
    for (int i = 1; i < N; ++i) {
      Trace htrace;
      for(int j = 0; j < max_trace_size - traces[i].size(); ++j){
	htrace.push_back(CommandWait{});
      }
      for(auto tr : traces[i]){
	htrace.push_back(tr);
      }
      traces[i] = htrace;
    }
    */

    // merge
    auto to_index = [R](const Vec3& p) {
        return p.x + p.y * R + p.z * R * R;
    };
    bool high_harmonics = false;
    int num_full = 0;
    const int ground = R * R * R;
    std::vector<bool> voxels(ground);
    UnionFind union_find(ground + 1);
    for (std::size_t t = 0; t < max_trace_size; ++t) {
        const auto flipper = trace.size();
        for (int i = 0; i < N; ++i) {
            if (t < traces[i].size()) {
                trace.push_back(traces[i][t]);
                if (auto* cmd = boost::get<CommandSMove>(&trace.back())) {
                    shadow_positions[i] += cmd->lld;
                } else if (auto* cmd = boost::get<CommandFill>(&trace.back())) {
                    const auto p = shadow_positions[i] + cmd->nd;
                    const auto index = to_index(p);
                    ++num_full;
                    voxels[index] = true;
                    if (p.y == 0) {
                        union_find.unionSet(ground, index);
                    } else {
                        for (const auto d : neighbors6) {
                            const auto neighbor = to_index(p + d);
                            if (voxels[neighbor]) {
                                union_find.unionSet(index, to_index(p + d));
                            }
                        }
                    }
                }
            } else {
                trace.push_back(CommandWait{});
            }
        }

        const bool grounded = union_find.size(ground) == num_full + 1;
        if (high_harmonics == grounded) {
            trace[flipper] = CommandFlip{};
            high_harmonics = !grounded;
        }
    }
    ASSERT(!high_harmonics);
    ASSERT(union_find.size(ground) == num_full + 1);

    // go home
    for (int i = N - 1; i > 0; --i) {
        Trace trace_to_join;
        naive_move(positions[i - 1], positions[i], trace_to_join);
        const auto* cmd = boost::get<CommandSMove>(&trace_to_join.back());
        ASSERT(cmd);
        positions[i] -= cmd->lld;
        trace_to_join.pop_back();
        for (std::size_t t = 0; t < trace_to_join.size(); ++t) {
            for (int j = 0; j < i; ++j) {
                trace.push_back(CommandWait{});
            }
            trace.push_back(trace_to_join[t]);
        }
        for (int j = 0; j < i - 1; ++j) {
            trace.push_back(CommandWait{});
        }
        trace.push_back(CommandFusionP{positions[i] - positions[i - 1]});
        trace.push_back(CommandFusionS{positions[i - 1] - positions[i]});
    }
    naive_move(Vec3(0, 0, 0), positions[0], trace);

    // finalize at the origin pos.
    trace.push_back(CommandHalt{});
    return trace;
}

}  // namespace

REGISTER_ENGINE(parallel_v3, solver);
// vim: set si et sw=4 ts=4:
