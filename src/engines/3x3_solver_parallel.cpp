#include "3x3_solver.h"
#include <boost/range/algorithm_ext/push_back.hpp>

#include "engine.h"
#include "log.h"
#include "nmms.h"
#include "state.h"
#include "system.h"

static const Vec3 kPlaneNeighbor8[] = {
    Vec3(-1, -1, 0),
    Vec3(-1, 0, 0),
    Vec3(-1, 1, 0),
    Vec3(0, -1, 0),
    Vec3(0, 1, 0),
    Vec3(1, -1, 0),
    Vec3(1, 0, 0),
    Vec3(1, 1, 0),
};

void DoFill(const Vec3& current_position, const Vec3& full_direction, Matrix& current_matrix, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + full_direction));
    ASSERT(current_matrix(current_position + full_direction) == Void);
    current_matrix(current_position + full_direction) = Full;
    trace.push_back(CommandFill{ full_direction });
}

void DoVoid(const Vec3& current_position, const Vec3& void_direction, Matrix& current_matrix, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + void_direction));
    ASSERT(current_matrix(current_position + void_direction) == Full);
    current_matrix(current_position + void_direction) = Void;
    trace.push_back(CommandVoid{ void_direction });
}

void DoSMove(const Matrix& current_matrix, const Vec3& move_direction, Vec3& current_position, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + move_direction));
    current_position += move_direction;
    trace.push_back(CommandSMove{ move_direction });
}

bool is_inside_bound(const Vec3 position, const Vec3 upper, const Vec3 lower){
  return ( position.x >= lower.x && position.x < upper.x && position.y >= lower.y && position.y < upper.y && position.z >= lower.z && position.z < upper.z);
}

void dig_to_x(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position, int x_direction, const int xstep) {

    Vec3 front_direction(x_direction, 0, 0);
    Vec3 back_direction(-x_direction, 0, 0);
    for (int i = 0; i < xstep; ++i) {
        // Void if the front voxel if full.
        if (current_matrix(current_position + front_direction) == Full) {
            DoVoid(current_position, front_direction, current_matrix, trace);
        }

        // Move to the front voxel.
        DoSMove(current_matrix, front_direction, current_position, trace);

        // Full if the back voxel as needed.
        ASSERT(current_matrix(current_position + back_direction) == Void);
        if (dst_matrix(current_position + back_direction) == Full) {
            DoFill(current_position, back_direction, current_matrix, trace);
        }
    }
}

void dig_to_y(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position, const int ystep) {

    Vec3 front_direction(0, 1, 0);
    Vec3 back_direction(0, -1, 0);
    for (int i = 0; i < ystep; ++i) {
      // Void if the front voxel if full.
        if (current_matrix(current_position + front_direction) == Full) {
            DoVoid(current_position, front_direction, current_matrix, trace);
        }

        // Move to the front voxel.
        DoSMove(current_matrix, front_direction, current_position, trace);

        // Full if the back voxel as needed.
        ASSERT(current_matrix(current_position + back_direction) == Void);
        if (dst_matrix(current_position + back_direction) == Full) {
            DoFill(current_position, back_direction, current_matrix, trace);
        }
    }
}


void dig_to_z(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position, int z_direction, const Vec3 upper, const Vec3 lower) {
  Vec3 front_direction(0, 0, z_direction);
  Vec3 back_direction(0, 0, -z_direction);
  for (;;) {
    // Full or void x-y 8-neighbor voxels.
    for (const auto& direction : kPlaneNeighbor8) {
      if (!dst_matrix.is_in_matrix(current_position + direction)) {
	continue;
      }
      
      if (current_matrix(current_position + direction) == Full && dst_matrix(current_position + direction) == Void && is_inside_bound(current_position + direction, upper, lower)) {
	DoVoid(current_position, direction, current_matrix, trace);
      }
      else if (current_matrix(current_position + direction) == Void && dst_matrix(current_position + direction) == Full && is_inside_bound(current_position + direction, upper, lower)) {
	DoFill(current_position, direction, current_matrix, trace);
      }
    }

    if(is_inside_bound(current_position + back_direction, upper, lower)){
      // Full the back voxel as needed.
      ASSERT(current_matrix(current_position + back_direction) == Void);
      if (dst_matrix(current_position + back_direction) == Full) {
      DoFill(current_position, back_direction, current_matrix, trace);
      }
    }
    
    if (current_position.z + z_direction < lower.z || upper.z - 1 < current_position.z + z_direction) {
      break;
    }
    
    // Void the front voxel as needed.
    if (current_matrix(current_position + front_direction) == Full) {
      DoVoid(current_position, front_direction, current_matrix, trace);
    }
    
    // Move to the front voxel.
    ASSERT(current_matrix(current_position + front_direction) == Void);
    DoSMove(current_matrix, front_direction, current_position, trace);
  }
}

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

void naive_move(Matrix &current_matrix, const Vec3& destination, Vec3& position, Trace& trace) {
  //cout<<position.x<<","<<position.y<<","<<position.z<<","<<destination.x<<","<<destination.y<<","<<destination.z<<endl;
  const Vec3 dz = (destination.z > position.z) ? unitZ : -unitZ;
  while (position.z != destination.z) {
    if(current_matrix(position+dz) == Full){
      DoVoid(position, dz, current_matrix, trace);
    }
    DoSMove(current_matrix, dz, position, trace); 
  }

  const Vec3 dx = (destination.x > position.x) ? unitX : -unitX;
  while (position.x != destination.x) {
    if(current_matrix(position+dx) == Full){
      DoVoid(position, dx, current_matrix, trace);
    }
    DoSMove(current_matrix, dx, position, trace);
  }

  const Vec3 dy = (destination.y > position.y) ? unitY : -unitY;
  while (position.y != destination.y) {
    if(current_matrix(position+dy) == Full){
      DoVoid(position, dy, current_matrix, trace);
    }
    DoSMove(current_matrix, dy, position, trace); 
  }
}


Trace single_three_by_three_solver(Matrix& current_matrix, const Matrix& src_matrix, const Matrix& dst_matrix, const Vec3& lower_bound, const Vec3& upper_bound, Vec3& position){
  Trace trace;
  // calculate bounding box
  
  Vec3 lower = upper_bound - Vec3(1, 1, 1);
  Vec3 upper = lower_bound;
  
  //cout<<"bef"<<upper.x<<","<<upper.y<<","<<upper.z<<","<<lower.x<<","<<lower.y<<","<<lower.z<<endl;
  for (int x = lower_bound.x; x < upper_bound.x; ++x) {
    for (int y = lower_bound.y; y < upper_bound.y; ++y) {
      for (int z = lower_bound.z; z < upper_bound.z; ++z) {
	if (src_matrix(Vec3(x, y, z)) || dst_matrix(Vec3(x, y, z))) {
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

  lower.x = lower_bound.x;
  upper.x = upper_bound.x;

  lower.z = lower_bound.z;
  upper.z = upper_bound.z;

  
  if (lower.x >= upper.x) {
    return {};
  }
  
  lower.x = min(lower.x, position.x);  
  //cout<<"aft"<<upper.x<<","<<upper.y<<","<<upper.z<<","<<lower.x<<","<<lower.y<<","<<lower.z<<endl;
  // Move to the start position.
  naive_move(current_matrix, lower, position, trace); 

  // ZigZag
  int x_direction = 1;
  int z_direction = 1;
  //cout<<"dig start"<<endl;
  for (;;) {
    //cout<<"digz : "<<position.x<<","<<position.y<<","<<position.z<<","<<z_direction<<endl;
    dig_to_z(dst_matrix, current_matrix, trace, position, z_direction, upper, lower);
    z_direction = -z_direction;
    const int xstep = std::min(3, std::max(x_direction * (lower.x - position.x), x_direction * (upper.x - 1 - position.x)));
    if(xstep > 0){
      //cout<<"digx : "<<position.x<<","<<position.y<<","<<position.z<<","<<x_direction<<endl;
      dig_to_x(dst_matrix, current_matrix, trace, position, x_direction, xstep);
    }else {
      const int ystep = std::min(3,  upper.y - position.y);
      if (ystep > 0) {
	x_direction = -x_direction;
	//cout<<"digy : "<<position.x<<","<<position.y<<","<<position.z<<","<<z_direction<<endl;
	dig_to_y(dst_matrix, current_matrix, trace, position, ystep);
      }else {
	break;
      }
    }
  }
  
  //cout<<"digdone"<<endl;
  if (dst_matrix(position) == Full) {
    // Move +y by one voxel to fill the last voxel.
    //cout<<"fill final"<<endl;
    DoSMove(current_matrix, Vec3(0, 1, 0 ), position, trace);
    DoFill(position, Vec3(0, -1, 0 ), current_matrix, trace);
  }
  
  //cout<<"single done"<<endl;
  return trace;
}

Trace three_by_three_solver_parallel(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    printf("three_by_three_solver\n");
    const int R = src_matrix.R;
    System system(R);
    Trace trace;
    Matrix current_matrix = src_matrix;
    // spread nanobots
    ASSERT(system.bots.size() == 1);
    
    // not a good hach
    int Nsq=1;
    while( (Nsq+1) * (Nsq+1) * 2 <= R && (Nsq+1) * (Nsq+1)  < system.bots.size() + system.bots[0].seeds.size()){
      ++Nsq;
    }
    const int N = Nsq * Nsq;
    cout<<"dotnum = "<<N<<endl;
    
    std::vector<int> boundaries_init;
    
    for (int i = 0; i <= N; ++i) {
      boundaries_init.push_back(R * i / N);
    }
    
    std::vector<int> boundaries;
    for (int i = 0; i <= Nsq; ++i) {
      boundaries.push_back(R * i / Nsq);
      //cout<<boundaries[i]<<",";
    }
    //cout<<"boundaries"<<boundaries.size()<<endl;
    
    ASSERT(system.bots[0].pos == Vec3(0, 0, 0));

    std::vector<Vec3> positions(N, system.bots[0].pos);
    
    // goto highest to avoid collision
    for(int i=0;i<R-1;++i){
      trace.push_back(CommandSMove{unitY});
      positions[0] += unitY;
    }
    //cout<<"move"<<positions[0].y<<endl;
    
    // spread on x-axis
    int num_active_nanobots = 1;
    while (positions[N - 1].x < boundaries_init[N - 1]) {
      const int index = num_active_nanobots - 1;
      for (int i = 0; i < index; ++i) {
	trace.push_back(CommandWait{});
      }
      if (positions[index].x == boundaries_init[index]) {
	trace.push_back(CommandFission{unitX, N - ++num_active_nanobots});
	positions[index + 1] = positions[index] + unitX;
      } else {
	trace.push_back(CommandSMove{unitX});
	positions[index] += unitX;
      }
    }

    //cout<<"spread end"<<endl;

    // filp should be after spread
    trace.push_back(CommandFlip{}); 
    for (int i = 1; i < N; ++i) {
      trace.push_back(CommandWait{});
    }

    int highest = 0;
    std::vector<Trace> traces;
    for (int i = 0; i < N; ++i) {
      //cout<<i<<endl;
      traces.push_back(single_three_by_three_solver(current_matrix, src_matrix, dst_matrix,
						    Vec3(boundaries[i / Nsq], 0, boundaries[i % Nsq]),
						    Vec3(boundaries[(i / Nsq)+1], R, boundaries[(i % Nsq) + 1]),
						    positions[i]));
        highest = std::max(highest, positions[i].y);
    }

    //cout<<"trace get"<<endl;
    std::size_t max_trace_size = 0;
    for (int i = 0; i < N; ++i) {
      naive_move(current_matrix, Vec3(positions[i].x, highest, positions[i].z),
		 positions[i], traces[i]);
      max_trace_size = std::max(max_trace_size, traces[i].size());
    }

    //cout<<"move to top"<<endl;
    // merge
    for (std::size_t t = 0; t < max_trace_size; ++t) {
      for (int i = 0; i < N; ++i) {
	if (t < traces[i].size()) {
	  trace.push_back(traces[i][t]);
	} else {
	  trace.push_back(CommandWait{});
	}
      }
    }

    //cout<<"merged"<<endl;
    trace.push_back(CommandFlip{}); // low. 
    for (int i = 1; i < N; ++i) {
      trace.push_back(CommandWait{});
    }
    
    // go home
    for (int i = N - 1; i > 0; --i) {
      Trace trace_to_join;
      naive_move(current_matrix, positions[i - 1], positions[i], trace_to_join);
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
    naive_move(current_matrix, Vec3(0, 0, 0), positions[0], trace);

    //cout<<"backed"<<endl;
    // finalize at the origin pos.
    trace.push_back(CommandHalt{});
    
    return trace;
}

REGISTER_ENGINE(3x3_solver_parallel, three_by_three_solver_parallel);
// vim: set si et sw=4 ts=4:
