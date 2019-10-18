/*
 * File:   NeighborAcquirerTest.h
 * Author: bierth, seckler
 *
 * Created on February 27, 2018, 5:01 PM
 */

#include "NeighborAcquirerTest.h"
#include "parallel/NeighborAcquirer.h"

using namespace std;

TEST_SUITE_REGISTRATION(NeighborAcquirerTest);

NeighborAcquirerTest::NeighborAcquirerTest() {
	_fullShell = new FullShell();
}

NeighborAcquirerTest::~NeighborAcquirerTest() { delete _fullShell; }

void NeighborAcquirerTest::testShiftIfNecessary() {
	HaloRegion region; // rmin, rmax, offset, width
	std::array<double,3> domainLength = {10.0, 10.0, 10.0};
	double shift[3] = {0.0};
	
	// region does not need to be shifted
	for(int i = 0; i < 3; i++) region.rmax[i] = 5.0;
	for(int i = 0; i < 3; i++) region.rmin[i] = 3.0;

	region = NeighborAcquirer::getPotentiallyShiftedRegion(domainLength, region, shift, 0.); // region is within domain box
	
	for(int i = 0; i < 3; i++) ASSERT_EQUAL(shift[i], 0.0);
	
	for(int i = 0; i < 3; i++) region.rmax[i] = 12.0;
	for(int i = 0; i < 3; i++) region.rmin[i] = 11.0;

	region = NeighborAcquirer::getPotentiallyShiftedRegion(domainLength, region, shift, 0.);
	
	for(int i = 0; i < 3; i++) { ASSERT_EQUAL(shift[i], -10.0); shift[i] = 0.0; }
	
	for(int i = 0; i < 3; i++) region.rmax[i] = 0.0;
	for(int i = 0; i < 3; i++) region.rmin[i] = -1.0;

	region = NeighborAcquirer::getPotentiallyShiftedRegion(domainLength, region, shift, 0.);
	
	for(int i = 0; i < 3; i++) { ASSERT_EQUAL(shift[i], 10.0); shift[i] = 0.0; }
	
	
}

void NeighborAcquirerTest::testOverlap() { // assume this one works for now, because you thought about it long and hard.
	HaloRegion region01;
	HaloRegion region02;
	
	for(int i = 0; i < 3; i++) { 
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 3.0;
	}
	
	NeighborAcquirer::overlap(&region01, &region02);
	
	for(int i = 0; i < 3; i++) {
		ASSERT_EQUAL(region02.rmax[i], 4.0);
		ASSERT_EQUAL(region02.rmin[i], 3.0);
	}
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 6.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 5.0;
		region02.rmin[i] = 3.0;
	}
	
	NeighborAcquirer::overlap(&region01, &region02);
	
	for(int i = 0; i < 3; i++) {
		ASSERT_EQUAL(region02.rmax[i], 5.0);
		ASSERT_EQUAL(region02.rmin[i], 3.0);
	}
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 3.0;
		region02.rmin[i] = 1.0;
	}
	
	NeighborAcquirer::overlap(&region01, &region02);
	
	for(int i = 0; i < 3; i++) {
		ASSERT_EQUAL(region02.rmax[i], 3.0);
		ASSERT_EQUAL(region02.rmin[i], 2.0);
	}
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 1.0;
	}
	
	NeighborAcquirer::overlap(&region01, &region02);
	
	for(int i = 0; i < 3; i++) {
		ASSERT_EQUAL(region02.rmax[i], 4.0);
		ASSERT_EQUAL(region02.rmin[i], 2.0);
	}
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 6.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 2.0;
	}
	
	NeighborAcquirer::overlap(&region01, &region02);
	
	for(int i = 0; i < 3; i++) {
		ASSERT_EQUAL(region02.rmax[i], 6.0);
		ASSERT_EQUAL(region02.rmin[i], 2.0);
	}
}

void NeighborAcquirerTest::testIOwnThis() { // i own a part of this
	HaloRegion region01;
	HaloRegion region02;
	
	for(int i = 0; i < 3; i++) { 
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 3.0;
	}
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), true);
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 6.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 5.0;
		region02.rmin[i] = 3.0;
	}
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), true);
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 3.0;
		region02.rmin[i] = 1.0;
	}
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), true);
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 4.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 1.0;
	}
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), true);
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 6.0;
		region01.rmin[i] = 2.0;
		region02.rmax[i] = 6.0;
		region02.rmin[i] = 2.0;
	}
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), true);
	
	for(int i = 0; i < 3; i++) {
		region01.rmax[i] = 5.0;
		region01.rmin[i] = 1.0;
	}
	
	region02.rmax[0] = 6.0;
	region02.rmax[1] = 7.0;
	region02.rmax[2] = 5.0;
	
	region02.rmin[0] = 5.0;
	region02.rmin[1] = 6.0;
	region02.rmin[2] = 5.0;
	
	ASSERT_EQUAL(NeighborAcquirer::isIncluded(&region01, &region02), false);
	
	
}
