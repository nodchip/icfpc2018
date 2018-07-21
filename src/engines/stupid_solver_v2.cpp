#include "stupid_solver_v2.h"

#include <vector>
#include <iostream>

#include "bounding_box.h"
#include "engine.h"
#include "matrix.h"
#include "nmms.h"
#include "region.h"
#include "system.h"
#include "union_find.h"
using namespace std;

void push_back_safe_long_move(int x, int y, int z, Trace &trace){
  int spx = abs(x / 15);
  for(int i=0;i<spx;++i){
    trace.push_back(CommandSMove{Vec3(x<0 ? -15 : 15, 0, 0)});
    x += ( x < 0 ? 15 : -15);
  }
  if(x!=0){
    trace.push_back(CommandSMove{Vec3(x, 0, 0)});
  }

  int spy = abs(y / 15);
  for(int i=0;i<spy;++i){
    trace.push_back(CommandSMove{Vec3(0, y<0 ? -15 : 15, 0)});
    y += ( y < 0 ? 15 : -15);
  }
  if(y !=0 ){
    trace.push_back(CommandSMove{Vec3(0, y, 0)});
  }

  int spz = abs(z / 15);
  for(int i=0;i<spz;++i){
    trace.push_back(CommandSMove{Vec3(0, 0, z<0 ? -15 : 15)});
    z += ( z < 0 ? 15 : -15);
  }
  if(z !=0 ){
    trace.push_back(CommandSMove{Vec3(0, 0, z)});
  }
}

bool is_high_harmonic_needed(const System& system, const Matrix& problem_matrix, const vector<bool> &is_filled, const int x, const int y, const int z){
  if(y == 0){
    return false;
  }
  if(x > 0) {
    if(is_filled[( (x - 1) * system.matrix.R + y ) * system.matrix.R + z]){
      return false;
    }
  }
  if(x < system.matrix.R - 1) {
    if(is_filled[( (x + 1) * system.matrix.R + y ) * system.matrix.R + z]){
      return false;
    }
  }

  if(y > 0) {
    if(is_filled[( x * system.matrix.R + y - 1 ) * system.matrix.R + z]){
      return false;
    }
  }
  if(y < system.matrix.R - 1) {
    if(is_filled[( x * system.matrix.R + y + 1 ) * system.matrix.R + z]){
      return false;
    }
  }

  if(z > 0) {
    if(is_filled[( x * system.matrix.R + y ) * system.matrix.R + z - 1]){
      return false;
    }
  }

  if(z < system.matrix.R - 1) {
    if(is_filled[( x * system.matrix.R + y ) * system.matrix.R + z + 1]){
      return false;
    }
  }

  return true;
}

void unfset(const System& system, UnionFind &unf, const vector<bool> &is_filled, int x, int y, int z){
  if(y == 0){
    unf.unionSet(0, x * system.matrix.R * system.matrix.R + z + 1);
    return;
  }

  if(x > 0) {
    if(is_filled[( (x - 1) * system.matrix.R + y ) * system.matrix.R + z]){
      unf.unionSet(( (x-1) * system.matrix.R + y) * system.matrix.R + z + 1 , (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }
  if(x < system.matrix.R - 1) {
    if(is_filled[( (x + 1) * system.matrix.R + y ) * system.matrix.R + z]){
      unf.unionSet(( (x+1) * system.matrix.R + y) * system.matrix.R + z + 1 , (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }

  if(y > 0) {
    if(is_filled[( x * system.matrix.R + y - 1 ) * system.matrix.R + z]){
      unf.unionSet(( x * system.matrix.R + ( y-1 )) * system.matrix.R + z + 1 , (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }
  if(y < system.matrix.R - 1) {
    if(is_filled[( x * system.matrix.R + y + 1 ) * system.matrix.R + z]){
      unf.unionSet(( x * system.matrix.R + ( y+1 )) * system.matrix.R + z + 1 , (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }

  if(z > 0) {
    if(is_filled[( x * system.matrix.R + y ) * system.matrix.R + z - 1]){
      unf.unionSet(( x * system.matrix.R + y) * system.matrix.R + z , (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }

  if(z < system.matrix.R - 1) {
    if(is_filled[( x * system.matrix.R + y ) * system.matrix.R + z + 1]){
      unf.unionSet(( x * system.matrix.R + y) * system.matrix.R + z + 2, (x * system.matrix.R + y) * system.matrix.R + z + 1);
    }
  }
}

Trace stupid_solver_v2(const Matrix& problem_matrix) {
    System system(problem_matrix.R);
    // use a single nanobot.
    // always in the high harmonics.
    // zig-zag scanning in the XZ plane.
    // only fill the "previous" voxels.
    // if xz plane is finished go to next step
    // judge if harmonic is needed or not and switch it automatically

    Trace trace;
    UnionFind unf(system.matrix.R*system.matrix.R*system.matrix.R + 1);

    bool is_high = false;
    bool all_done = false;
    long long int total_filled = 0;
    long long int total_size = 0;
    vector<int> filled(system.matrix.R+1);
    vector<int> blocknum(system.matrix.R+1);
    vector<bool> is_filled(system.matrix.R * system.matrix.R * system.matrix.R);
    for(int y = 0; y < system.matrix.R ; ++y){
      for(int x = 0; x < system.matrix.R ; ++x){
	for(int z = 0; z < system.matrix.R ; ++z){
	  if(problem_matrix(x,y,z)){
	    ++blocknum[y];
	  }
	}
      }
      total_size += blocknum[y];
    }

    Vec3 p(system.bots[0].pos);

    // move to bbox.
    Region bbox = find_bounding_box(problem_matrix, nullptr).canonical();

    bbox.c1.x = 0;
    bbox.c1.y = 0;
    bbox.c1.z = 0;

    bbox.c2.x = system.matrix.R-1;
    bbox.c2.y = system.matrix.R-1;
    bbox.c2.z = system.matrix.R-1;

    bfs_shortest_in_void(problem_matrix, p, bbox.c1, &trace, nullptr);
    p = bbox.c1;

    Vec3 prev = p;

    while (true) {
        int zdir = p.y % 2 == 0 ? +1 : -1;
	bool is_plane_finished = false;
	if(filled[p.y] == blocknum[p.y]){
	  is_plane_finished = true;
	}
        while (bbox.c1.z <= p.z + zdir && p.z + zdir <= bbox.c2.z) {
            int xdir = p.z % 2 == 0 ? +1 : -1;
	    while (bbox.c1.x <= p.x + xdir && p.x + xdir <= bbox.c2.x) {
                p.x += xdir;
                trace.push_back(CommandSMove{Vec3(xdir, 0, 0)});
                if (problem_matrix(prev)) {
		  if(!is_high && is_high_harmonic_needed(system, problem_matrix, is_filled, prev.x, prev.y, prev.z)){
		    // for debug
		    cout<<"high harmonic "<<prev.x<<","<<prev.y<<","<<prev.z<<endl;
		    is_high = true;
		    trace.push_back(CommandFlip{}); // high.
		  }
                    trace.push_back(CommandFill{prev - p});
		    is_filled[(prev.x * system.matrix.R + prev.y ) * system.matrix.R + prev.z] = true;
		    filled[p.y] +=1;
		    total_filled += 1;
		    if(total_filled == total_size){
		      all_done = true;
		    }

		    unfset(system, unf, is_filled, prev.x, prev.y, prev.z);
		    if(unf.size(0)-1 == total_filled && is_high){
		      is_high = false;
		      trace.push_back(CommandFlip{}); // low.
		    }

		    // finished
		    if(filled[p.y] == blocknum[p.y]){
		      is_plane_finished = true;
		      cout << "finished " << p.x << "," << p.y << "," << p.z << endl;
		    }
                }
                prev = p;
		if(is_plane_finished || all_done){
		  break;
		}
            }
	    if(is_plane_finished || all_done){
	      break;
	    }
            // turn around.
            p.z += zdir;
            trace.push_back(CommandSMove{Vec3(0, 0, zdir)});
            if (problem_matrix(prev)) {
	      if(!is_high && is_high_harmonic_needed(system, problem_matrix, is_filled, prev.x, prev.y, prev.z)){
		is_high = true;
		trace.push_back(CommandFlip{}); // high.
	      }
                trace.push_back(CommandFill{prev - p});
		is_filled[(prev.x * system.matrix.R + prev.y ) * system.matrix.R + prev.z] = true;
		filled[p.y] += 1;
		total_filled += 1;
		if(total_filled == total_size){
		  all_done = true;
		}
		unfset(system, unf, is_filled, prev.x, prev.y, prev.z);
		if(unf.size(0)-1 == total_filled && is_high){
		  is_high = false;
		  trace.push_back(CommandFlip{}); // low.
		}

		// finished
		if(filled[p.y] == blocknum[p.y]){
		  is_plane_finished = true;
		  cout<<"finished "<<p.x<<","<<p.y<<","<<p.z<<endl;
		}
            }
            prev = p;
	    if(is_plane_finished || all_done){
	      break;
	    }
        }
	if(all_done){
	  break;
	}
        if (p.y + 1 < system.matrix.R ) {
            // up.
            p.y += 1;
            trace.push_back(CommandSMove{Vec3(0, 1, 0)});

	    if(is_plane_finished){
	      // go to edge point (not so good...)
	      if(p.y % 2 == 0){
		push_back_safe_long_move(-p.x + bbox.c1.x, 0, -p.z + bbox.c1.z, trace);
		p.x = bbox.c1.x; p.z = bbox.c1.z;
	      }else{
		push_back_safe_long_move(bbox.c2.z % 2 == 0 ? -p.x + bbox.c1.x : + bbox.c2.x- p.x , 0, bbox.c2.z - p.z, trace);
		p.z = bbox.c2.z;
		p.x = bbox.c2.z % 2 == 0 ? bbox.c1.x : bbox.c2.x;
	      }
	    }
        } else {
            break;
        }
    }

    // finalize at the origin pos.
    if(is_high){
      trace.push_back(CommandFlip{});
    }

    cout<<"gohome"<<endl;
    // go home.
    if (!bfs_shortest_in_void(problem_matrix, p, system.final_pos(),
			      &trace, nullptr)) {
      std::cout << "sorry, stupid algorithm failed.." << std::endl;
      return trace;
    }

    trace.push_back(CommandHalt{});
    return trace;
}

REGISTER_ENGINE(stupid_v2, stupid_solver_v2);
// vim: set si et sw=4 ts=4:
