//
// Created by Helge Penne on 01/11/2023.
//

#ifndef GTEST_DEMO_BAR_H
#define GTEST_DEMO_BAR_H

#include "owned_ptr.h"

class Foo;

class Bar {
public:
    Bar();

private:
    owned_ptr<Foo> _foo;
};


#endif //GTEST_DEMO_BAR_H
