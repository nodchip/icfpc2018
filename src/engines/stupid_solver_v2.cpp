#include "engine.h"
#include "stupid_solver_v2.h"
#include <vector>
#include <iostream>
using namespace std;

Trace stupid_solver_v2(const System& system, const Matrix& problem_matrix) {
    // use a single nanobot.
    // always in the high harmonics.
    // zig-zag scanning in the XZ plane.  
    // only fill the "previous" voxels.
    // if xz plane is finished go to next step

    Trace trace;
    trace.push_back(CommandFlip{}); // high.
    vector<int> filled(system.matrix.R+1);
    vector<int> blocknum(system.matrix.R+1);
    for(int y = 0; y < system.matrix.R ; ++y){
      for(int x = 0; x < system.matrix.R ; ++x){
	for(int z = 0; z < system.matrix.R ; ++z){
	  if(problem_matrix(x,y,z)){
	    ++blocknum[y];
	  }
	}
      }
    }

    Vec3 p(system.bots[0].pos);
    Vec3 prev = p;
    while (true) {
        int zdir = p.y % 2 == 0 ? +1 : -1;
	bool is_plane_finished = false;
	if(filled[p.y] == blocknum[p.y]){
	  is_plane_finished = true;
	}

        while (0 <= p.z + zdir && p.z + zdir < system.matrix.R) {
            int xdir = p.z % 2 == 0 ? +1 : -1;
            while (0 <= p.x + xdir && p.x + xdir < system.matrix.R) {
                p.x += xdir;
                trace.push_back(CommandSMove{Vec3(xdir, 0, 0)});
                if (problem_matrix(prev)) {
                    trace.push_back(CommandFill{prev - p});
		    filled[p.y] +=1;
		    // finished
		    if(filled[p.y] == blocknum[p.y]){
		      is_plane_finished = true;
		    }
                }
                prev = p;
		if(is_plane_finished){
		  break;
		}
            }
	    if(is_plane_finished){
	      break;
	    }
            // turn around.
            p.z += zdir;
            trace.push_back(CommandSMove{Vec3(0, 0, zdir)});
            if (problem_matrix(prev)) {
                trace.push_back(CommandFill{prev - p});
		filled[p.y] += 1;
		// finished
		if(filled[p.y] == blocknum[p.y]){
		  is_plane_finished = true;
		}
            }
            prev = p;
	    if(is_plane_finished){
	      break;
	    }
        }
        if (p.y + 1 < system.matrix.R ) {
            // up.
            p.y += 1;
            trace.push_back(CommandSMove{Vec3(0, 1, 0)});
	    
	    if(is_plane_finished){
	      // go to edge point (not so good...)
	      if(p.y % 2 == 0){
		trace.push_back(CommandSMove{Vec3(-p.x, 0, -p.z)});
		p.x = 0; p.z = 0;
	      }else{
		trace.push_back(CommandSMove{Vec3( (system.matrix.R-1) % 2 == 0 ? -p.x : system.matrix.R - p.x -1, 0, system.matrix.R - p.z -1)});
		p.z = system.matrix.R - 1;
		p.x = (system.matrix.R-1) % 2 == 0 ? 0 : system.matrix.R -1;
	      }
	    }
        } else {
            break;
        }
    }

    // go home.
    std::vector<Vec3> trajectory;
    if (!bfs_shortest_in_void(system.matrix, p, system.final_pos(),
        &trace, &trajectory)) {
        std::cout << "sorry, stupid algorithm failed.." << std::endl;
        return trace;
    }

    if (false) {
        for (auto p : trajectory) {
            p.print();
        }
    }

    // finalize at the origin pos.
    trace.push_back(CommandFlip{});
    trace.push_back(CommandHalt{});
    return trace;
}

REGISTER_ENGINE(stupid_v2, stupid_solver_v2);
// vim: set si et sw=4 ts=4:
