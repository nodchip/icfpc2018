#include "parallel_stupid_solver.h"

#include "engine.h"
#include "nmms.h"
#include "system.h"
#include <utility>
#include <algorithm>

namespace {

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

  void naive_move(const Vec3& destination, Vec3& position, Trace& trace, long long int &t) {
    const Vec3 dx = (destination.x > position.x) ? unitX : -unitX;
    while (position.x != destination.x) {
        trace.push_back(CommandSMove{dx});
	++t;
        position += dx;
    }
    const Vec3 dy = (destination.y > position.y) ? unitY : -unitY;
    while (position.y != destination.y) {
        trace.push_back(CommandSMove{dy});
	++t;
        position += dy;
    }
    const Vec3 dz = (destination.z > position.z) ? unitZ : -unitZ;
    while (position.z != destination.z) {
        trace.push_back(CommandSMove{dz});
	++t;
        position += dz;
    }
}

Trace single_stupid_solver(const System& system, const Matrix& problem_matrix,
                           const Vec3& lower_bound, const Vec3& upper_bound,
                           Vec3& position, std::vector< std::pair<long long int, Vec3> > &time_and_loc) {
    // calculate bounding box
  long long int t = 0;
    Vec3 lower = upper_bound - Vec3(1, 1, 1);
    Vec3 upper = lower_bound;
    for (int x = lower_bound.x; x < upper_bound.x; ++x) {
        for (int y = lower_bound.y; y < upper_bound.y; ++y) {
            for (int z = lower_bound.z; z < upper_bound.z; ++z) {
                if (problem_matrix(Vec3(x, y, z))) {
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
    naive_move(lower, position, trace, t);
    int dx = -1;
    int dz = -1;
    for (int y = lower.y + 1; y <= upper.y; ++y) {
        dz *= -1;
        trace.push_back(CommandSMove{unitY});
	++t;
        position += unitY;
        for (int z = lower.z; z < upper.z; ++z) {
            dx *= -1;
            if (z > lower.z) {
                trace.push_back(CommandSMove{Vec3(0, 0, dz)});
		++t;
                position += Vec3(0, 0, dz);
            }
            for (int x = lower.x; x < upper.x; ++x) {
                if (x > lower.x) {
                    trace.push_back(CommandSMove{Vec3(dx, 0, 0)});
		    ++t;
                    position += Vec3(dx, 0, 0);
                }
                if (problem_matrix(position - unitY)) {
                    trace.push_back(CommandFill{-unitY});
		    time_and_loc.push_back(std::pair<long long int, Vec3>(t, position-unitY));
		    ++t;
                }
            }
        }
    }

    return trace;
}

}  // namespace


void unfset(const int R, UnionFind &unf, vector<bool> &is_filled, int x, int y, int z){

  if(y == 0){
    unf.unionSet(0, x * R * R + z + 1);
  }

  if(x > 0) {
    if(is_filled[( (x - 1) * R + y ) * R + z]){
      unf.unionSet(( (x-1) * R + y) * R + z + 1 , (x * R + y) * R + z + 1);
    }
  }
  if(x < R - 1) {
    if(is_filled[( (x + 1) * R + y ) * R + z]){
      unf.unionSet(( (x+1) * R + y) * R + z + 1 , (x * R + y) * R + z + 1);
    }
  }


  if(y > 0) {
    if(is_filled[( x * R + y - 1 ) * R + z]){
      unf.unionSet(( x * R + ( y-1 )) * R + z + 1 , (x * R + y) * R + z + 1);
    }
  }

  if(y < R - 1) {
    if(is_filled[( x * R + y + 1 ) * R + z]){
      unf.unionSet(( x * R + ( y+1 )) * R + z + 1 , (x * R + y) * R + z + 1);
    }
  }


  if(z > 0) {
    if(is_filled[( x * R + y ) * R + z - 1]){
      unf.unionSet(( x * R + y) * R + z , (x * R + y) * R + z + 1);
    }
  }

  if(z < R - 1) {
    if(is_filled[( x * R + y ) * R + z + 1]){
      unf.unionSet(( x * R + y) * R + z + 2, (x * R + y) * R + z + 1);
    }
  }
  is_filled[(x * R + y) * R + z] = true;
}

vector<long long int> get_time_to_flip(const int R, UnionFind &unf, std::vector< std::pair<long long int, Vec3> > &time_and_loc){
  std::vector<bool> is_filled(R * R * R);

  std::vector<long long int> out;
  bool is_harmonic = false;
  for(long long int i = 0; i< time_and_loc.size()-1; ++i ){
    // unite
    unfset(R, unf, is_filled, time_and_loc[i].second.x, time_and_loc[i].second.y, time_and_loc[i].second.z);

    // check
    // cout<<time_and_loc[i].first<<","<<time_and_loc[i].second.x<<","<<time_and_loc[i].second.y<<","<<time_and_loc[i].second.z<<endl;
    if(time_and_loc[i].first != time_and_loc[i+1].first){
      if(unf.size(0)-1 == i + 1){
	// no harmonic
	if(is_harmonic){
	  out.push_back(time_and_loc[i].first + 1);
	  is_harmonic = false;
	}
      }else{
	// harmonic
	if(!is_harmonic){
	  out.push_back(time_and_loc[i].first);
	  is_harmonic = true;
	}
      }
    }
  }
  return out;
}

Trace parallel_stupid_solver(const Matrix& problem_matrix) {
    System system(problem_matrix.R);
    Trace trace;
    //trace.push_back(CommandFlip{}); // high.

    // union find
    UnionFind unf(system.matrix.R*system.matrix.R*system.matrix.R + 1);

    // time and location
    std::vector< std::pair<long long int, Vec3> > time_and_loc;

    // spread nanobots
    // ASSERT(system.bots.size() == 1);
    const int R = system.matrix.R;
    const int N = system.bots.size() + system.bots[0].seeds.size();
    std::vector<int> boundaries;
    for (int i = 0; i <= N; ++i) {
        boundaries.push_back(R * i / N);
    }
    // ASSERT(system.bots[0].pos == Vec3(0, 0, 0));
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
            int diff = min<int>(15, boundaries[index] - positions[index].x);
            trace.push_back(CommandSMove{Vec3 {diff, 0, 0}});
            positions[index].x += diff;
        }
    }

    // run each nanobot
    int highest = 0;
    std::vector<Trace> traces;
    for (int i = 0; i < N; ++i) {
        traces.push_back(single_stupid_solver(system, problem_matrix,
                                              Vec3(boundaries[i], 0, 0),
                                              Vec3(boundaries[i + 1], R, R),
                                              positions[i], time_and_loc));
        highest = std::max(highest, positions[i].y);
    }
    std::sort(time_and_loc.begin(), time_and_loc.end());
    std::vector<long long int> flips;
    flips= get_time_to_flip(system.matrix.R, unf, time_and_loc);
    if(flips.size()==0){
      flips.push_back(-1);
    }
    for(auto f : flips){
      cout<<f<<",";
    }
    cout<<endl;
    std::size_t max_trace_size = 0;
    long long int dump = 0;
    for (int i = 0; i < N; ++i) {
      naive_move(Vec3(positions[i].x, highest, positions[i].z),
		 positions[i], traces[i], dump);
        max_trace_size = std::max(max_trace_size, traces[i].size());
    }

    // merge
    bool flipped = false;
    int flipcnt = 0;
    for (std::size_t t = 0; t < max_trace_size; ++t) {
      if(t == flips[flipcnt]){
	trace.push_back(CommandFlip{});
	flipped = !flipped;
	for (int i = 1; i < N; ++i) {
	  trace.push_back(CommandWait{});
	}
	++flipcnt;
      }
        for (int i = 0; i < N; ++i) {
            if (t < traces[i].size()) {
                trace.push_back(traces[i][t]);
            } else {
                trace.push_back(CommandWait{});
            }
        }
    }

    // go home
    for (int i = N - 1; i > 0; --i) {
        Trace trace_to_join;
        naive_move(positions[i - 1], positions[i], trace_to_join,dump);
        if (auto* command = boost::get<CommandSMove>(&trace_to_join.back())) {
            positions[i] -= command->lld;
            trace_to_join.pop_back();
        } else {
            std::cerr << "unexpected error" << std::endl;
        }
        for (std::size_t t = 0; t < trace_to_join.size(); ++t) {
            for (int j = 0; j < i; ++j) {
                trace.push_back(CommandWait{});
            }
            trace.push_back(trace_to_join[t]);
        }
        for (int j = 0; j < i - 1; ++j) {

	    if(flipped){
	      // flip before merge
	      trace.push_back(CommandFlip{});
	      flipped = false;

	    }else{
	      trace.push_back(CommandWait{});
	    }
        }
        trace.push_back(CommandFusionP{positions[i] - positions[i - 1]});
        trace.push_back(CommandFusionS{positions[i - 1] - positions[i]});
    }
    naive_move(Vec3(0, 0, 0), positions[0], trace, dump);

    if (flipped){
      // flip before merge
      trace.push_back(CommandFlip{});
      flipped = false;
    }
    // finalize at the origin pos.
    trace.push_back(CommandHalt{});
    return trace;
}

REGISTER_ENGINE(parallel_stupid, parallel_stupid_solver);
// vim: set si et sw=4 ts=4:
