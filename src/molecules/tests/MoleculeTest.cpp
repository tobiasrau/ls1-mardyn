/*
 * MoleculeTest.cpp
 *
 * @Date: 04.02.2011
 * @Author: eckhardw
 */

#include "MoleculeTest.h"
#include "molecules/Molecule.h"

TEST_SUITE_REGISTRATION(MoleculeTest);

MoleculeTest::MoleculeTest() {

}

MoleculeTest::~MoleculeTest() {
}

void MoleculeTest::testSerialize() {
	std::vector<Component> components;
    decltype(std::declval<Component>().ID()) const id = 0;
	Component dummyComponent(id);
	dummyComponent.addLJcenter(0,0,0,0,0,0,0,false);
	components.push_back(dummyComponent);

	Molecule a(
        0,                      //id
        &components[0],         //component pointer
        1.0, 2.0, 3.0,          //r
        4.0, 5.0, 6.0,          //v
        7.0, 8.0, 9.0, 10.0,    //q
        11.0, 12.0, 13.0        //D
    );

    std::vector<char> buffer;
    a.serialize()
}


void MoleculeTest::testIsLessThan() {
	std::vector<Component> components;
	Component dummyComponent(0);
	dummyComponent.addLJcenter(0,0,0,0,0,0,0,false);
	components.push_back(dummyComponent);

	Molecule a(0, &components[0], 1.0, 1.0, 1.0,0,0,0,0,0,0,0,0,0,0);
	Molecule b(0, &components[0], 2.0, 2.0, 2.0,0,0,0,0,0,0,0,0,0,0);

	ASSERT_TRUE(a.isLessThan(b));
	ASSERT_TRUE(!b.isLessThan(a));

	a.setr(2, 3.0);

	ASSERT_TRUE(!a.isLessThan(b));
	ASSERT_TRUE(b.isLessThan(a));

	a.setr(2, 2.0);
	ASSERT_TRUE(a.isLessThan(b));
	ASSERT_TRUE(!b.isLessThan(a));

	a.setr(1, 3.0);

	ASSERT_TRUE(!a.isLessThan(b));
	ASSERT_TRUE(b.isLessThan(a));

	a.setr(1, 2.0);
	ASSERT_TRUE(a.isLessThan(b));
	ASSERT_TRUE(!b.isLessThan(a));

	a.setr(0, 3.0);

	ASSERT_TRUE(!a.isLessThan(b));
	ASSERT_TRUE(b.isLessThan(a));
}
