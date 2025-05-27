/*
 *  LocalBBXddTimeProcessor implementation
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

#include <otawa/xengine/pipeline_analyses/LocalBBXddTimeProcessor.hpp>
#include <otawa/xengine/features.h>
#include <otawa/xengine/pipeline_analyses/XddPipelineState.hpp>
#include <otawa/xdd/XddMatrix.hpp>

namespace otawa { namespace xengine {

/**
 * Defines the threshold, in number of events, to split an instruction block
 * to speed up calculation. This limit allows to get smaller XDD and therefore
 * to speed up the calculation. Notice there is no precision loss.
 *
 * Default value is 12.
 *
 * @ingroup xengine
 */
p::id<unsigned> SPLIT_THRESHOLD("otawa::xengine::SPLIT_THRESHOLD", 12);

/**
 * This feature ensures that an XDD representing the time has been computed for
 * each BB in isolation.
 *
 * Processors: @ref LocalBBXddTimeProcessor (default)
 *
 * @ingroup xengine
 */
p::feature LOCAL_BBTIMES_FEATURE("otawa::xengine::LOCAL_BBTIMES_FEATURE", p::make<LocalBBXddTimeProcessor>());



/**
 * This feature ensures that an XDD representing the time has been computed for
 * each BB relatively to the predecessor.
 *
 * Processors: none
 *
 * Properties:
 * * @ref BBTIMES (hooked to the preceding Edge)
 *
 * @ingroup xengine
 */
p::feature BBTIMES_FEATURE("otawa::xengine::BBTIMES_FEATURE");


/**
 * Property storing the time of a basic block. If several times are hooked to
 * a BB, the BB time is the sum of these times.
 *
 * Feature: @ref LOCAL_BBTIME_FEATURE
 *
 * Hook: Edge
 *
 * @ingroup xengine
 */
p::id<Xdd> BBTIMES("otawa::xengine::BBTIMES");


/**
 * @class LocalBBXddTimeProcessor
 * Compute the execution of each basic block with a local approach, considering
 * a time for each predecessor.
 *
 * Provides:
 * * @ref XDD_SIMPLE_BBTIME_FEATURE
 *
 * Requires:
 * * @ref XSTEPS_FEATURE
 * * @ref XENGINE_FEATURE
 *
 * @ingroup xengine
 */

///
p::declare LocalBBXddTimeProcessor::reg = p::init("otawa::xengine::LocalBBXddTimeProcessor", Version(1, 0, 0))
        .extend<BBProcessor>()
        .provide(LOCAL_BBTIMES_FEATURE)
		.provide(BBTIMES_FEATURE)
        .require(XSTEPS_FEATURE)
        .require(XENGINE_FEATURE)
        .make<LocalBBXddTimeProcessor>();

LocalBBXddTimeProcessor::LocalBBXddTimeProcessor()
	: BBProcessor(reg), _stats(nullptr)
	{}

///
void LocalBBXddTimeProcessor::setup(WorkSpace *ws) {
    BBProcessor::setup(ws);
    _xengine = XENGINE(ws);
	_xman = _xengine->getXMan();
	_rman = _xengine->getRMan();
	_compiler = new XStepsMatrixCompiler(_xengine);
	if(hasGlobalStats()) {
		_stats = new MatrixStats(*_xengine);
		_stats->start();
	}
}

///
void LocalBBXddTimeProcessor::processBB(WorkSpace *ws, CFG *cfg, Block *b) {
#	ifndef XDD_PARA
		if(!b->isBasic())
			return;
		const BasicBlock *bb = b->toBasic();
		for(Edge *edge: bb->inEdges())
				processEdge(edge);
#	endif
}


#ifdef XDD_PARA
	///
	void LocalBBXddTimeProcessor::processAll(WorkSpace *ws) {
		Producer producer(*this, COLLECTED_CFG_FEATURE.get(ws));
		sys::JobScheduler scheduler(producer);
		scheduler.start();
	}
#endif

/**
 * Compute time for an edge.
 * @param e		Edge to compute time for.
 */
void LocalBBXddTimeProcessor::processEdge(Edge *e) {
	Vector<const AbstractXStep *> xsteps;
	XddTimingState state(_rman, _xman);

	// split according to events
	for(auto xstep: *XSTEPS(e)) {
		if(xstep->type() != XStepType::Split)
			xsteps.push(xstep);
		else {
			auto split = static_cast<const class Split *>(xstep);
			computeTime(e, xsteps, state);
			xsteps.clear();
			auto base = state[_rman->getTimePointerIdx()];
			for(int i = 0; i < state.length(); i++)
				state[i] = state[i].sub_saturated(base);
		}
	}

	// compute last time
	if(!xsteps.isEmpty())
		computeTime(e, xsteps, state);
}


/**
 * Compute the time for executing passed xsteps with the passed state as input.
 * The state is updated after the execution.
 * @param e			Current edge.
 * @param xsteps	X steps to execute.
 * @param state		Input and output state.
 */
void LocalBBXddTimeProcessor::computeTime(
	Edge *e,
	const Vector<const AbstractXStep *>& xsteps,
	XddTimingState& state
) {

	// build the matrix
	XddMatrix *mat = _compiler->compileSequence(xsteps);
	if(_stats != nullptr)
		_stats->record(*mat);

	// compute the output state
	mat->VecXMat(state);
	ASSERT(state[_rman->getTimePointerIdx()].maxLeaf() > 0)
	delete mat;

	// record time
	BBTIMES(e).add(state[_rman->getTimePointerIdx()]);
	if (logFor(LOG_BB))
		log << "\t\t\tWCET of " << e << "=" << state[_rman->getTimePointerIdx()] << io::endl;
}


///
void LocalBBXddTimeProcessor::destroy(WorkSpace *ws) {
    BBProcessor::destroy(ws);
    delete _compiler;
	if(_stats != nullptr)
		delete _stats;
}

///
void LocalBBXddTimeProcessor::dumpGlobalStats(io::Output &out) {
	_stats->stop();
	_stats->display(out);
}


///
LocalBBXddTimeProcessor::Producer::Producer(
	LocalBBXddTimeProcessor& proc,
	const CFGCollection *coll
): _proc(proc), _block(coll) {
	ASSERT(coll);
	if(_block()) {
		ASSERTP(!_block->isBasic(), "first block should be an entry");
		nextBB();
	}
}

///
LocalBBXddTimeProcessor::Job *LocalBBXddTimeProcessor::Producer::next() {
	if(!_block())
		return nullptr;
	auto edge = *_ins;
	_ins++;
	if(!_ins())
		nextBB();
	return new Job(_proc, edge);
}

///
void LocalBBXddTimeProcessor::Producer::nextBB() {
	while(_block() && !_block->isBasic())
		_block++;
	if(_block())
		_ins = _block->inEdges().begin();
}


void LocalBBXddTimeProcessor::Job::run() {
	if(_proc.logFor(Processor::LOG_BLOCK))
		_proc.log << "\tcomputing " << _edge << " on thread " << (void *)this->thread() << io::endl;
	_proc.processEdge(_edge);
}

} } // otawa::xengine

