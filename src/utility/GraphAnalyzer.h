/*
 * GraphAnalyzer.h
 *
 *  Created on: 28.11.2012
 *      Author: Christian Dehnert
 */

#ifndef STORM_UTILITY_GRAPHANALYZER_H_
#define STORM_UTILITY_GRAPHANALYZER_H_

#include "src/models/AbstractDeterministicModel.h"
#include "src/exceptions/InvalidArgumentException.h"

#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"

extern log4cplus::Logger logger;

namespace storm {

namespace utility {

class GraphAnalyzer {
public:
	/*!
	 * Computes the sets of states that have probability 0 or 1, respectively, of satisfying phi until psi in a
	 * deterministic model.
	 * @param model The model whose graph structure to search.
	 * @param phiStates The set of all states satisfying phi.
	 * @param psiStates The set of all states satisfying psi.
	 * @param statesWithProbability0 A pointer to a bit vector that is initially empty and will contain all states with
	 * probability 0 after the invocation of the function.
	 * @param statesWithProbability1 A pointer to a bit vector that is initially empty and will contain all states with
	 * probability 1 after the invocation of the function.
	 */
	template <class T>
	static void performProb01(storm::models::AbstractDeterministicModel<T>& model, const storm::storage::BitVector& phiStates, const storm::storage::BitVector& psiStates, storm::storage::BitVector* statesWithProbability0, storm::storage::BitVector* statesWithProbability1) {
		// Check for valid parameters.
		if (statesWithProbability0 == nullptr) {
			LOG4CPLUS_ERROR(logger, "Parameter 'statesWithProbability0' must not be null.");
			throw storm::exceptions::InvalidArgumentException("Parameter 'statesWithProbability0' must not be null.");
		}
		if (statesWithProbability1 == nullptr) {
			LOG4CPLUS_ERROR(logger, "Parameter 'statesWithProbability1' must not be null.");
			throw storm::exceptions::InvalidArgumentException("Parameter 'statesWithProbability1' must not be null.");
		}

		// Perform the actual search.
		GraphAnalyzer::performProbGreater0(model, phiStates, psiStates, statesWithProbability0);
		GraphAnalyzer::performProb1(model, phiStates, psiStates, *statesWithProbability0, statesWithProbability1);
		statesWithProbability0->complement();
	}

	/*!
	 * Performs a backwards depth-first search trough the underlying graph structure
	 * of the given model to determine which states of the model have a positive probability
	 * of satisfying phi until psi. The resulting states are written to the given bit vector.
	 * @param model The model whose graph structure to search.
	 * @param phiStates A bit vector of all states satisfying phi.
	 * @param psiStates A bit vector of all states satisfying psi.
	 * @param statesWithProbabilityGreater0 A pointer to the result of the search for states that possess
	 * a positive probability of satisfying phi until psi.
	 */
	template <class T>
	static void performProbGreater0(storm::models::AbstractDeterministicModel<T>& model, const storm::storage::BitVector& phiStates, const storm::storage::BitVector& psiStates, storm::storage::BitVector* statesWithProbabilityGreater0) {
		// Check for valid parameter.
		if (statesWithProbabilityGreater0 == nullptr) {
			LOG4CPLUS_ERROR(logger, "Parameter 'statesWithProbabilityGreater0' must not be null.");
			throw storm::exceptions::InvalidArgumentException("Parameter 'statesWithProbabilityGreater0' must not be null.");
		}

		// Get the backwards transition relation from the model to ease the search.
		storm::models::GraphTransitions<T>& backwardTransitions = model.getBackwardTransitions();

		// Add all psi states as the already satisfy the condition.
		*statesWithProbabilityGreater0 |= psiStates;

		// Initialize the stack used for the DFS with the states
		std::vector<uint_fast64_t> stack;
		stack.reserve(model.getNumberOfStates());
		psiStates.getList(stack);

		// Perform the actual DFS.
		while(!stack.empty()) {
			uint_fast64_t currentState = stack.back();
			stack.pop_back();

			for(auto it = backwardTransitions.beginStateSuccessorsIterator(currentState); it != backwardTransitions.endStateSuccessorsIterator(currentState); ++it) {
				if (phiStates.get(*it) && !statesWithProbabilityGreater0->get(*it)) {
					statesWithProbabilityGreater0->set(*it, true);
					stack.push_back(*it);
				}
			}
		}
	}

	/*!
	 * Computes the set of states of the given model for which all paths lead to
	 * the given set of target states and only visit states from the filter set
	 * before. In order to do this, it uses the given set of states that
	 * characterizes the states that possess at least one path to a target state.
	 * The results are written to the given bit vector.
	 * @param model The model whose graph structure to search.
	 * @param phiStates A bit vector of all states satisfying phi.
	 * @param psiStates A bit vector of all states satisfying psi.
	 * @param statesWithProbabilityGreater0 A reference to a bit vector of states that possess a positive
	 * probability mass of satisfying phi until psi.
	 * @param alwaysPhiUntilPsiStates A pointer to the result of the search for states that only
	 * have paths satisfying phi until psi.
	 */
	template <class T>
	static void performProb1(storm::models::AbstractDeterministicModel<T>& model, const storm::storage::BitVector& phiStates, const storm::storage::BitVector& psiStates, const storm::storage::BitVector& statesWithProbabilityGreater0, storm::storage::BitVector* statesWithProbability1) {
		// Check for valid parameter.
		if (statesWithProbability1 == nullptr) {
			LOG4CPLUS_ERROR(logger, "Parameter 'statesWithProbability1' must not be null.");
			throw storm::exceptions::InvalidArgumentException("Parameter 'statesWithProbability1' must not be null.");
		}

		GraphAnalyzer::performProbGreater0(model, ~psiStates, ~statesWithProbabilityGreater0, statesWithProbability1);
		statesWithProbability1->complement();
	}

	/*!
	 * Computes the set of states of the given model for which all paths lead to
	 * the given set of target states and only visit states from the filter set
	 * before. In order to do this, it uses the given set of states that
	 * characterizes the states that possess at least one path to a target state.
	 * The results are written to the given bit vector.
	 * @param model The model whose graph structure to search.
	 * @param phiStates A bit vector of all states satisfying phi.
	 * @param psiStates A bit vector of all states satisfying psi.
	 * @param alwaysPhiUntilPsiStates A pointer to the result of the search for states that only
	 * have paths satisfying phi until psi.
	 */
	template <class T>
	static void performProb1(storm::models::AbstractDeterministicModel<T>& model, const storm::storage::BitVector& phiStates, const storm::storage::BitVector& psiStates, storm::storage::BitVector* statesWithProbability1) {
		// Check for valid parameter.
		if (statesWithProbability1 == nullptr) {
			LOG4CPLUS_ERROR(logger, "Parameter 'statesWithProbability1' must not be null.");
			throw storm::exceptions::InvalidArgumentException("Parameter 'statesWithProbability1' must not be null.");
		}

		storm::storage::BitVector* statesWithProbabilityGreater0 = new storm::storage::BitVector(model.getNumberOfStates());
		GraphAnalyzer::performProbGreater0(model, phiStates, psiStates, statesWithProbabilityGreater0);
		GraphAnalyzer::performProbGreater0(model, ~psiStates, ~(*statesWithProbabilityGreater0), statesWithProbability1);
		delete statesWithProbabilityGreater0;
		statesWithProbability1->complement();
	}
};

} // namespace utility

} // namespace storm

#endif /* STORM_UTILITY_GRAPHANALYZER_H_ */
