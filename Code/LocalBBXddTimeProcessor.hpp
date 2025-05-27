/*
 *	LocalBBXddTimeProcessor interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2022, IRIT UPS.
 *
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef OTAWA_XDD_PIPXANALYSIS_LOCALBBXDDTIMEPROCESSOR_HPP
#define OTAWA_XDD_PIPXANALYSIS_LOCALBBXDDTIMEPROCESSOR_HPP

#include "../frontend/XStepsGen.hpp"
#include "XStepsMatrixCompiler.hpp"
#include "../MatrixStats.h"

#ifdef XDD_PARA
#	include <elm/sys/JobScheduler.h>
#endif

namespace otawa{ namespace xengine{

class XddTimingState;

class LocalBBXddTimeProcessor: public BBProcessor {
public:
	static p::declare reg;
	LocalBBXddTimeProcessor();

protected:
	void setup(WorkSpace* ws) override;
#	ifdef XDD_PARA
		void processAll(WorkSpace *ws) override;
#	endif
	void processBB(WorkSpace* ws, CFG* cfg, Block* bb) override;
	void destroy(WorkSpace* ws) override;
	void computeTime(Edge *e, const Vector<const AbstractXStep *>& xsteps, XddTimingState& state);
	void processEdge(Edge *e);
	void dumpGlobalStats(io::Output &out) override;
private:

#	ifdef XDD_PARA
		class Job: public sys::Job {
		public:
			Job(LocalBBXddTimeProcessor& proc, Edge *edge): _proc(proc), _edge(edge) {}
			void run() override;
		private:
			LocalBBXddTimeProcessor& _proc;
			Edge *_edge;
		};

		class Producer: public sys::JobProducer {
		public:
			Producer(LocalBBXddTimeProcessor& proc, const CFGCollection *coll);
			Job *next() override;
		private:
			void nextBB();
			LocalBBXddTimeProcessor& _proc;
			CFGCollection::BlockIter _block;
			Block::EdgeIter _ins;
		};
#	endif

	XEngine* _xengine;
	XStepsMatrixCompiler* _compiler;
	StandardXddManager *_xman;
	UnitXResourcesManager *_rman;
	MatrixStats *_stats;
};

} }

#endif // OTAWA_XDD_PIPXANALYSIS_LOCALBBXDDTIMEPROCESSOR_HPP
