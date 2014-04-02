/*
 * IncidenceMatrixGTest.cpp
 *
 *  Created on: 01.04.2014
 *      Author: Michael
 */

#include "IncidenceMatrixGTest.h"

IncidenceMatrixGTest::IncidenceMatrixGTest() {
}

IncidenceMatrixGTest::~IncidenceMatrixGTest() {
}

TEST_F(IncidenceMatrixGTest, tryElementAccess) {
	IncidenceMatrix mat(graph);

	EXPECT_EQ(1.0, mat(0,0));
	EXPECT_EQ(-1.0, mat(1,0));
	for (uint64_t i = 2; i < mat.numberOfRows(); ++i) {
		EXPECT_EQ(0.0, mat(i, 0));
	}

	EXPECT_EQ(-1.0, mat(2,1));

	EXPECT_EQ(-1.0, mat(3,2));
	EXPECT_EQ(-1.0, mat(3,3));

	for (uint64_t i = 0; i < mat.numberOfRows(); ++i) {
		EXPECT_EQ(0.0, mat(i, 5));
	}
}

TEST_F(IncidenceMatrixGTest, tryRowAndColumnAccess) {
	IncidenceMatrix mat(graph);

	Vector row0 = mat.row(0);
	ASSERT_EQ(row0.getDimension(), mat.numberOfColumns());

	EXPECT_EQ(1.0, row0(0));
	EXPECT_EQ(1.0, row0(1));
	EXPECT_EQ(1.0, row0(2));
	for (uint64_t j = 3; j < row0.getDimension(); ++j) {
		EXPECT_EQ(0.0, row0(j));
	}

	for (uint64_t j = 0; j < 5; ++j) {
		Vector column = mat.column(j);
		ASSERT_EQ(column.getDimension(), mat.numberOfRows());

		double sum = 0.0;
		for (uint64_t i = 0; i < column.getDimension(); ++i) {
			sum += column(i);
		}

		EXPECT_EQ(0.0, sum);
	}

	Vector column5 = mat.column(5);
	ASSERT_EQ(column5.getDimension(), mat.numberOfRows());

	for (uint64_t i = 0; i < column5.getDimension(); ++i) {
		EXPECT_EQ(0.0, column5(i));
	}
}

TEST_F(IncidenceMatrixGTest, tryMatrixVectorProduct) {
	IncidenceMatrix mat(graph);
	Vector v = {12, 3, 9, 28, 0, -1};

	Vector result = mat * v;
	ASSERT_EQ(result.getDimension(), mat.numberOfRows());

	EXPECT_EQ(24, result(0));
	EXPECT_EQ(-12, result(1));
	EXPECT_EQ(25, result(2));
	EXPECT_EQ(-37, result(3));
	EXPECT_EQ(0, result(4));
}
