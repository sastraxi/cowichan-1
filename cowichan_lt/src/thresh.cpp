#include <iostream>
#include <cstdio>
#include <map>
#include "thresh.hpp"

const char* LTFrequency::SYNCH_LOCK = "thresh synch lock";
const char* LTFrequency::ROWS_DONE = "thresh rows reporting";

void CowichanLinuxTuples::thresh(IntMatrix matrix, BoolMatrix outMatrix) {

	// calculate the frequency breaking-point
	LTFrequency freq;
	freq.addInput(0, matrix);
	freq.start(SERVER, PORT, NUM_WORKERS);

	// calculate
	LTThresh thresh;
	thresh.addInput(0, matrix);
	thresh.addOutput(0, outMatrix);
	thresh.start(SERVER, PORT, NUM_WORKERS);

}

//===========================================================================//

void LTFrequency::consumeInput() {

	// create a tuple synch lock
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	put_tuple(synchLock, &ctx);

	// create a "rows reporting" tuple, so that we
	// know when the computation should end.
	tuple *rowsReporting = make_tuple("si", ROWS_DONE, 0);
	put_tuple(rowsReporting, &ctx);

	// TODO communicate all of the rows

}

void LTFrequency::work() {

	IntMatrix matrix = (IntMatrix) inputs[0];

	// tuple templates
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	tuple *recv = make_tuple("s?", "thresh request");

	while (1) {

		// block until we receieve a tuple.
		tuple* gotten = get_tuple(recv, &ctx);
		size_t y = gotten->elements[1].data.i;

		// perform the actual computation for this row (counting)
		std::map<INT_TYPE, size_t> freq;
		for (index_t x = 0; x < THRESH_NC; ++x) {
			++freq[MATRIX_RECT_NC(matrix, y,x, THRESH_NC)];
		}

		// purge local memory of the tuple we received
		destroy_tuple(gotten);

		// Now, we combine the results from this row with every other row.
		// enter the critical section
		get_tuple(synchLock, &ctx);

			// combine results with master copy
			std::map<INT_TYPE, size_t>::const_iterator it;
			for (it = freq.begin(); it != freq.end(); ++it) {
				tuple *templateHist = make_tuple("sii", "thresh hist", it->first);
				tuple *hist = get_nb_tuple(templateHist, &ctx);
				if (hist == NULL) {
					// use only this row's frequency
					templateHist->elements[2].data.i = it->second;
				} else {
					// combine this row's frequency and existing frequency
					templateHist->elements[2].data.i = it->second + hist->elements[2].data.i;
					destroy_tuple(hist);
				}
				put_tuple(templateHist, &ctx);
				destroy_tuple(templateHist);
			}

			// record the number of rows reporting
			tuple *templateRowsReporting = make_tuple("si", ROWS_DONE);
			tuple *rowsReporting = get_tuple(templateRowsReporting, &ctx);
			rowsReporting->elements[1].data.i += 1;
			put_tuple(rowsReporting, &ctx);

		// leave the critical section
		put_tuple(synchLock, &ctx);

	}

}

void LTFrequency::produceOutput() {

	// wait for all rows to be done.
	tuple *allRowsReporting = make_tuple("si", ROWS_DONE, THRESH_NR);
	get_tuple(allRowsReporting, &ctx);

	// figure out when we have to stop.
	index_t retain = (index_t)(THRESH_PERCENT * THRESH_NC * THRESH_NR);

	// go through all tuple values until we have reached the threshold point
	for (size_t n = 0; n < MAXIMUM_INT; ++n) {
		tuple *templateHist = make_tuple("sii", "thresh hist", n);
		tuple *hist = get_nb_tuple(templateHist, &ctx);
		if (hist != NULL) {
			retain -= hist->elements[2].data.i;
		}
		if (retain <= 0) {
			retain = n;
			break;
		}
	}

	// retain now holds the threshold point.
	// communicate this into the tuple space
	tuple *threshPoint = make_tuple("si", "thresh point", retain);
	put_tuple(threshPoint, &ctx);

	// remove the tuple synch lock from tuple space
	tuple *synchLock = make_tuple("s", SYNCH_LOCK);
	get_tuple(synchLock, &ctx);

}

//===========================================================================//

void LTThresh::consumeInput() {

	// tuple template
	tuple *send = make_tuple("si", "thresh request");

	// send off a request for each grid row.
	for (size_t y = 0; y < THRESH_NR; ++y) {
		send->elements[1].data.i = y;
		put_tuple(send, &ctx);
	}

	// destroy the template tuple
	destroy_tuple(send);

}

void LTThresh::work() {

	tuple *recv = make_tuple("s?", "thresh request");
	tuple *send = make_tuple("sis", "thresh done");

	// grab pointers locally.
	IntMatrix input = (IntMatrix) inputs[0];

	// get the threshold point from the frequency calculation
	tuple *templateThreshPoint = make_tuple("si", "thresh point");
	tuple *threshPoint = get_tuple(templateThreshPoint, &ctx);
	size_t threshold = threshPoint->elements[1].data.i;

	// satisfy thresh requests.
	while (1) {

		// block until we receive a tuple.
		tuple* gotten = get_tuple(recv, &ctx);

		// copy over row co-ordinate of the computation; create
		// a buffer for the results of the computation.
		size_t y = gotten->elements[1].data.i;
		send->elements[1].data.i = y;
		bool* buffer = (bool*) malloc(sizeof(bool) * THRESH_NC);
		send->elements[2].data.s.len = sizeof(bool) * THRESH_NC;
		send->elements[2].data.s.ptr = (char*) buffer;

		// perform the actual computation for this row.
		for (int x = 0; x < THRESH_NC; ++x) {
			buffer[x] = MATRIX_RECT_NC(input, y,x, THRESH_NC) > threshold;
		}

		// send off the new tuple and purge local memory of the one we got
		put_tuple(send, &ctx);
		destroy_tuple(gotten);

	}

	// TODO destroy the template tuples; must send tuples for this
//	destroy_tuple(send);
//	destroy_tuple(recv);

}

void LTThresh::produceOutput() {

	// tuple template
	tuple *recv = make_tuple("s??", "thresh done");

	// grab output pointer locally.
	IntMatrix output = (IntMatrix) outputs[0];

	// grab all of the mandelbrot computations from the workers,
	// in an unspecified order.
	int computations = THRESH_NR;
	while (computations > 0) {

		// get the tuple and copy it into the matrix.
		tuple* received = get_tuple(recv, &ctx);
		memcpy(
			&MATRIX_RECT_NC(output, received->elements[1].data.i, 0, THRESH_NC),
			received->elements[2].data.s.ptr,
			received->elements[2].data.s.len);
		computations--;
		destroy_tuple(received);

	}

	// destroy the template tuple
	destroy_tuple(recv);

}
