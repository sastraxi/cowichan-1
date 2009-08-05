#include <iostream>
#include <cstdio>
#include <cmath>
#include "winnow.hpp"

const char* LTWinnow::SYNCH_LOCK = "winnow synchronization lock";
const char* LTWinnow::REQUEST = "winnow request";
const char* LTWinnow::WEIGHTED_POINT = "winnow weighted point";
const char* LTWinnow::ROWS_REPORTING = "winnow rows reporting";
const char* LTWinnow::COUNT = "winnow number of points";

void CowichanLinuxTuples::winnow(IntMatrix matrix, BoolMatrix mask, PointVector points) {

	bool notEnoughPoints = false;

	// calculate the 2D bounds of the point cloud
	LTWinnow program;
	program.addInput(0, matrix);
	program.addInput(1, mask);
	program.addOutput(0, points);
	program.addOutput(1, &notEnoughPoints);
	program.start(SERVER, PORT, NUM_WORKERS);

	// error condition set by LTWinnow::produceOutput
	if (notEnoughPoints) {
		not_enough_points();
	}

}

//===========================================================================//

void LTWinnow::consumeInput() {

	// create a tuple synch lock
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	put_tuple(synchLock, &ctx);

	// tuple template.
	tuple *send = make_tuple("si", REQUEST);

	// allow workers to grab work by-the-row
	for (index_t row = 0; row < WINNOW_NR; ++row) {
		send->elements[1].data.i = row;
		put_tuple(send, &ctx);
	}

}

void LTWinnow::work() {

	// tuple templates
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	tuple *recv = make_tuple("s?", REQUEST);
	tuple *send = make_tuple("ssi", WEIGHTED_POINT);

	// grab pointers locally.
	IntMatrix matrix = (IntMatrix) inputs[0];
	BoolMatrix mask = (BoolMatrix) inputs[1];

	while (1) {

		// block until we receive a tuple.
		tuple* gotten = get_tuple(recv, &ctx);
		index_t r = gotten->elements[1].data.i;

		// perform the actual computation for these elements;
		// check mask and add points to tuple space/count
		index_t sum = 0;
		for (index_t c = 0; c < WINNOW_NC; ++c) {
			if (MATRIX_RECT_NC(mask, r, c, WINNOW_NC)) {

				++sum;

				// put a point, with weight, into tuple space
				Point pt((real)c, (real)r);
				send->elements[1].data.s.len = sizeof(WeightedPoint);
				send->elements[1].data.s.ptr = (char*) &pt;
				send->elements[2].data.i = MATRIX_RECT_NC(matrix, r, c, WINNOW_NC); // weight
				put_tuple(send, &ctx);

			}
		}

		// purge local memory of the tuple we received/generated
		destroy_tuple(gotten);
		destroy_tuple(send);

		// Now, we combine the results from these points with the "world".
		// We are computing the number of points that are not masked off.
		get_tuple(synchLock, &ctx);

			// combine results with master copy (max element difference)
			tuple *tmpCount = make_tuple("s?", COUNT);
			tuple *tupleCount = get_nb_tuple(tmpCount, &ctx);
			if (tupleCount != NULL) {
				sum += tupleCount->elements[1].data.i;
				destroy_tuple(tupleCount);
			}
			tmpCount->elements[1].data.d = sum;
			put_tuple(tmpCount, &ctx);
			destroy_tuple(tmpCount);

		// leave the critical section
		put_tuple(synchLock, &ctx);

	}

}

Point LTWinnow::nextWeightedPoint(tuple *recv, INT_TYPE* order) {

	// Grab the next weighted point from tuple space.
	tuple* gotten = NULL;
	while (true) {
		recv->elements[2].data.i = *order;
		gotten = get_nb_tuple(recv, &ctx);
		if (gotten) {
			break; // we can exit the function soon.
		} else {
			++(*order); // go onto next potential weight
		}
	}

	// strip data out of the received tuple and destroy it.
	Point pt = *((Point*) gotten->elements[1].data.s.ptr);
	destroy_tuple(gotten);
	destroy_tuple(recv);

	// return the point.
	return pt;

}

void LTWinnow::produceOutput() {

	// tuple templates.
	tuple *recv = make_tuple("s?i", WEIGHTED_POINT);

	// grab output pointers locally.
	PointVector points = (PointVector) outputs[0];

	// grab the count of weighted points generated.
	tuple *tmpCount = make_tuple("s?", COUNT);
	tuple *tupleCount = get_tuple(tmpCount, &ctx);
	index_t count = tupleCount->elements[1].data.i;
	if (count < WINNOW_N) {

		// error condition; flag caller.
		bool* notEnoughPoints = (bool*) outputs[1];
		*notEnoughPoints = true;
		return;

	}

	// selection stride.
	index_t stride = count / WINNOW_N;

	// loop over all generated points.
	Point current;
	INT_TYPE order = MINIMUM_INT;
	index_t pos = 0;
	while (pos < count) {

		// skip over as many points as we need to
		for (index_t i = 0; i < stride; ++i) {
			current = nextWeightedPoint(recv, &order);
		}

		// put one in the list
		points[pos] = current;

	}

	// remove the tuple synch lock from tuple space
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	get_tuple(synchLock, &ctx);

	// clean-up local tuple memory.
	destroy_tuple(synchLock);
	destroy_tuple(recv);
	destroy_tuple(tmpCount);
	destroy_tuple(tupleCount);

}