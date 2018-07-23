* Build/run the solution
Please execute the following command.

$ export NUMBER_OF_PROCESSORS=(the number of the logical CPU cores)
$ chmod +x ./*.sh
$ ./pack.sh

There are several dependencies.  Please install them as needed.
ex) Python3, gcc, boost, google tes, etc...

* Description of the solution approach
1. Divide the matter subspace pad into several sub-spaces in x-direction.
2. Fission the nanobot and place into each subspace.
3. Move and fill from bottom to top with each nanobot.
4. Fussion the nanobots.
5. Goto the starting position.

* Feedback about the contest
- The contest rule is very good.
- The official executor is very good. Especially, the visualizer is great.
- The submission system sometimes did not work. It was bad.
