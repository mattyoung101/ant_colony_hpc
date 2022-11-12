callgrind.out.2233942 is profiled against the code with all the ant update code inside World::update()

callgrind.out.1076540 is profiled against the code where I split World::update further out into World::updateAnt()
(this was chiefly done for MPI)

callgrind.out.966881 is profiled against the 32-thread OpenMP version on my PC, with World::updateAnt()
existing